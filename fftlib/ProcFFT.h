//
// Copyright (c) 2008-2022 Paul Ranson, paul@epicyclism.com
//
// Refer to licence in repository.
//

#pragma once

#include "FFT.h"

template <typename T, size_t FFTSZ> class ProcessorFFT : public IProcessorFFT
{
private :
	// working spaces
	std::array<T, FFTSZ>  wsp1_ ;
	std::array<T, FFTSZ>  wsp2_ ;
	std::array<std::complex<T>, FFTSZ> fftin_ ;
	std::array<std::complex<T>, FFTSZ> fftout_ ;

	// processor objects
	Window<T, FFTSZ> window_ ;
	FFT<T, FFTSZ>    fft_ ;

	// helper fns
	void PrepareFFT () ;
	void PostFFT () ;

public :
	ProcessorFFT ( window_t wt = window_t::HAMMING ) ;
	virtual ~ProcessorFFT ();
	virtual std::pair<T const*, T const*> operator () ( T const* ib, T const* ie );
	virtual size_t width () final { return FFTSZ ; } 
} ;

#include "ProcFFTImpl.h"