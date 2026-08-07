//
// Copyright (c) 2008-2022 Paul Ranson, paul@epicyclism.com
//
// Refer to licence in repository.
//

#include <complex>
#include <algorithm>
#include <numeric>
#include <functional>
#include <array>
#include <cmath>
#if defined(__clang__)
#include <boost/math/special_functions/bessel.hpp>
#endif
#include <numbers>
#include <bit>

#include "fftlib.h"

#include "FFT.h"
#include "ProcFFT.h"

using namespace std::literals;

window_t wt_from_code(char t)
{
	switch (t)
	{
	default:
	case '0':
		return window_t::NOWINDOW;
	case '1':
		return window_t::HAMMING;
	case '2':
		return window_t::BLACKMAN;
	case '3':
		return window_t::BLACKMANHARRIS;
	case '4':
		return window_t::KAISER5;
	case '5':
		return window_t::KAISER7;
	}
}

window_t wt_from_string(std::string_view t)
{
	if (t == "HAMMING"sv)
		return window_t::HAMMING;
	if (t == "BMACKMAN"sv)
		return window_t::BLACKMAN;
	if (t == "BLACKMANHARRIS"sv)
		return window_t::BLACKMANHARRIS;
	if (t == "HAISER5"sv)
		return window_t::KAISER5;
	if (t == "KAISER7"sv)
		return window_t::KAISER7;
	return window_t::NOWINDOW;
}

std::string_view wt_to_string(window_t wt)
{
	switch (wt)
	{
	case window_t::NOWINDOW:
		return "No Window"sv;
	case window_t::HAMMING:
		return "Hamming"sv;
	case window_t::BLACKMAN:
		return "Blackman"sv;
	case window_t::BLACKMANHARRIS:
		return "Blackman-Harris"sv;
	case window_t::KAISER5:
		return "Kaiser5"sv;
	case window_t::KAISER7:
		return "Kaiser7"sv;
	default:
		return "Unknown window type"sv;
	}
}

std::unique_ptr<IProcessorFFT> make_fft(size_t width, window_t wt)
{
	switch (width)
	{
	case  8:
		return std::unique_ptr<IProcessorFFT>( new ProcessorFFT<fp_t, 256>(wt));
	case  9:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 512>(wt));
	case 10:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 1024>(wt));
	case 11:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 2048>(wt));
	case 12:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 4096>(wt));
	case 13:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 8192>(wt));
	case 14:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 16384>(wt));
	case 15:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 32768>(wt));
	case 16:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 65536>(wt));
	case 17:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 131072>(wt));
	case 18:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 262144>(wt));
	case 19:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 524288>(wt));
	case 20:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 1048576>(wt));
	case 21:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 2097152>(wt));
	case 22:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 4194304>(wt));
	case 23:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 8388608>(wt));
	case 24:
		return std::unique_ptr<IProcessorFFT>(new ProcessorFFT<fp_t, 16777216>(wt));
	}
	return std::unique_ptr<IProcessorFFT>();
}

template <typename F> class angle_generator
{
private:
	size_t ind_;
	size_t interval_;
	F      frequency_;
	F      time_;
	F      tinc_;

public:
	angle_generator(F frequency, size_t sample_rate) : ind_(0), frequency_(frequency), time_(0)
	{
		interval_ = 40 * sample_rate / static_cast<size_t> (frequency_);
		tinc_ = 1.0 / sample_rate;
	}
	F operator ()()
	{
		F ret = static_cast<F>(std::numbers::pi_v<fp_t> * 2.0 * frequency_ * time_);
		time_ += tinc_;
		++ind_;
		if (ind_ == interval_)
		{
			ind_ = 0;
			fp_t f = ::floor(time_);
			time_ -= f;
		}
		return ret;
	}
};

// f = frequency in Hz
// sample_rate = sample rate in Hz, 44100, 96000 etc.
//
void fill_buffer_with_sine(fp_t f, fp_t* buf_b, fp_t* buf_e, size_t sample_rate, fp_t amplitude)
{
	angle_generator<fp_t> ag(f, sample_rate);

	std::generate(buf_b, buf_e, [&ag, amplitude]() { return sin(ag()) * amplitude; });
}

// fm = modulation frequency
// dev = modulation depth, deviation.
// sample_rate = sample rate in Hz, 44100, 96000 etc.
//
void fill_buffer_with_FM(fp_t fc, fp_t fm, fp_t dev, fp_t* buf_b, fp_t* buf_e, size_t sample_rate)
{
	angle_generator<fp_t> ag_carrier(fc, sample_rate);
	angle_generator<fp_t> ag_mod    (fm, sample_rate);

	fp_t fd = dev * fc / fm;
	while(buf_b != buf_e)
	{
		// FM Equation is y(t) = A*sin(2Pi*fc*t + I*sin(2Pi*fm*t))
		// where A is amplitude, for us 1 and I is deviation.
		// fc is carrier frequency and fm is modulation frequency
		//
		*buf_b = fp_t(0.5) * sin(ag_carrier() + fd * sin(ag_mod()));
		++buf_b;
	}
}
