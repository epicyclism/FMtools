//
// Copyright (c) 2008-2022 Paul Ranson, paul@epicyclism.com
//
// Refer to licence in repository.
//
//
// Implement a bandpass filter
//
// Input from a complete buffer.
//

// K  - number of taps, assumed odd.
// W  - width of filter band pass (Hz)
// F  - type of input variable
//
#pragma once

#include "constants.h"

template< size_t K, size_t W, typename F > struct BandPassFilter
{
private :
	F coeff_ [ K ] ;

public :
	BandPassFilter ( size_t centre_freq, size_t sample_rate )
	{
		for ( int n = 0; n < K; ++n )
		{
			F a = F((n - int((K+1)/2)) * TWO_PI * W / sample_rate) ;
			F ys = F(1) ;
			if ( a != F(0))
				ys = sin ( a )/ a ;
			F yg = F(2) * F(4) * W / sample_rate ;
			F yw = F(0.54) - F(0.46) * cos ( F(n * TWO_PI / K )) ;
			F yf = cos (F(( n - (K+1)/2) * TWO_PI * centre_freq / sample_rate )) ;
			coeff_ [ n ] = yf * yw * yg * ys ;
		}
	}
	void Process (F const* in, size_t len, F* out )
	{
		for ( size_t off = 0; off < len - K; ++off )
		{
			F acc = F( 0 ) ;
			for ( size_t k = 0; k < K; ++k )
			{
				acc += *(in+k) * coeff_ [ k ] ;
			}
			*out = acc ;
			++in ;
			++out ;
		}
		for ( size_t off = len - K; off < len; ++off )
		{
			*out = F(0) ;
			++out; 
		}
	}
	// treat the real and imaginary parts as independent sequences, not as a complex no.
	//
	void Process (std::complex<F> const* in, size_t len, std::complex<F>* out )
	{
		for ( size_t off = 0; off < len - K; ++off )
		{
			F accr = F( 0 ) ;
			F acci = F( 0 ) ;
			for ( size_t k = 0; k < K; ++k )
			{
				accr += (in+k)->real () * coeff_ [ k ] ;
				acci += (in+k)->imag () * coeff_ [ k ] ;
			}
			*out = std::complex<F> ( accr, acci ) ;
			++in ;
			++out ;
		}
		for ( size_t off = len - K; off < len; ++off )
		{
			*out = std::complex<F>(0) ;
			++out; 
		}
	}
} ;

template< size_t K, typename F > struct BandPassFilterDynamic
{
private :
	F coeff_ [ K ] ;

public :
	BandPassFilterDynamic ( size_t centre_freq, size_t width, size_t sample_rate )
	{
		for ( int n = 0; n < K; ++n )
		{
			F a = F((n - int((K+1)/2)) * TWO_PI * width / sample_rate) ;
			F ys = F(1) ;
			if ( a != F(0))
				ys = sin ( a )/ a ;
			F yg = F(2) * F(4) * width / sample_rate ;
			F yw = F(0.54) - F(0.46) * cos ( F(n * TWO_PI / K )) ;
			F yf = cos (F(( n - (K+1)/2) * TWO_PI * centre_freq / sample_rate )) ;
			coeff_ [ n ] = yf * yw * yg * ys ;
		}
	}
	void Process (F const* in, size_t len, F* out )
	{
		for ( size_t off = 0; off < len - K; ++off )
		{
			F acc = F( 0 ) ;
			for ( size_t k = 0; k < K; ++k )
			{
				acc += *(in+k) * coeff_ [ k ] ;
			}
			*out = acc ;
			++in ;
			++out ;
		}
		for ( size_t off = len - K; off < len; ++off )
		{
			*out = F(0) ;
			++out; 
		}
	}
	// treat the real and imaginary parts as independent sequences, not as a complex no.
	//
	void Process (std::complex<F> const* in, size_t len, std::complex<F>* out )
	{
		for ( size_t off = 0; off < len - K; ++off )
		{
			F accr = F( 0 ) ;
			F acci = F( 0 ) ;
			for ( size_t k = 0; k < K; ++k )
			{
				accr += (in+k)->real () * coeff_ [ k ] ;
				acci += (in+k)->imag () * coeff_ [ k ] ;
			}
			*out = std::complex<F> ( accr, acci ) ;
			++in ;
			++out ;
		}
		for ( size_t off = len - K; off < len; ++off )
		{
			*out = std::complex<F>(0) ;
			++out; 
		}
	}
} ;