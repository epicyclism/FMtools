//
// Copyright (c) 2008-2022 Paul Ranson, paul@epicyclism.com
//
// Refer to licence in repository.
//

template <typename T, size_t FFTSZ>
void ProcessorFFT<T, FFTSZ>::PrepareFFT ()
{
	std::copy(wsp1_.begin(), wsp1_.end(), fftin_.begin());
}

template <typename T, size_t FFTSZ>
void ProcessorFFT<T, FFTSZ>::PostFFT ()
{
	// taking the magnitude of each FFT output point
	std::transform ( fftout_, fftout_ + FFTSZ / 2, wsp2_, std::norm<T> ) ;
}

template <typename T, size_t FFTSZ>
ProcessorFFT<T, FFTSZ>::ProcessorFFT ( window_t wt ) : window_ ( wt )
{
	static_assert(std::popcount(FFTSZ) == 1, "FFTSZ must be a power of 2.");
}

template <typename T, size_t FFTSZ>
ProcessorFFT<T, FFTSZ>::~ProcessorFFT ()
{
}

template <typename T, size_t FFTSZ>
std::pair<T const*, T const*> ProcessorFFT<T, FFTSZ>::operator () ( T const* ib, T const* ie )
{
	window_ ( ib, ie, wsp1_.begin() ) ;
	PrepareFFT () ;
	fft_ ( fftin_.data(), fftout_.data());

	// taking the magnitude of each FFT output point
	std::transform(fftout_.begin(), fftout_.begin() + FFTSZ / 2, wsp1_.begin(), [factor = window_.Gain()](auto t) { return std::abs<T>(t) * T { 2.0 } * factor / FFTSZ; });
	
	return std::make_pair(wsp1_.data(), wsp1_.data() + FFTSZ / 2);
}
