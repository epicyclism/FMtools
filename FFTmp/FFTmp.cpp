// FFTmp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "fft.h"
#include "procfft.h"
#include "fftFactory.h"
#include "MemMapFile.h"

typedef double F ;

void Welcome ()
{
	CERR << TEXT("FFTmp 1.01 Copyright Paul Ranson (c) 2009-2014\n") ;
	CERR << TEXT("email - paul2718@gmail.com\n\n") ;
	CERR << TEXT("Performs single FFT on a file of raw sample data.\n") ;
	CERR << TEXT("Writes magnitude and phase resuls to stdout, requires at least FFT width input samples.\n") ;
	CERR << TEXT("(sizeof F is ") << sizeof ( F ) << TEXT(")\n\n") ;
}

void Usage ()
{
	CERR << TEXT("Performs single FFT on a file of raw sample data\n") ;
	CERR << TEXT("Usage : FFTmp [-Fn] [-D] [-Wn] <input file> [sample rate]\n") ;
	CERR << TEXT("Where input file is a packed array of floats. Output is text to stdout.\n") ;
	CERR << TEXT("Options. -Fn, use an FFT width of 2^n.\n") ;
	CERR << TEXT("              n between 8 for 256 and 24 for 16777216.\n" ) ;
	CERR << TEXT("              Default is 18 for 262144\n" ) ;
	CERR << TEXT("         -D,  output in dB scaled so 1.0 is 0dB\n" ) ;
	CERR << TEXT("         -Wn, select a window function. 0 is no window.") ;
	CERR << TEXT("				1 is Hamming and the default.\n") ;
	CERR << TEXT("              2 is Blackman, 3 Blackman-Harris.\n" ) ;
	CERR << TEXT("              4 is Kaiser5,  5 Kaiser7.\n" ) ;
	CERR << TEXT("And if you provide the sample rate, the centre frequencies of each bin are written to the output.\n\n") ;
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
	bool    bOnce = false ;
	size_t sample_rate = -1 ;
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
	if (mmf.Length() / sizeof(F) < fftWidth)
	{
		CERR << TEXT("FFTWidth ") << fftWidth << TEXT(" exceeds available signal of ") << mmf.Length() / sizeof(F) << TEXT(" samples.\n");
		Usage () ;
		return -1 ;
	}

	// an FFT implementation!
	std::unique_ptr<IProcessorFFT<F>> pfft (GetFFT<F>(fftWidth, wt)) ;

	// report
	CERR << TEXT("FFTmp. Processing,  width ") << pfft->Width () << TEXT(", window ") << WTToString ( wt ) << TEXT("\n") ;
	
	// just a single effort
	std::complex<F>* result;

	(*pfft) (mmf.Ptr(), &result);

	// run through twice, two data sets...
	//
	if ( sample_rate )
	{
		double fb = 0 ;
		double fbinc = double ( sample_rate ) /  pfft->Width () ;

		// magnitudes
		for ( size_t i = 0; i <  pfft->Width () / 2 ; ++i )
		{
			F out ;
			if ( bDB )
				out = F(20.0) * log10(result[i].real());
			else
				out = result[i].real();

			COUT << fb << TEXT(" ") << out << std::endl ;
			fb += fbinc ;
		}
		// separate the records
		COUT << std::endl << std::endl;
		fb = 0 ;
		// phases
		for ( size_t i = 0; i <  pfft->Width () / 2 ; ++i )
		{
			COUT << fb << TEXT(" ") << result[i].imag() * F(10) << std::endl ;
			fb += fbinc ;
		}
	}
	else
	{
		for ( size_t i = 0; i <  pfft->Width () / 2; ++i )
		{
			F out ;
			if ( bDB )
				out = F(20.0) * log10 ( result [ i ].real()) ;
			else
				out = result [ i ].real() ;
			COUT << out << std::endl ;
		}
		COUT << std::endl << std::endl;
		for ( size_t i = 0; i <  pfft->Width () / 2 ; ++i )
		{
			COUT << result[i].imag() * F(10) << std::endl;
		}
	}

	return 0;
}

