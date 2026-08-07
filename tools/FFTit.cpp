//
// Copyright (c) 2008-2022 Paul Ranson, paul@epicyclism.com
//
// Refer to licence in repository.
//
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <charconv>

#include <fmt/format.h>

#include "fftlib.h"
#include "mm_file.h"

template <typename T> void from_chars(char const* arg, T& result)
{
	auto [ptr, ec] = std::from_chars(arg, arg + strlen(arg), result);
}

void welcome()
{
	fmt::println("FFTit 2.00\n");
	fmt::println("Performs FFT on a file of raw sample data");
	fmt::println("(sizeof fp type is {})", sizeof(fp_t));
}

void usage()
{
	fmt::println("Performs FFTs on a file of raw sample data");
	fmt::println("Usage : FFTit [-Fn] [-D] [-1] [-Wn] <input file> [sample rate]");
	fmt::println("Where input file is a packed array of floats. Output is text to stdout.");
	fmt::println("Options. -Fn, use an FFT width of 2^n.");
	fmt::println("              n between 8 for 256 and 24 for 16777216.");
	fmt::println("              Default is 18 for 262144");
	fmt::println("         -D,  output in dB scaled so 1.0 is 0dB");
	fmt::println("         -1,  perform a single FFT on the centre FFT width samples of the");
	fmt::println("              input, otherwise (default) process the entire file and average");
	fmt::println("              the results of each FFT.");
	fmt::println("         -Wn, select a window function. 0 is no window.");
	fmt::println("				1 is Hamming and the default.");
	fmt::println("              2 is Blackman, 3 Blackman-Harris.");
	fmt::println("              4 is Kaiser5,  5 Kaiser7.");
	fmt::println("And if you provide the sample rate, the centre frequencies of each bin are written to the output.");
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
				fmt::print(stderr, "Unknown argument \'{:c}\'!\n", argv[arg][1]);
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
		fmt::println("Sample rate provided was not understood");
		usage();
		return -1;
	}
	if (fftWidth < FFTWdMin || fftWidth > FFTWdMax)
	{
		fmt::println("FFTWidth provided is out of range, valid between {:d} and {:d} inclusive.", FFTWdMin, FFTWdMax);
		usage();
		return -1;
	}
	mem_map_file<fp_t> mmf(argv[nInFileArg]);
	if (!mmf)
	{
		fmt::println("Couldn't open <{}>", argv[nInFileArg]);
		return -1;
	}

	// an FFT implementation!
	auto pfft = make_fft(fftWidth, wt);
	std::vector<fp_t> mean(pfft->width());

	// report
	fmt::println("FFTit. Processing,  width {:d}, window {}", pfft->width(), wt_to_string(wt));

	if (bOnce)
	{
		if (mmf.length() < pfft->width())
		{
			fmt::println("Insufficient signal supplied for the specified FFT width");
			return -1;
		}
		size_t offset = (mmf.length() - pfft->width()) / 2;
		// just a single effort
		auto[ob, oe] = (*pfft) (mmf.ptr() + offset, mmf.ptr() + offset + pfft->width());
		std::copy(ob, oe, mean.begin());
	}
	else
	{
		// 50% overlap
		size_t nffts = mmf.length();
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
			fmt::println("Insufficient signal supplied for the specified FFT width");
			return -1;
		}

		for (size_t n = 0; n < nffts; ++n)
		{
			auto[ob, oe] = (*pfft) (mmf.ptr() + n * pfft->width() / 2, mmf.ptr() + n * pfft->width() / 2 + pfft->width());
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