#include <vector>

#include <fmt/format.h>

#include "FMDemodFunctions.h"
#include "fftlib.h"
#include "constants.h"

using F = fp_t;

void usage ()
{
	fmt::println("DSPTest") ;
}

void FFTTest ()
{
	std::vector<F> buf ;

	for ( int w = 8; w < 20; ++w )
	{
		auto pFFT = make_fft(w, window_t::HAMMING) ;
		buf.resize ( pFFT->width ()) ;
		fmt::println("FFT width = {}", pFFT->width ()) ;
//		for ( int f = 120; f < 136; ++f )
		for ( int f = 1; f < 8193; f *= 2 )
		{
			FillBufferWithSine ( F(f + 1), &buf [ 0 ], buf.size (), 32768 ) ;
			auto[b, e] = pFFT->operator()(buf.data(), buf.data() + buf.size()) ;
			auto mit = std::max_element ( b, e) ;

			fmt::println("f = {}, max = {}, at bucket {}, freq {}", f, *mit, std::distance ( b, mit ), F(F(32768) * std::distance ( b, mit ) / pFFT->width ())) ;
		}
	}
}

template< int sz> void DoDiffTest ( F* buf, F* out, int f, F amp )
{
	FillBufferWithSine ( F(f), buf, sz, sz, amp ) ;
	Differentiate ( buf, out, sz ) ;
	auto mit = std::max_element ( out, out + sz) ;
	fmt::println("f={}, amp={}, md={}", f, amp, *mit * f) ;
	Differentiate3 ( buf, out, sz ) ;
	mit = std::max_element ( out, out + sz) ;
	fmt::println(", d3 md={}", *mit / f) ;
}

void DifferentiatorTest ()
{
	std::vector<F> buf ;
	std::vector<F> out ;

	buf.resize ( 256 ) ;
	out.resize ( 256 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],   2, 0.2 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],   4, 0.2 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],   8, 0.2 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],  16, 0.2 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],  32, 0.2 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],  64, 0.2 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ], 127, 0.2 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],   2, 0.8 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],   4, 0.8 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],   8, 0.8 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],  16, 0.8 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],  32, 0.8 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ],  64, 0.8 ) ;
	DoDiffTest<256> ( &buf [ 0 ], &out [ 0 ], 127, 0.8 ) ;
}

void PrecTest ()
{
	float f ;
	double d ;
	long double dd ;

	f  = cos ( static_cast<float>(PI_FOUR)) ;
	d  = cos ( static_cast<double>(PI_FOUR)) ;
	dd = cos ( static_cast<long double>(PI_FOUR)) ;
}

int main(int argc, char* argv[])
{
#if 1
	FFTTest () ;
#endif
#if 1
	DifferentiatorTest () ;
#endif
#if 1
	PrecTest () ;
#endif
	return 0;
}

