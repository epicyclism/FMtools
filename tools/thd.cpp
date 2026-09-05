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

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "audio_file_reader.h"
#include "fftlib.h"
#include "ctre.hpp"

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

void usage()
{
	fmt::println(std::cerr, "Usage: thd <inputfile>");
	fmt::println(std::cerr, "       inputfile should be an audio recording of a tone.");
	fmt::println(std::cerr, "       ideally wav or flac. Other formats may work but are not tested.");
}

constexpr size_t nbuckets = 48000;

int main(int ac, char** av)
{
	if (ac < 2)
	{
		usage();
		return -1;
	}
	auto [data, sample_rate] = read_audio_file(av[1], 0);
	if (data.empty() || sample_rate == 0)
	{
		fmt::println(std::cerr, "Failed to read data from input file <{}>", av[1]);
		return -1;
	}
	if (data.size() < 2 * sample_rate)
	{
		fmt::println(std::cerr, "Input file <{}> is too short, must be at least 2 seconds of data", av[1]);
		return -1;
	}
	fmt::println("Audio length: {}", data.size());
	fmt::println("Sample rate: {}", sample_rate);
	fmt::println("FFT width: {}", clp2(sample_rate));
	// use an fft width greater than the sample rate. we don't need super fine resolution, just enough to get the harmonics.
	auto fft = make_fft(clp2(sample_rate), window_t::HAMMING);
	size_t offset = (data.size() - fft->width()) / 2;
	fmt::println("FFT width: {}", fft->width());
	// just a single effort
	auto [ob, oe] = (*fft) (data.data() + offset, data.data() + offset + fft->width());
	double fbinc = double(sample_rate) / fft->width();
	double fb = -fbinc / 2.0;
	auto mxe = std::max_element(ob, ob + fft->width() / 2);
	fmt::println("Max value: {:.6f} at {:.6f} Hz", *mxe, fbinc * std::distance(ob, mxe));
	auto oee = ob + fft->width() / 2;
	while (ob < oee)
	{
		if(fb > 999 && fb < 1005)
			fmt::println("{:.6f} {:.6f}", fb, *ob);
		if(fb > 1999 && fb < 2005)
			fmt::println("{:.6f} {:.6f}", fb, *ob);
		if(fb > 2999 && fb < 3009)
			fmt::println("{:.6f} {:.6f}", fb, *ob);
		if(fb > 3999 && fb < 4009)
			fmt::println("{:.6f} {:.6f}", fb, *ob);
		fb += fbinc;
		++ob;
	}
	// compute the thd 
}