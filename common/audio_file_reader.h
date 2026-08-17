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
