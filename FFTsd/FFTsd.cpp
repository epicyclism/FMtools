// FFTsd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "fft.h"
#include "procfft.h"
#include "fftFactory.h"
#include "MemMapFile.h"

typedef double F ;

void Welcome ()
{
	CERR << TEXT("FFTsd 1.02 Copyright Paul Ranson (c) 2012\n") ;
	CERR << TEXT("email - paul2718@gmail.com\n\n") ;
	CERR << TEXT("Performs sequential FFTs on a file of raw sample data.\n") ;
	CERR << TEXT("Output is something suitable for plotting with GNUPlot\n") ;
	CERR << TEXT("(sizeof F is ") << sizeof ( F ) << TEXT(")\n\n") ;
}

void Usage ()
{
	CERR << TEXT("Performs sequential FFTs on a file of raw sample data.\n") ;
	CERR << TEXT("Usage : FFTsd [-Fn] [-D] [-Sn] [-Wn] [-Ln] [-Hn] <input file> [sample rate]\n") ;
	CERR << TEXT("Where input file is a packed array of floats. Output is text to stdout.\n") ;
	CERR << TEXT("Options. -Fn, use an FFT width of 2^n.\n") ;
	CERR << TEXT("              n between 8 for 256 and 24 for 16777216.\n" ) ;
	CERR << TEXT("              Default is 18 for 262144\n" ) ;
	CERR << TEXT("         -D,  output in dB scaled so 1.0 is 0dB\n" ) ;
	CERR << TEXT("         -Sn, step n samples between consecutive ffts.\n") ;
	CERR << TEXT("              default is 50 or (if given) sample_rate/1000\n") ;
	CERR << TEXT("         -Wn, select a window function. 0 is no window.\n") ;
	CERR << TEXT("				1 is Hamming and the default.\n") ;
	CERR << TEXT("              2 is Blackman, 3 Blackman-Harris.\n" ) ;
	CERR << TEXT("              4 is Kaiser5,  5 Kaiser7.\n" ) ;
	CERR << TEXT("         -Ln, start writing output above n, n is Hz if a sample rate provided, otherwise the bucket index\n") ;
	CERR << TEXT("         -Hn, stop writing output above n, etc.\n") ;
	CERR << TEXT("And if you provide the sample rate, the centre frequencies of each bin are written to the output as 'x'.\n\n") ;
}

// store results into here, so for each frequency a time series.
//
struct bucket
{
	F freq_ ;
	std::vector<F> res_ ;
} ;

// prepare the output array
//
bool PrepareOutputArray ( std::vector<bucket>& rab, size_t width, size_t sample_rate, size_t low, size_t high )
{
	if ( low >= high )
	{
		CERR << TEXT("low limit is greater than or equal to upper limit!\n") ;
		return false ;
	}
	F bucket_width = F(1) ;
	if ( sample_rate )
	{
		bucket_width = F(sample_rate) / width ;
	}
	rab.resize ( high - low ) ;
	size_t cnt = high - low ;
	std::for_each ( rab.begin (), rab.end (), [=, &low]( bucket& rb )
	{
		rb.freq_ = F(low) * bucket_width ;
		rb.res_.reserve ( cnt ) ;
		++low ;
	}) ;

	return true ;
}

int _tmain(int argc, _TCHAR* argv[])
{
	Welcome () ;

	if ( argc < 2 )
	{
		Usage () ;
	
		return -1 ;
	}
	int		nInFileArg  = 0 ;
	size_t  fftWidth = 18 ;
	bool    bDB = false ;
	size_t  step_sz = 0 ;
	size_t  sample_rate = -1 ;
	size_t  low = 0 ;
	size_t  high = 0 ;
	WindowType wt = HAMMING ;

	int		arg  = 1 ;
	while ( arg < argc )
	{
		if ( argv [ arg ][ 0 ] == TEXT ('-') || argv [ arg ][ 0 ] == TEXT('/'))
		{
			switch ( argv [ arg ][ 1 ])
			{
			case TEXT('F') :
			case TEXT('f') :
				fftWidth = ::_tstoi ( argv [ arg ] + 2 ) ;
				break ;
			case TEXT('D') :
			case TEXT('d') :
				bDB = true ;
				break ;
			case TEXT('L') :
			case TEXT('l') :
				low = ::_tstoi ( argv [ arg ] + 2 ) ;
				break ;
			case TEXT('H') :
			case TEXT('h') :
				high = ::_tstoi ( argv [ arg ] + 2 ) ;
				break ;
			case TEXT('S') :
			case TEXT('s') :
				step_sz = ::_tstoi ( argv [ arg ] + 2 ) ;
				break ;
			case TEXT('W') :
			case TEXT('w') :
				wt = WTFromCode ( argv [ arg ][ 2 ]) ;
				break ;
			default :
				CERR << TEXT("Unknown argument \'") << argv [ arg ][ 1 ] << TEXT("\'!\n") ;
				Usage () ;
				return -1 ;
			}
		}
		else
		{
			if ( nInFileArg == 0 )
			{
				nInFileArg = arg ;
			}
			else
			{
				sample_rate = ::_ttoi ( argv [ arg ]) ;
				if ( step_sz == 0 )
					step_sz = sample_rate / 1000 ; // milliseconds
			}
		}
		++arg ;
	}
	// checks
	if ( sample_rate == 0 )
	{
		CERR << TEXT("Sample rate provided was not understood\n") ;
		Usage () ;
		return -1 ;
	}
	if ( step_sz == 0 )
		step_sz = 50 ;
	if ( fftWidth < FFTWdMin || fftWidth > FFTWdMax )
	{
		CERR << TEXT("FFTWidth provided is out of range, valid between") << FFTWdMin << TEXT(" and ") << FFTWdMax << TEXT(" inclusive.\n" ) ;
		Usage () ;
		return -1 ;
	}
	MemoryMappedFile<F> mmf ( argv [ nInFileArg ]) ;
	if ( !mmf )
	{
		CERR << TEXT("Couldn't open <") << argv [ nInFileArg ] << TEXT(">\n") ;

		return -1 ;
	}

	// an FFT implementation!
	std::auto_ptr<IProcessorFFT<F> > pfft ( GetFFT<F> ( fftWidth, wt )) ;

	if ( high == 0 )
		high = pfft->Width () / 2 ;

	// frequency to bucket index
	if ( sample_rate )
	{
		F bucket_width = F(sample_rate) / pfft->Width () ;
		low = size_t( F(low) / bucket_width ) ;
		high = size_t( F(high) / bucket_width ) + 1 ;
		if ( high > pfft->Width () / 2 )
			high = pfft->Width () / 2 ;
	}

	std::vector<bucket> outarray ;
	if ( !PrepareOutputArray ( outarray, pfft->Width (), sample_rate, low, high ))
		return -1 ;

	// work
	if ( mmf.Length () / sizeof ( F ) < pfft->Width ())
	{
		CERR << TEXT("Insufficient signal supplied for the specified FFT width\n") ;
		return -1 ;
	}
	size_t nffts = (mmf.Length () / sizeof ( F ) - pfft->Width ()) / step_sz ;
	if ( nffts < 1 ) // 1 would be pretty pointless, but remains valid...
	{
		CERR << TEXT("Insufficient signal supplied for the specified FFT width\n") ;
		return -1 ;
	}

	// report
	CERR << TEXT("FFTsd. Performing ") << nffts << TEXT(" sequential FFTs,  width ") << pfft->Width () << TEXT(", step size ") << step_sz << TEXT(", window ") << WTToString ( wt ) << TEXT("\n") ;
	
	size_t w = pfft->Width () / 2 ;
	// make it equivalent to 200Hz
	std::vector<F> output ;
	output.resize ( pfft->Width ()) ;
	for ( size_t n = 0; n < nffts; ++n )
	{
		(*pfft) (  mmf.Ptr () + n * step_sz, &output[0] ) ;
		size_t i = low ;
#if 1
		std::for_each ( outarray.begin (), outarray.end (), [bDB, output, &i](bucket& rb )
		{
			F out ;
			if ( bDB )
			{
				if ( output [ i ] > F(0))
					out = F(20.0) * log10 ( output [ i ]) ;
				else
					out = F(-200) ;
			}
			else
				out = output [ i ] ;
			rb.res_.push_back ( out ) ;
			++i ;
		}) ;
#else
		auto bit = outarray.begin () ;
		for ( size_t i = low; i < high ; ++i )
		{
			F out ;
			if ( bDB )
				out = F(20.0) * log10 ( output [ i ]) ;
			else
				out = output [ i ] ;
			(*bit).res_.push_back ( out ) ;
			++bit ;
		}
#endif
	}
	// now print the results
	std::for_each ( outarray.begin (), outarray.end (), [outarray, step_sz]( bucket const& rb )
	{
		size_t time = 0 ;
//		COUT << TEXT("# freq = ") << rb.freq_ << TEXT("\n") ;
		std::for_each ( rb.res_.begin (), rb.res_.end (), [&time, step_sz]( F f )
		{
			COUT << time << TEXT(" ") << f << TEXT("\n") ;
			time += step_sz ;
		}) ;
		COUT << TEXT("\n") ;
	}) ;

	return 0;
}

