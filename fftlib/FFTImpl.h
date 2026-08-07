//
//	FFTImpl.h
//
// Copyright (c) 2008-2022 Paul Ranson, paul@epicyclism.com
//
// Refer to licence in repository.
//

#pragma once

// helpers to apply a Hamming window to a range of data.
// usually associated with FFT...
//
template <typename T, size_t FFTSZ> class HamFn
{
private :
	size_t ind_ ;
public :
	HamFn () : ind_ ( 0 )
	{
	}
	T operator () ()
	{
		T ret = static_cast<T>( 0.54 ) - static_cast<T>( 0.46 ) * cos ( static_cast<T>( 2 ) * std::numbers::pi * static_cast<T>( ind_ ) / static_cast<T>( FFTSZ - 1 )) ;
		++ind_ ;
		return ret ;
	}
} ;

// Blackman
template <typename T, size_t FFTSZ> class BlackmanFn
{
private :
	size_t ind_ ;
public :
	BlackmanFn () : ind_ ( 0 )
	{
	}
	T operator () ()
	{
		T ret = static_cast<T>( 7938 ) / static_cast<T>( 18608 )
			  - static_cast<T>( 9240 ) / static_cast<T>( 18608 ) * cos ( static_cast<T>( 2 ) * std::numbers::pi * static_cast<T>( ind_ ) / static_cast<T>( FFTSZ - 1 ))
			  + static_cast<T>( 1430 ) / static_cast<T>( 18608 ) * cos ( static_cast<T>( 4 ) * std::numbers::pi * static_cast<T>( ind_ ) / static_cast<T>( FFTSZ - 1 )) ;
		++ind_ ;
		return ret ;
	}
} ;

// Blackman-Harris
template <typename T, size_t FFTSZ> class BlackmanHarrisFn
{
private :
	size_t ind_ ;
public :
	BlackmanHarrisFn () : ind_ ( 0 )
	{
	}
	T operator () ()
	{
		T ret = static_cast<T>( 0.35875 ) - static_cast<T>( 0.48829 ) * cos ( static_cast<T>( 2 ) * std::numbers::pi * static_cast<T>( ind_ ) / static_cast<T>( FFTSZ - 1 ))
			+ static_cast<T>( 0.1365995 ) * cos ( static_cast<T>( 4 ) * std::numbers::pi * static_cast<T>( ind_ ) / static_cast<T>( FFTSZ - 1 ))
			- static_cast<T>( 0.0106411 ) * cos ( static_cast<T>( 6 ) * std::numbers::pi * static_cast<T>( ind_ ) / static_cast<T>( FFTSZ - 1 ));
		++ind_ ;
		return ret ;
	}
} ;

// Kaiser
//
template <typename T, size_t FFTSZ, size_t order> class KaiserFn
{
private :
	size_t ind_ ;
	T      div_ ;
public :
	KaiserFn () : ind_ ( 0 )
	{
#if defined(__clang__)
		div_ = boost::math::cyl_bessel_i ( 0, static_cast<T>(order) * std::numbers::pi) ;
#else
		div_ = std::cyl_bessel_i ( 0, static_cast<T>(order) * std::numbers::pi) ;
#endif	
}
	T operator () ()
	{
		T sq  = static_cast<T>(2) * static_cast<T>(ind_) / static_cast<T>(FFTSZ - 1 ) - static_cast<T>( 1 ) ;
		sq *= sq ;
		T arg = static_cast<T>(order) * std::numbers::pi * sqrt ( static_cast<T>(1) - sq ) ;
#if defined(__clang__)
		T ret = boost::math::cyl_bessel_i ( 0, arg ) ;
#else
		T ret = std::cyl_bessel_i ( 0, arg ) ;
#endif
		++ind_ ;
		return ret ;
	}
} ;

template <typename T, size_t FFTSZ> Window<T, FFTSZ>::Window ( window_t wt )
{
	// build ham table
	switch ( wt )
	{
	default :
	case window_t::HAMMING :
		std::generate ( coeff_table_.begin(), coeff_table_.end(), HamFn<T, FFTSZ>());
		break ;
	case window_t::NOWINDOW :
		std::fill (coeff_table_.begin(), coeff_table_.end(), static_cast<T>( 1 )) ;
		break ;
	case window_t::BLACKMAN :
		std::generate (coeff_table_.begin(), coeff_table_.end(), BlackmanFn<T, FFTSZ> ()) ;
		break ;
	case window_t::BLACKMANHARRIS :
		std::generate (coeff_table_.begin(), coeff_table_.end(), BlackmanHarrisFn<T, FFTSZ> ()) ;
		break ;
	case window_t::KAISER5 :
		std::generate (coeff_table_.begin(), coeff_table_.end(), KaiserFn<T, FFTSZ, 5> ()) ;
		break ;
	case window_t::KAISER7 :
		std::generate (coeff_table_.begin(), coeff_table_.end(), KaiserFn<T, FFTSZ, 7> ()) ;
		break ;
	}
	// calculate gain.
	auto t = std::accumulate(coeff_table_.begin(), coeff_table_.end(), T{ 0 });
	gain_ = T(FFTSZ) / t ;
}

template <typename T, size_t FFTSZ> 
template<typename II, typename OI> void Window<T, FFTSZ>::operator () (II samples_b, II samples_e, OI out_b) const
{
	std::transform ( samples_b, samples_e, coeff_table_.begin(), out_b, std::multiplies<T>());
}

template <typename T, size_t FFTSZ> T Window<T, FFTSZ>::Gain () const
{
	return gain_ ;
}

template <typename T, size_t FFTSZ, int Invert> class WFn
{
private :
	size_t ind_ ;
public :
	WFn () : ind_ ( 0 )
	{
		static_assert(Invert == 1 || Invert == -1, "WFn Invert must be 1 or -1 (-1 to invert)");
	}
	std::complex<T> operator () ()
	{
		// w = exp(-2*PI*i/N), w[k] = w^k
		// ^^^ ???
		T x = T( -2.0 * std::numbers::pi) * Invert * ind_ / FFTSZ ;
		++ind_ ;
		return std::complex<T> ( cos ( x ), sin ( x )) ;
	}
} ;

template < typename T, size_t FFTSZ, int Invert>
FFT<T, FFTSZ, Invert>::FFT () : div_ { Invert == 1 ? 1.0 : T{FFTSZ}}
{
	static_assert(Invert == 1 || Invert == -1, "WFn Invert must be 1 or -1 (-1 to invert)");

	// compute lgN_. Use compile time magic?
	size_t n ;
	size_t lg;
	for (n = FFTSZ, lg = 0; n > 1; n /= 2, ++lg)
		;
	size_t lgn = std::bit_width(FFTSZ) - 1;

	// compute 'w' (the complex roots of '1'. w[1]*w[1] == 1, w[2]*w[2]*w[2] == 1 etc etc.
	std::generate_n ( w_.begin(), FFTSZ / 2, WFn<T, FFTSZ, Invert>());
}

template < typename T, size_t FFTSZ, int Invert>
void FFT<T, FFTSZ, Invert>::operator () ( std::complex<T> * in, std::complex<T> * out )
{
	// set up
	std::complex<T> * to_ ;
	std::complex<T> * from_ ;
	if ( (std::bit_width(FFTSZ) - 1) % 2 == 0)
	{
		from_ = out ;
		to_   = buf_.data();
	}
	else
	{
		to_		= out ;
		from_	= buf_.data();
	}

	using namespace std::placeholders;
	// copy the input data to a workspace, dividing as necessary.
	std::transform ( in, in + FFTSZ, from_, std::bind ( std::divides<std::complex<T> >(), _1, div_ )) ;

	// the actual thing the thing
	for ( size_t k = FFTSZ / 2; k > 0; k /= 2 )
	{
		for ( size_t s = 0; s < k; ++s )
		{
			// initialize pointers
			std::complex<T> * f1, * f2, * t1, * t2, * ww ;
			std::complex<T> wwf2 ;
			f1 = &from_[s]; f2 = &from_[s+k];
			t1 = &to_[s]; t2 = &to_[s+FFTSZ/2];
			ww = w_.data();
			// compute <s,k>
			while ( ww < w_.data() + FFTSZ / 2)
			{
				// wwf2 = ww*f2
				wwf2 = *ww * *f2 ;
				// t1 = f1+wwf2
				*t1 = *f1 + wwf2 ;
				// t2 = f1-wwf2
				*t2 = *f1 - wwf2 ;
				// increment
				f1 += 2*k; f2 += 2*k;
				t1 += k; t2 += k;
				ww += k;
			}
		}
		std::swap ( from_, to_ ) ;
	}
}
