//
// Copyright (c) 2026 Paul Ranson, paul@epicyclism.com
//
//
#pragma once

#include <vector>
#include <string_view>

#include "mm_file.h"

// flags, 0 = mono, 1 = left, 2 = right
// returns pair of vector of samples and sample rate
// on error, vector is empty and sample rate contains error code.
//
std::pair<std::vector<float>, uint32_t> read_audio_file(const char* filename, uint32_t flags);
std::pair<std::vector<float>, uint32_t> read_raw_file(const char* filename, uint32_t flags);

#include <string_view>

// define flags for consistency...
//
constexpr uint32_t AUDIO_FILE_MONO = 0;
constexpr uint32_t AUDIO_FILE_LEFT = 1;
constexpr uint32_t AUDIO_FILE_RIGHT = 2;

constexpr std::string_view descflags[] = { "mono", "left", "right" };

// if an audio file, read it, if a raw data file, map it.
// return ptr/size pair
//
template<typename fp_t>
struct signal_wrap
{
	std::vector<fp_t> data_;
	int32_t sample_rate_ = -1;

	signal_wrap(const char* fn, int flags)
	{
		std::string_view  fns(fn);
		if (fns.ends_with(".wav") || fns.ends_with(".WAV") || fns.ends_with(".flac") || fns.ends_with(".FLAC"))
		{
			auto [data, sample_rate] = read_audio_file(fn, flags);
			data_ = std::move(data);
			sample_rate_ = sample_rate;
		}
		else
		{
			auto [data, status] = read_raw_file(fn, flags);
			data_ = std::move(data);
			sample_rate_ = status;
		}
	}
	std::pair<fp_t const*, size_t> get() const
	{
		return { data_.data(), data_.size() };
	}
};
