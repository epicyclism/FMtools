//
// Copyright (c) 2008-2022 Paul Ranson, paul@epicyclism.com
//
// Refer to licence in repository.
//

#pragma once

template <typename T, size_t FFTSZ > class Window
{
private :
	// coeffs.
	std::array<T, FFTSZ> coeff_table_;
	T gain_ ;
public :
	Window (window_t wt = window_t::HAMMING) ;
	template<typename II, typename OI> void operator () ( II samples_b, II samples_e, OI out_b) const ;
	T Gain () const ;
} ;

template < typename T, size_t FFTSZ, int Invert = 1> class FFT
{
private :
	// 'static'
//	int lgN_ ;
	const T   div_ ;
	std::array<std::complex<T>, FFTSZ / 2>  w_ ;

	// working variables.
	std::array<std::complex<T>, FFTSZ>  buf_ ;

public :
	FFT () ;
	void operator () ( std::complex<T> * in, std::complex<T> * out ) ;
} ;

// implementation
#include "FFTImpl.h"