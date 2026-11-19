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
    int sample_rate_ = 44100;
    double rpm_ = 100.0/3; // 33.3333

    // how
    bool filter_ = false;
    int filter_width_ = 1000; // Hz
    bool polar_ = false;
};

struct fm2_proc
{
    F frequency_ = 0.0;
    std::vector<F> buf_;
    std::vector<std::complex<F>> iqbuf_;
    std::vector<std::complex<F>> pbuf_;
    bool sufficient_data(std::vector<F> const& audio_buf);
    void process(fm2_proc_cfg const& cfg, std::vector<F> const& audio_buf);
};

// mmm
template <typename T> double mean ( T begin, T end )
{
	size_t m = 0 ;
	size_t n = 0 ;
	double mn = 0.0 ;
	double partmn = 0.0 ;

	while( begin != end )
	{
		++n ;
		partmn += *begin ;
		if ( n == 5000 )
		{
			mn += partmn / n ;
			partmn = 0.0 ;
			n = 0 ;
			++m ;
		}
		++begin ;
	}
	// not quite right since the last part doesn't contribute equally...
	mn += partmn / n ;
	mn /= m + 1 ;

	return mn ;
}
