//
// Copyright (c) 2026 Paul Ranson, paul@epicyclism.com
//
//

#pragma once
#include <vector>
#include <complex>

#include "constants.h"

struct fm2_proc_cfg
{
    // about
    int sample_rate = 44100;
    double rpm = 100.0/3; // 33.3333

    // how
    bool filter = false;
    int filter_width = 1000; // Hz
};

struct fm2_proc
{
    F frequency_ = 0.0;
    std::vector<F> buf_;
    std::vector<std::complex<F>> cbuf_;

    void process(fm2_proc_cfg const& cfg, std::vector<F> const& audio_buf);
};