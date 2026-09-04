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

#include "ctre.hpp"

void usage()
{
	fmt::println(std::cerr, "Usage: thd <inputfile>");
	fmt::println(std::cerr, "       inputfile should be a text file containing a list of frequencies,");
	fmt::println(std::cerr, "       one per line, followed by a level in dB.");
}

constexpr size_t nbuckets = 48000;

std::vector<float> read_input_file(const std::string& filename)
{
	std::vector<float> levels(nbuckets, 0.0f);
	std::ifstream infile(filename);
	if (!infile)
		return levels;

	std::string line;
	while (std::getline(infile, line))
	{
		auto [m, f, l] = ctre::match<"([0-9\\.]+)\\s+(\\-?[0-9\\.]+)">(line);
		if (m)
		{
			auto fr  = f.to_number<float>();
			auto lvl = l.to_number<float>();
			size_t fri = static_cast<size_t>(fr);
			if (fri < nbuckets)
			{
				levels[fri] += std::exp(lvl/10.0);
			}
		}
	}
	std::ranges::transform(levels, levels.begin(), [](float x) { return 10.0f * std::log10(x); });
	return levels;
}

int main(int ac, char** av)
{
	if (ac < 2)
	{
		usage();
		return -1;
	}
	auto levels = read_input_file(av[1]);
	if (levels.empty())
	{
		fmt::println(std::cerr, "Failed to open input file <{}>", av[1]);
		return -1;
	}
	auto mxe = std::ranges::max_element(levels);
	auto fundamental = std::distance(levels.begin(), mxe);
	fmt::println("fundamental  = {}Hz, at {}", fundamental, *mxe);
	fmt::println("2nd harmonic = {}Hz, at {}", fundamental * 2, levels[fundamental * 2]);
	auto thd = std::accumulate(levels.begin() + fundamental * 3 / 2, levels.end(), 0.0f, [](float a, float b) { return a + std::pow(10.0f, b / 10.0f); });
	fmt::println("THD = {}dB", 10.0f * std::log10(thd));
}