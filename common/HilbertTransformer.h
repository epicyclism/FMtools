//
// Copyright (c) 2008-2022 Paul Ranson, paul@epicyclism.com
//
// Refer to licence in repository.
//

//
// Implement a sort of generic Hilbert Transformer
//
// Input from a complete buffer, output aligned real and imaginary into a complex buffer.
//

// K  - width of filter, assumed odd.
// F  - type of input variable
// SR - Sample rate in Hz
//
template< size_t K, typename F > struct HilbertTransformer
{
private :
	F coeff_ [ K ] ;

public :
	HilbertTransformer ( size_t SR )
	{
		// create the coefficients
		// they need to be 'backwards'.
		int n = K / 2 ;
		for ( int cnt = 0 ; cnt < K; ++cnt )
		{
			if ( n == 0 )
				coeff_ [ cnt ] = F ( 0 ) ;
			else
				coeff_ [ cnt ] = /*SR **/ ( F( 1 ) - cos ( F (std::numbers::pi * n )))  / (n * F( std::numbers::pi )) ;
#if 1
#if 0
			// Blackman-Harris window...
			coeff_ [ cnt ] *= F( 0.35875 - 0.48829 * cos ( 2.0 * std::numbers::pi * cnt / K ) + 0.14128 * cos ( 4.0 * std::numbers::pi * cnt / K ) - 0.01168 * cos ( 6.0 * std::numbers::pi * cnt / K )) ;
#else
			coeff_ [ cnt ] *= F(0.5) + F(0.5) * cos ( F(2.0 * std::numbers::pi * n / (K - 1))) ;
#endif
#endif
			--n ;
		}
	}

	// writes len - K/2 to out.
	// out is complex of input real and Hilbert imaginary
	//
	void Process (F const* in, size_t len, std::complex<F>* out )
	{
		for ( size_t off = 0; off < len - K; ++off )
		{
			F acc = F( 0 ) ;

			for ( size_t k = 0; k < K; k+=2 ) // odd coefficients are zero
			{
				acc += *(in+k) * coeff_ [ k ] ;
			}
			*out = std::complex<F>( *(in+K/2), acc ) ;
			++in ;
			++out ;
		}
	}
} ;