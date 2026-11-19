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

bool fm2_proc::sufficient_data(std::vector<F> const& audio_buf)
{
    return audio_buf.size() > 1536 + 2048 + 1024 + 2048; // numbers derivived from the demodulation process
}

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
    frequency_ = zero_crossing_frequency(average_interval, cfg.sample_rate_);
    buf_.resize(audio_buf.size());
    std::copy(audio_buf.begin(), audio_buf.end(), buf_.begin());
    if(cfg.filter_)
    {
        DoBandFilter(buf_.data(), buf_.size(), static_cast<size_t>(frequency_), cfg.sample_rate_);
    }
    iqbuf_.resize ( buf_.size ()) ;
	GenerateIQ ( buf_.data(), iqbuf_.data(), iqbuf_.size(), cfg.sample_rate_ );
	EqualiseIQ ( iqbuf_.data(), iqbuf_.size() );
	// IQ derivative buffer
	std::vector<std::complex<F> > iqdiffbuf ;
	iqdiffbuf.resize ( buf_.size ()) ;
	Differentiate3 ( iqbuf_.data(), iqdiffbuf.data(), iqdiffbuf.size());
	// demodulate
	DemodulateFinal ( iqbuf_.data(), iqdiffbuf.data(), buf_.data(), buf_.size () ) ;
	// trim off the end to account for unprocessed points at the end of filter operations
	buf_.resize ( buf_.size () - 2048 - 1024 - 2048 ) ;
	// trim off the beginning
	buf_.erase ( buf_.begin (),  buf_.begin () + 1536 ) ;

    F fs = F ( cfg.sample_rate_ ) ;
	std::for_each ( buf_.begin (), buf_.end (), [fs]( F& rf )
	{
		rf = fs * rf / TWO_PI;
	}) ;
	// optional polar output
	if ( cfg.polar_ )
	{
        F angle_inc = TWO_PI * (cfg.rpm_ / 60.0) / fs ;
        F angle = 0.0 ;
        pbuf_.resize ( buf_.size () ) ;
        std::transform( buf_.begin(), buf_.end(), pbuf_.begin(), [angle_inc, &angle](F v)
        {
            angle += angle_inc ;
            return std::complex<F>(angle, v);
        }) ;
    }
}