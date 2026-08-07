//
// Copyright (c) 2008-2022 Paul Ranson, paul@epicyclism.com
//
// Refer to licence in repository.
//
//
//	FMDemodFunctions.h
//
//	Utility functions for FM demodulation....
//

#pragma once

#include <complex>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/normal_distribution.hpp>

#include "constants.h"
#include "HilbertTransformer.h"
#include "BandpassFilter.h"

template <typename F> class AngleGenerator
{
private :
	size_t ind_ ;
	size_t interval_ ;
	F      frequency_ ;
	F      time_ ;
	F      tinc_ ;

public :
	AngleGenerator ( F frequency, size_t sample_rate ) : ind_ ( 0 ), frequency_ ( frequency ), time_ ( 0 )
	{
		interval_ = 40 * sample_rate / static_cast<size_t> ( frequency_ ) ;
		tinc_     = 1.0 / sample_rate ;
	}
	F operator ()()
	{
		F ret = static_cast<F>( TWO_PI* frequency_ * time_ ) ;
		time_ += tinc_ ;
		++ind_ ;
		if ( ind_ == interval_ )
		{
			ind_ = 0 ;
			F f = floor ( time_ ) ;
			time_ -= f ;
		}
		return ret ;
	}
	F operator ()( F wobble )
	{
		F ret = static_cast<F>(TWO_PI * frequency_ * (time_ + wobble )) ;
		time_ += tinc_ ;
		++ind_ ;
		if ( ind_ == interval_ )
		{
			ind_ = 0 ;
			F f = floor ( time_ ) ;
			time_ -= f ;
		}
		return ret ;
	}
} ;

template <typename T> double RMS ( T begin, T end )
{
	size_t m = 0 ;
	size_t n = 0 ;
	double rms = 0.0 ;
	double partrms = 0.0 ;

	while( begin != end )
	{
		++n ;
		partrms += *begin * *begin ;
		if ( n == 8192 )
		{
			rms += partrms / n ;
			partrms = 0.0 ;
			n = 0 ;
			++m ;
		}
		++begin ;
	}
	// not quite right since the last part doesn't contribute equally...
	rms += partrms / n ;
	rms /= m + 1 ;

	return sqrt ( rms ) ;
}

// fix this repugnant code repetition!
//
template <typename T> double RMS_R ( T begin, T end )
{
	size_t m = 0 ;
	size_t n = 0 ;
	double rms = 0.0 ;
	double partrms = 0.0 ;

	while( begin != end )
	{
		++n ;
		partrms += begin->real () * begin->real () ;
		if ( n == 8192 )
		{
			rms += partrms / n ;
			partrms = 0.0 ;
			n = 0 ;
			++m ;
		}
		++begin ;
	}
	// not quite right since the last part doesn't contribute equally...
	rms += partrms / n ;
	rms /= m + 1 ;

	return sqrt ( rms ) ;
}

template <typename T> double RMS_I ( T begin, T end )
{
	size_t m = 0 ;
	size_t n = 0 ;
	double rms = 0.0 ;
	double partrms = 0.0 ;

	while( begin != end )
	{
		++n ;
		partrms += begin->imag () * begin->imag () ;
		if ( n == 8192 )
		{
			rms += partrms / n ;
			partrms = 0.0 ;
			n = 0 ;
			++m ;
		}
		++begin ;
	}
	// not quite right since the last part doesn't contribute equally...
	rms += partrms / n ;
	rms /= m + 1 ;

	return sqrt ( rms ) ;
}

// f = frequency in Hz
// sample_rate = sample rate in Hz, 44100, 96000 etc.
//
template <typename F> void FillBufferWithSine ( F f, F* buf, size_t len, size_t sample_rate, F amplitude = F(0.5) )
{
	AngleGenerator<F> ag ( f, sample_rate ) ;

	for (size_t c = 0; c < len; ++c )
	{
		*buf = amplitude * sin ( ag ()) ;
		++buf ;
	}
}

// f = frequency in Hz
// sample_rate = sample rate in Hz, 44100, 96000 etc.
// len is stereo samples.
//
template <typename F> void FillBufferWithIQ ( F f, F* buf, size_t len, size_t sample_rate, F amplitude = F(0.5) )
{
	AngleGenerator<F> ag ( f, sample_rate ) ;

	for (size_t c = 0; c < len; ++c )
	{
		F angle = ag () ;
		*buf = amplitude * sin ( angle ) ;
		++buf ;
		*buf = amplitude * cos ( angle ) ;
		++buf ;
	}
}

// fc = carrier frequency
// fm = modulation frequency
// dev = modulation depth, deviation.
// sample_rate = sample rate in Hz, 44100, 96000 etc.
//
template <typename F> void FillBufferWithFM ( F fc, F fm, F dev, F * buf, size_t len, size_t sample_rate )
{
	F tinc = F(1.0)/sample_rate ;
	F t = 0.0 ;

	F fd = dev * fc / fm ;
	for (size_t c = 0; c < len; ++c )
	{
		// FM Equation is y(t) = A*sin(2Pi*fc*t + I*sin(2Pi*fm*t))
		// where A is amplitude, for us 1 and I is deviation.
		// fc is carrier frequency and fm is modulation frequency
		//
		*buf = F(0.5) * sin ( static_cast<F>(TWO_PI * fc * t ) + fd * sin ( static_cast<F>(TWO_PI * fm * t ))) ;
		++buf ;
		t += tinc ;
	}
}

// Produce a sine with sinusoidal jitter.
// fc sine freq
// jd jitter depth
// sample_rate = ...
// amplitude = ...
//
template <typename F> void JitterBufSine ( F fc, F fj, F jd, F* buf, size_t len, size_t sample_rate, F amplitude = F(0.5) )
{
	// calculate the actual desired jitter depth from the frequency and requested side band size.
	F d = jd ;
	d -= 20 * log10 ( amplitude ) ; // compensate for our amplitude at 0.5.
	d /= 20 ;
	d = pow ( 10.0,  d ) ;
	d *= F(4) ;
	d /= fc ;

	AngleGenerator<F> agC  ( fc, sample_rate ) ;
	AngleGenerator<F> agJP ( fc + fj, sample_rate ) ;
	AngleGenerator<F> agJM ( fc - fj, sample_rate ) ;

	for (size_t c = 0; c < len; ++c )
	{
		*buf = amplitude * ( cos ( agC ()))
			- F(0.25) * d * TWO_PI * fc * cos ( agJM ())
			+ F(0.25) * d * TWO_PI * fc * cos ( agJP ()) ;
		++buf ;
	}
}

template <typename F> void JitterBufSineOld ( F fc, F fj, F jd, F* buf, size_t len, size_t sample_rate, F amplitude = F(0.5) )
{
	// calculate the actual desired jitter depth from the frequency and requested side band size.
	F d = jd ;
	d -= 20 * log10 ( amplitude ) ; // compensate for our amplitude at 0.5.
	d /= 20 ;
	d = pow ( 10.0,  d ) ;
	d *= F(4) ;
	d /= fc ;

	double tinc = 1.0/sample_rate ;
	double t = 0.0 ;

	for (size_t c = 0; c < len; ++c )
	{
		*buf = amplitude * ( cos ( TWO_PI * fc * t )
			- F(0.25) * d * TWO_PI * fc * cos ( TWO_PI * (fc - fj) * t )
			+ F(0.25) * d * TWO_PI * fc * cos ( TWO_PI * (fc + fj) * t )) ;
		++buf ;
		t += tinc ;
	}
}

// now 'jd' is the maximum offset at each sample in ps.
//
template <typename F> void JitterBufSineDirect ( F fc, F fj, F jd, F* buf, size_t len, size_t sample_rate, F amplitude = F(0.1) )
{
	F d = jd * 1.0E-12 ;

	AngleGenerator<F> agC ( fc, sample_rate ) ;
	AngleGenerator<F> agJ ( fj, sample_rate ) ;

	for (size_t c = 0; c < len; ++c )
	{
		*buf = amplitude * ( cos ( agC ( cos ( agJ ()) * d ))) ;
		++buf ;
	}
}

// Produce a sine with random jitter.
// fc sine freq
// jd jitter depth
// sample_rate = ...
// amplitude = ...
//
template <typename F> void JitterBufRandom ( F fc, F jd, F* buf, size_t len, size_t sample_rate, F amplitude = F(0.5) )
{
	AngleGenerator<F> ag ( fc, sample_rate ) ;
	double d = jd * 1.0E-12 ;
	// randomness
	boost::random::mt19937 gen ;
	boost::random::uniform_real_distribution<> dist ( -d, d ) ;

	for (size_t c = 0; c < len; ++c )
	{
		*buf = amplitude * ( cos ( ag ( dist ( gen )))) ;
		++buf ;
	}
}

template <typename F> void JitterBufRandomOld ( F fc, F jd, F* buf, size_t len, size_t sample_rate, F amplitude = F(0.5) )
{
	double tinc = 1.0/sample_rate ;
	double t = 0.0 ;
	double d = jd * 1.0E-12 ;
	// randomness
	boost::random::mt19937 gen ;
	boost::random::uniform_real_distribution<> dist ( -d, d ) ;

	for (size_t c = 0; c < len; ++c )
	{
		*buf = amplitude * ( cos ( TWO_PI * fc * ( t + dist ( gen )))) ;
		++buf ;
		t += tinc ;
	}
}

// Produce a sine with random jitter.
// fc sine freq
// jd jitter depth
// sample_rate = ...
// amplitude = ...
//
template <typename F> void JitterBufGaussian ( F fc, F jd, F* buf, size_t len, size_t sample_rate, F amplitude = F(0.5) )
{
	AngleGenerator<F> ag ( fc, sample_rate ) ;
	double d = jd * 1.0E-12 ;
	// randomness
	boost::random::mt19937 gen ;
	boost::random::normal_distribution<> dist ( F(0.0), d ) ;

	for (size_t c = 0; c < len; ++c )
	{
		*buf = amplitude * ( cos ( ag ( dist ( gen )))) ;
		++buf ;
	}
}

template <typename F> void JitterBufGaussianOld ( F fc, F jd, F* buf, size_t len, size_t sample_rate, F amplitude = F(0.5) )
{
	double tinc = 1.0/sample_rate ;
	double t = 0.0 ;
	double d = jd * 1.0E-12 ;
	// randomness
	boost::random::mt19937 gen ;
	boost::random::normal_distribution<> dist ( F(0.0), d ) ;

	for (size_t c = 0; c < len; ++c )
	{
		*buf = amplitude * ( cos ( TWO_PI * fc * ( t + dist ( gen )))) ;
		++buf ;
		t += tinc ;
	}
}

// three sine waves simultaneously!
//
template <typename F> void FillBufferWithTriSine(F f1, F f2, F f3, F* buf, size_t len, size_t sample_rate, F amplitude = F(0.5))
{
	AngleGenerator<F> ag1(f1, sample_rate);
	AngleGenerator<F> ag2(f2, sample_rate);
	AngleGenerator<F> ag3(f3, sample_rate);
	for (size_t c = 0; c < len; ++c)
	{
		*buf = amplitude * (sin(ag1()) + sin(ag2() + TWO_PI/F(4)) + sin(ag3())) / F(3);
		++buf;
	}
}

template <typename F> void DoBandFilter ( F* in, size_t len, size_t centre_freq, size_t sample_rate )
{
//	BandPassFilter<2047, 40, F> bp ( centre_freq, sample_rate ) ;
	BandPassFilter<4095, 200, F> bp ( centre_freq, sample_rate ) ;
	bp.Process ( in, len, in ) ;
}

template <typename F> void DoBandFilter ( std::complex<F>* in, size_t len, size_t centre_freq, size_t sample_rate )
{
	BandPassFilter<2047, 40, F> bp ( centre_freq, sample_rate ) ;
	bp.Process ( in, len, in ) ;
}

template <typename F> void DoBandFilterWide ( F* in, size_t len, size_t centre_freq, size_t sample_rate )
{
	BandPassFilter<2047, 5000, F> bp ( centre_freq, sample_rate ) ;
	bp.Process ( in, len, in ) ;
}

template <typename F> void DoLowPassFilter ( F* in, size_t len, size_t sample_rate )
{
//	BandPassFilter<2047, 200, F> bp ( 0, sample_rate ) ;
	BandPassFilter<2047, 40, F> bp ( 0, sample_rate ) ;
	bp.Process ( in, len, in ) ;
}

template <typename F> void DoLowPassFilter ( F* in, size_t len, size_t width, size_t sample_rate )
{
	BandPassFilterDynamic<2047, F> bp ( 0, width, sample_rate ) ;
	bp.Process ( in, len, in ) ;
}

// just hiding the specific Hilbert Transformer from view.
// could be done in other ways.
// pointer interface to be tweaked sometime.
//
template <typename F> void GenerateIQ ( F* in, std::complex<F>* out, size_t len, size_t sample_rate )
{
	HilbertTransformer< 1023, F> ht ( sample_rate ) ;
	ht.Process ( in, len, out ) ;
}

// Effectively three point FIR differentiator
// 0 at begin and end. So x and x' are aligned.
//
template <typename F> void Differentiate ( std::complex<F>* in, std::complex<F>* out, size_t len )
{
	std::complex<F>* prev = in ;
	std::complex<F>* next = in + 2 ;
	++in ;
	*out = std::complex<F> ( F( 0 ), F( 0 )) ;
	++out ;
	for ( size_t c = 1; c < len - 1; ++c )
	{
		*out = std::complex <F>( next->real () - prev->real () ,  next->imag () - prev->imag () ) ;
		++prev ;
		++next ;
		++in ;
		++out ;
	}
	*out = std::complex<F> ( F( 0 ), F( 0 )) ;
}

template <typename F> void Differentiate ( F* in, F* out, size_t len )
{
	F* prev = in ;
	F* next = in + 2 ;
	++in ;
	*out = F( 0 ) ;
	++out ;
	for ( size_t c = 1; c < len - 1; ++c )
	{
		*out = (*next - *prev) / F(2) ;
		++prev ;
		++next ;
		++in ;
		++out ;
	}
	*out = F( 0 ) ;
}

template <typename F> void Differentiate3 ( F* in, F* out, size_t len )
{
	F* n__3 = in ;
	F* n__1 = in + 2 ;
	F* n_1  = in + 4 ;
	F* n_3  = in + 6 ;

	*out = F( 0 ) ;
	++out ;
	*out = F( 0 ) ;
	++out ;
	*out = F( 0 ) ;
	++out ;

	for ( size_t c = 3; c < len - 2; ++c )
	{
		*out = (*n__3 - *n_3) / 16.0 + *n_1 - *n__1 ;
		++n__3 ;
		++n__1 ;
		++n_3 ;
		++n_1 ;
		++out ;
	}
	*out = F( 0 ) ;
	++out ;
	*out = F( 0 ) ;
}

// look at the real and imaginary series, compute gain and offset to normalise them as a pair.
// assume that the input is sensible.
//
template <typename F> void EqualiseIQ ( std::complex<F> * buf, size_t len )
{
	// first log the top and bottom limits for each series.
	F dc_r = F(0), dc_i = F(0) ;
	std::complex<F>* pb = buf ;
	for ( size_t n = 0; n < len; ++n )
	{
		dc_r += pb->real () ;
		dc_i += pb->imag () ;
		++pb ;
	}
	dc_r /= len ;
	dc_i /= len ;
	F rms_r = RMS_R ( buf, buf + len ) ;
	F rms_i = RMS_I ( buf, buf + len ) ;
//	CERR << TEXT("Equalise IQ : ") << rms_r << TEXT(", ") << rms_i  << TEXT(", ") << dc_r << TEXT(", ") << dc_i  << std::endl ;
	F mul ;
	if ( rms_r > rms_i )
	{
		// inc i
		mul = rms_r / rms_i ;
		pb = buf ;
		for ( size_t n = 0; n < len; ++n )
		{
			*pb = std::complex<F>( pb->real () - dc_r, pb->imag () * mul - dc_i) ;
			++pb ;
		}
	}
	else
	{
		// inc r
		mul = rms_i / rms_r ;
		pb = buf ;
		for ( size_t n = 0; n < len; ++n )
		{
			*pb = std::complex<F>( pb->real () * mul - dc_r, pb->imag () - dc_i ) ;
			++pb ;
		}
	}
	pb = buf ;
	dc_r = F(0), dc_i = F(0) ;
	for ( size_t n = 0; n < len; ++n )
	{
		dc_r += pb->real () ;
		dc_i += pb->imag () ;
		++pb ;
	}
	dc_r /= len ;
	dc_i /= len ;
	rms_r = RMS_R ( buf, buf + len ) ;
	rms_i = RMS_I ( buf, buf + len ) ;
//	CERR << TEXT("Equalise IQ : ") << rms_r << TEXT(", ") << rms_i << TEXT(", ") << dc_r << TEXT(", ") << dc_i  << std::endl ;
}

// The demodulation function
// Produces an output that is proportional to sample by sample frequency.
// needs further manipulation to turn into instant f
// multiply by Fs/2Pi to get actual frequency.
//
template <typename F> void DemodulateFinal ( std::complex<F>* in, std::complex<F>* indiff, F* out, size_t len )
{
	for ( size_t c = 1; c < len; ++c )
	{
		F n = std::norm ( *in ) ;
		if ( n == F(0))
			*out = F(0) ;
		else
			*out = ( in->real () * indiff->imag () - in->imag () * indiff->real ()) / n ;
		++in ;
		++indiff ;
		++out ;
	}
}
