//
// Copyright (c) 2026 Paul Ranson, paul@epicyclism.com
//
//
#pragma once

#include <vector>

#include "constants.h"

// assumes values cross 0, good assumption for this application.
//
template<typename I> I find_next_opposite_sign(I b, I e)
{
    if(*b < 0)
    {
        while(b != e)
        {
            if( *b > 0)
                break;
            ++b;
        }
    }
    else
    {
        while(b != e)
        {
            if( *b < 0)
                break;
            ++b;
        }
    }
    return b;
}

F interpolate_zero_crossing(F a, F b)
{
    // linear interpolation to find the zero crossing point between a and b
    return a / (a - b);
}

// collects the intervals
template<typename I> std::vector<F> collect_zero_crossings(I b, I e)
{
    std::vector<F> result;
    F tl = 0.0;
    auto bb = b;
    while(bb != e)
    {
        auto next = find_next_opposite_sign(bb, e);
        if(next == e)
            break;
        auto tn = interpolate_zero_crossing(*(next - 1), *next) + static_cast<F>(std::distance(b, next) - 1);
        result.push_back(tn - tl);
        tl = tn;
        bb = next;
    }
    return result;
}

inline std::vector<F> collect_zero_crossings(std::vector<F> const& buf)
{
    return collect_zero_crossings(buf.begin(), buf.end());
}

// the frequency implied by a zero crossing interval, in Hz
inline F zero_crossing_frequency(F interval, int sample_rate)
{
    // a zero crossing interval is half a cycle, so the frequency is 0.5 sample_rate / interval
    return 0.5 * static_cast<F>(sample_rate) / interval;
}