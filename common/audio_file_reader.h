//
// Copyright (c) 2026 Paul Ranson, paul@epicyclism.com
//
//
#pragma once

#include <vector>

// flags, 0 = mono, 1 = left, 2 = right
// returns pair of vector of samples and sample rate
// on error, vector is empty and sample rate contains error code.
//
std::pair<std::vector<float>, uint32_t> read_audio_file(const char* filename, uint32_t flags);

#include <string_view>
#include "mm_file.h"

// if an audio file, read it, if a raw data file, map it.
// return ptr/size pair
//
template<typename fp_t>
struct signal_wrap
{
	mem_map_file<fp_t> mmf_;
	std::vector<fp_t> data_;
	int32_t sample_rate_ = -1;

	signal_wrap(const char* fn)
	{
		std::string_view  fns(fn);
		if (fns.ends_with(".wav") || fns.ends_with(".WAV") || fns.ends_with(".flac") || fns.ends_with(".FLAC"))
		{
			auto [data, sample_rate] = read_audio_file(fn, 0);
			data_ = std::move(data);
			sample_rate_ = sample_rate;
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
