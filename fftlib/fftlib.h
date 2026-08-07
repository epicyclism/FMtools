//
// Copyright (c) 2008-2022 Paul Ranson, paul@epicyclism.com
//
// Refer to licence in repository.
//

#pragma once

#include <complex>
#include <utility>
#include <string_view>
#include <memory>

using fp_t = double;

struct IProcessorFFT
{
public:
	virtual ~IProcessorFFT() {};
	virtual std::pair<fp_t const*, fp_t const*> operator () (fp_t const* ib, fp_t const* ie) = 0;
	virtual size_t width() = 0;
};


enum class window_t { NOWINDOW, HAMMING, BLACKMAN, BLACKMANHARRIS, KAISER5, KAISER7 };

window_t wt_from_code(char t);
window_t wt_from_string(std::string_view t);
std::string_view wt_to_string(window_t wt);

const size_t FFTWdMin = 8;
const size_t FFTWdMax = 24;

// creates an FFT processor with the specified width and using the specified windowint function.
// width is the power of 2 of the FFTSZ, to avoid complications.
// currently  between FFTWdMin and FFTWinMax, inclusive.
//
std::unique_ptr<IProcessorFFT> make_fft(size_t width, window_t wt);

// f = frequency in Hz
// sample_rate = sample rate in Hz, 44100, 96000 etc.
//
void fill_buffer_with_sine(fp_t f, fp_t* buf_b, fp_t* buf_e, size_t sample_rate, fp_t amplitude = fp_t(0.5));

// fm = modulation frequency
// dev = modulation depth, deviation.
// sample_rate = sample rate in Hz, 44100, 96000 etc.
//
void fill_buffer_with_FM(fp_t fc, fp_t fm, fp_t dev, fp_t* buf_b, fp_t* buf_e, size_t sample_rate);