//
// Copyright (c) 2008-2022 Paul Ranson, paul@epicyclism.com
//
// Refer to licence in repository.
//

#include <iostream>
#include <string_view>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <charconv>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "fftlib.h"
#include "mm_file.h"
#include "audio_file_reader.h"

// if an audio file, read it, if a raw data file, map it.
// return ptr/size pair
//
struct signal_wrap
{
	mem_map_file<fp_t> mmf_;
	std::vector<fp_t> data_;

	signal_wrap(const char* fn)
	{
		std::string_view  fns(fn);
		if (fns.ends_with(".wav") || fns.ends_with(".WAV") || fns.ends_with(".flac") || fns.ends_with(".FLAC"))
		{
			auto [data, sample_rate] = read_audio_file(fn, 0);
			data_ = std::move(data);
		}
		else
		{
			mmf_.open(fn);
		}
	}
	std::pair<fp_t const*, size_t> get() const
	{
		if (mmf_)
			return { mmf_.ptr(), mmf_.length() };
		else
			return { data_.data(), data_.size() };
	}
};

template <typename T> void from_chars(char const* arg, T& result)
{
	auto [ptr, ec] = std::from_chars(arg, arg + strlen(arg), result);
}

void welcome()
{
	fmt::println(std::cerr,"FFTit 2.00\n");
	fmt::println(std::cerr,"Performs FFT on a file of raw sample data");
	fmt::println(std::cerr,"(sizeof fp type is {})", sizeof(fp_t));
}

void usage()
{
	fmt::println(std::cerr,"Performs FFTs on a file of raw sample data");
	fmt::println(std::cerr,"Usage : FFTit [-Fn] [-D] [-1] [-Wn] <input file> [sample rate]");
	fmt::println(std::cerr,"Where input file is a packed array of floats. Output is text to stdout.");
	fmt::println(std::cerr,"Options. -Fn, use an FFT width of 2^n.");
	fmt::println(std::cerr,"              n between 8 for 256 and 24 for 16777216.");
	fmt::println(std::cerr,"              Default is 18 for 262144");
	fmt::println(std::cerr,"         -D,  output in dB scaled so 1.0 is 0dB");
	fmt::println(std::cerr,"         -1,  perform a single FFT on the centre FFT width samples of the");
	fmt::println(std::cerr,"              input, otherwise (default) process the entire file and average");
	fmt::println(std::cerr,"              the results of each FFT.");
	fmt::println(std::cerr,"         -Wn, select a window function. 0 is no window.");
	fmt::println(std::cerr,"              1 is Hamming and the default.");
	fmt::println(std::cerr,"              2 is Blackman, 3 Blackman-Harris.");
	fmt::println(std::cerr,"              4 is Kaiser5,  5 Kaiser7.");
	fmt::println(std::cerr,"And if you provide the sample rate, the centre frequencies of each bin are written to the output.");
}

int main(int argc, char* argv[])
{
	welcome();

	if (argc < 2)
	{
		usage();

		return -1;
	}
	int		nInFileArg = 0;
	size_t  fftWidth = 18;
	bool    bDB = false;
	bool    bOnce = false;
	size_t sample_rate = -1;
	window_t wt = window_t::HAMMING;

	int		arg = 1;
	while (arg < argc)
	{
		if (argv[arg][0] == '-' || argv[arg][0] == '/')
		{
			switch (argv[arg][1])
			{
			case 'F':
			case 'f':
				from_chars(argv[arg] + 2, fftWidth);
				break;
			case 'D':
			case 'd':
				bDB = true;
				break;
			case '1':
				bOnce = true;
				break;
			case 'W':
			case 'w':
				wt = wt_from_code(*(argv[arg] + 2));
				break;
			default:
				fmt::println(std::cerr, "Unknown argument \'{:c}\'!", argv[arg][1]);
				usage();
				return -1;
			}
		}
		else
		{
			if (nInFileArg == 0)
			{
				nInFileArg = arg;
			}
			else
			{
				from_chars(argv[arg], sample_rate);
			}
		}
		++arg;
	}
	// checks
	if (sample_rate == 0)
	{
		fmt::println(std::cerr, "Sample rate provided was not understood");
		usage();
		return -1;
	}
	if (fftWidth < FFTWdMin || fftWidth > FFTWdMax)
	{
		fmt::println(std::cerr, "FFTWidth provided is out of range, valid between {:d} and {:d} inclusive.", FFTWdMin, FFTWdMax);
		usage();
		return -1;
	}
#if 0
	mem_map_file<fp_t> mmf(argv[nInFileArg]);
	if (!mmf)
	{
		fmt::println(std::cerr, "Couldn't open <{}>", argv[nInFileArg]);
		return -1;
	}
#else
	signal_wrap sw(argv[nInFileArg]);
	auto [ptr, len] = sw.get();
	if(len == 0)
	{
		fmt::println(std::cerr, "Couldn't open <{}>", argv[nInFileArg]);
		return -1;
	}
#endif
	// an FFT implementation!
	auto pfft = make_fft(fftWidth, wt);
	std::vector<fp_t> mean(pfft->width());

	// report
	fmt::println(std::cerr, "FFTit. Processing,  width {:d}, window {}", pfft->width(), wt_to_string(wt));

	if (bOnce)
	{
		if (len < pfft->width())
		{
			fmt::println(std::cerr, "Insufficient signal supplied for the specified FFT width");
			return -1;
		}
		size_t offset = (len - pfft->width()) / 2;
		// just a single effort
		auto[ob, oe] = (*pfft) (ptr + offset, ptr + offset + pfft->width());
		std::copy(ob, oe, mean.begin());
	}
	else
	{
		// 50% overlap
		size_t nffts = len;
		if (nffts >= pfft->width() * 3 / 2)
		{
			nffts /= (pfft->width() / 2);
			nffts -= 1;
		}
		else
		if (nffts > pfft->width())
			nffts = 1;
		else
		{
			fmt::println(std::cerr, "Insufficient signal supplied for the specified FFT width");
			return -1;
		}

		for (size_t n = 0; n < nffts; ++n)
		{
			auto[ob, oe] = (*pfft) (ptr + n * pfft->width() / 2, ptr + n * pfft->width() / 2 + pfft->width());
			// add to average
			std::transform(mean.begin(), mean.end(), ob, mean.begin(), std::plus<>());
		}
		using namespace std::placeholders;
		std::transform(mean.begin(), mean.end(), mean.begin(), std::bind(std::divides<fp_t>(), _1, fp_t(nffts)));
	}
	if (sample_rate != -1)
	{
		double fb = 0;
		double fbinc = double(sample_rate) / pfft->width();
		for (size_t i = 0; i < pfft->width() / 2; ++i)
		{
			fp_t out;
			if (bDB)
				out = fp_t(20.0) * log10(mean[i]);
			else
				out = mean[i];

			fmt::println("{:.6f} {:.6f}", fb, out);
			fb += fbinc;
		}
	}
	else
	{
		for (size_t i = 0; i < pfft->width() / 2; ++i)
		{
			fp_t out;
			if (bDB)
				out = fp_t(20.0) * log10(mean[i]);
			else
				out = mean[i];
			fmt::println("{:.6f}\n", out);
		}
	}

	return 0;
}