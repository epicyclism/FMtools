//
// Copyright (c) 2026 Paul Ranson, paul@epicyclism.com
//
//

#include <algorithm>
#include <numeric>
#include <vector>

#include "FMDemodFunctions.h"
#include "zero_crossing.h"
#include "fm2_proc.h"

void fm2_proc::process(fm2_proc_cfg const& cfg, std::vector<F> const& audio_buf)
{
    // zero crossing detection
    auto zero_crossings = collect_zero_crossings(audio_buf);
    auto average_interval = std::accumulate(zero_crossings.begin(), zero_crossings.end(), 0.0) / zero_crossings.size();
    auto low_interval = average_interval * 0.9;
    auto high_interval = average_interval * 1.1;
    size_t count = 0;
    average_interval = std::accumulate(zero_crossings.begin(), zero_crossings.end(), 0.0, [low_interval, high_interval, &count](double acc, double val) {
        if (val >= low_interval && val <= high_interval) {
            ++count;
            return acc + val;
        }
        return acc;
    }) / count ;
    frequency_ = zero_crossing_frequency(average_interval, cfg.sample_rate);
    buf_.resize(audio_buf.size());
    std::copy(audio_buf.begin(), audio_buf.end(), buf_.begin());
    if(cfg.filter)
    {
        DoBandFilter(buf_.data(), buf_.size(), static_cast<size_t>(frequency_), cfg.sample_rate);
    }
}