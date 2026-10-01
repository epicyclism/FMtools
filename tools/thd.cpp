//
// Copyright (c) 2026 Paul Ranson, paul@epicyclism.com
//
//

#include <iostream>
#include <string>
#include <string_view>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <charconv>

#include <fmt/format.h>
#include <fmt/ostream.h>

//#include "mm_file.h"
#include "audio_file_reader.h"
#include "fftlib.h"
//#include "ctre.hpp"

constexpr uint32_t clp2(uint32_t v)
{
	if (v == 0)
		return 1;
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

template <typename T> void from_chars(char const* arg, T& result)
{
	auto [ptr, ec] = std::from_chars(arg, arg + strlen(arg), result);
}

void usage()
{
	fmt::println(std::cerr, "Usage: thd <inputfile> [sample_rate]");
	fmt::println(std::cerr, "       inputfile should be an audio recording of a tone.");
	fmt::println(std::cerr, "       ideally wav or flac. A 'raw' file coupled with a sample rate may also be used.");
}

constexpr size_t FFT_SZ = 32768;

int main(int ac, char** av)
{
	int32_t sample_rate = -1;

	if (ac < 2)
	{
		usage();
		return -1;
	}
	if (ac > 2)
	{
		from_chars(av[2], sample_rate);
	}
	signal_wrap<fp_t> sw(av[1]);
	auto [ptr, len] = sw.get();
	if (len == 0)
	{
		fmt::println(std::cerr, "Couldn't open <{}>", av[1]);
		return -1;
	}
	if (sample_rate == -1)
		sample_rate = sw.sample_rate_;
	if (sample_rate == -1)
		sample_rate = 96000; // default for raw file with nothing provided
	fmt::println("Audio length: {}", len);
	fmt::println("Sample rate: {}", sample_rate);
	// use an fft width greater than the sample rate. we don't need super fine resolution, just enough to get the harmonics.
	auto fft = make_fft(clp2(sample_rate), window_t::HFT248D);
//	auto fft = make_fft(FFT_SZ, window_t::HAMMING);
	fmt::println("FFT width: {}", fft->width());
#if 0
	int tm = 0;
	size_t offset = 0;
	while (offset + fft->width() < data.size())
	{
		auto [ob, oe] = (*fft) (ptr + offset, ptr + offset + fft->width());
		// estimate the second harmonic by looking for the max value in the first half of the FFT output, then looking for the max value in the second half of the FFT output.
		auto mx1 = std::max_element(ob, ob + fft->width() / 4);
		auto mx2 = std::max_element(ob + 2 * std::distance(ob, mx1) - 10, ob + fft->width() / 2);
//		fmt::println("{} {:.6f} {:.6f} {:.6f}", tm, 100.0 * *mx2 / *mx1, *mx1, *mx2);
		fmt::println("{} {:.6f}", tm, 100.0 * *mx2 / *mx1);
		offset += sample_rate;
		++tm;
	}
#else
	size_t offset = (len - fft->width()) / 2;
	// just a single effort
	auto [ob, oe] = (*fft) (ptr + offset, ptr + offset + fft->width());
	double fbinc = double(sample_rate) / fft->width();
//	double fb = -fbinc / 2.0;
	double fb = 0.0;
	auto mxe = std::max_element(ob, ob + fft->width() / 2);
	const auto bin = std::distance(ob, mxe);
	const auto fundamental = bin * fbinc;
#if 0
	fmt::println("Max value: {:.6f} at {:.6f} Hz", *mxe, fundamental);
	auto oee = ob + fft->width() / 2;
	auto obb = ob;
	while (obb < oee)
	{
		if(fb > 998 && fb < 1003)
			fmt::println("{:.6f} {:.6f}", fb, *obb);
		if(fb > 1998 && fb < 2003)
			fmt::println("{:.6f} {:.6f}", fb, *ob);
		if(fb > 2998 && fb < 3003)
			fmt::println("{:.6f} {:.6f}", fb, *obb);
		if(fb > 3998 && fb < 4003)
			fmt::println("{:.6f} {:.6f}", fb, *obb);
		fb += fbinc;
		++obb;
	}
#endif
	// compute the thd 
	int num_harmonics = (sample_rate / 2) / fundamental;
	auto f1 = ob[bin];
	auto f2 = ob[bin * 2];
	auto thd = f2;
	for (size_t h = 3; h <= num_harmonics; ++h)
	{
		const auto hbin = bin * h;
		if (hbin >= fft->width() / 2)
			break;
		thd += ob[hbin];
//		fmt::println("Harmonic {}: {:.6f} at {:.6f} Hz", h, *(ob + hbin), fbinc * hbin);
	}
	fmt::println("THD: {:.6f}%", 100.0 * thd / f1);
	fmt::println("2HD: {:.6f}%", 100.0 * f2/f1);
#endif
}