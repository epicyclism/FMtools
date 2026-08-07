// FMDemod.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "MediaProcessor.h"
#include "MemMapFile.h"
#include "FMDemodFunctions.h"

typedef double F ;

void Welcome ()
{
	CERR << TEXT("FMDemod 1.11 Copyright Paul Ranson (c) 2009-2016\n") ;
	CERR << TEXT("email - paul2718@gmail.com\n\n") ;
	CERR << TEXT("(sizeof F is ") << sizeof ( F ) << TEXT(")\n\n") ;
}

void Usage ()
{
	CERR << TEXT("Usage : fmdemod [<-L>|<-R>|<-M>|<-I>] [-F<xxxx>] [-C] [-B] [-N<0|1>] <inputwav> <outputdat> [rpm]\n") ;
	CERR << TEXT("Options : -L, use left. -R, use right. -M, convert to mono.\n") ;
	CERR << TEXT("          -I, if stereo input assume ready made IQ pair.\n") ;
	CERR << TEXT("          (These are all ignored if the input is mono...)\n") ;
	CERR << TEXT("          -Fxxx, apply a narrow band filter centered on xxxxHz.\n\n") ;
	CERR << TEXT("          -C, copy the loaded data to the output, no demodulation.\n") ;
	CERR << TEXT("          -B[nnnn], input file is treated as a raw array of F at (if given) sample rate nnnn.\n") ;
	CERR << TEXT("          (If also -I then raw array will be treated as stereo IQ pairing)\n") ;
	CERR << TEXT("          -N, -N1, normalize centering on 0 and filter the result. Default behaviour.\n") ;
	CERR << TEXT("          -N2,     normalize centering on 0 DO NOT low pass filter the result.\n") ;
	CERR << TEXT("          -N0, do not normalize, do not filter. Will probably mess up a polar plot...\n") ;
	CERR << TEXT("If an rpm is supplied then data will also be written to stdout\n0.0 gets you the raw data, anything else sets up for polar plotting\n") ;
	CERR << TEXT("Use 33 for 33 1/3...\n\n") ;
	CERR << TEXT("For example: fmdemod -M -F3150 LP12.wav LP12.dat 33 > LP12Polar.dat\n\n") ;
	CERR << TEXT("(sizeof F is ") << sizeof ( F ) << TEXT(")\n\n") ;
}

template <typename T> F Mean ( T begin, T end )
{
	size_t m = 0 ;
	size_t n = 0 ;
	double mean = 0.0 ;
	double partmean = 0.0 ;

	while( begin != end )
	{
		++n ;
		double d = (*begin) - partmean ;
		partmean = partmean + d / n ;
		if ( n == 10000 )
		{
			n = 0 ;
			mean += partmean ;
			partmean = 0.0 ;
			++m ;
		}
		++begin ;
	}
#if 0
	// not quite right since the last part doesn't contribute equally...
	mean += partmean ;
	mean /= m + 1 ;
#else
	// or just ignore it....
	mean /= m ;
#endif

	return F( mean ) ;
}

template <typename T> F Mean2 ( T begin, T end )
{
	size_t m = 0 ;
	size_t n = 0 ;
	double mean = 0.0 ;
	double partmean = 0.0 ;

	while( begin != end )
	{
		++n ;
		partmean += *begin ;
		if ( n == 5000 )
		{
			mean += partmean / n ;
			partmean = 0.0 ;
			n = 0 ;
			++m ;
		}
		++begin ;
	}
	// not quite right since the last part doesn't contribute equally...
	mean += partmean / n ;
	mean /= m + 1 ;

	return F( mean ) ;
}

template<typename T> void DumpRange ( T b, T e )
{
	size_t m = 0 ;
	size_t n = 0 ;
	while ( b != e )
	{
		++n ;
		if ( n == 101 )
		{
			TCHAR buf [ 255 ] ;
			COUT << TEXT("Pause at ") << m << TEXT("\n") ; 
			CIN.getline ( buf, 255 ) ;
			n = 0 ;
		}
		COUT << *b << std::endl ;
		++m ;
		++b ;
	}
}

template <typename T> void PrintComplex ( T t )
{
	COUT << t << std::endl ;
}

template <typename R> void PrintReal ( R r )
{
	COUT << r << std::endl ;
}

int _tmain(int argc, _TCHAR* argv[])
{
	Welcome () ;

	if ( argc < 3 )
	{
		Usage () ;
	
		return -1 ;
	}

	int		nInFileArg  = 1 ;
	int		nOutFileArg = 2 ;
	int		nInputProcFlags = 0 ; // 0 = make mono, 1 = left, 2 = right
	F    	rpm = -1 ;
	int		nFilterFreq = 0 ;
	bool    bDemodulate = true ;
	bool    bNormalize  = true ;
	bool	bFilterDemod = true ;
	bool	bBare        = false ; // assume wav
	bool    bIQ		 = false ; // assume full demod
	size_t sample_rate   = 96000 ;
	int		arg  = 1 ;
	while ( arg < argc )
	{
		if ( argv [ arg ][ 0 ] == TEXT ('-') || argv [ arg ][ 0 ] == TEXT('/'))
		{
			switch ( argv [ arg ][ 1 ])
			{
			case TEXT('B') :
			case TEXT('b') :
				bBare = true ;
				sample_rate = ::_tstoi ( argv [ arg ] + 2 ) ;
				if ( sample_rate == 0 )
					sample_rate = 96000 ;
				break ;
			case TEXT('C') :
			case TEXT('c') :
				bDemodulate = false ;
				break ;
			case TEXT('F') :
			case TEXT('f') :
				nFilterFreq = ::_tstoi ( argv [ arg ] + 2 ) ;
				break ;
			case TEXT('I') :
			case TEXT('i') :
				bIQ = true ;
				break ;
			case TEXT('L') :
			case TEXT('l') :
				nInputProcFlags = 1 ;
				break ;
			case TEXT('M') :
			case TEXT('m') :
				nInputProcFlags = 0 ;
				break ;
			case TEXT('N') :
			case TEXT('n') :
				switch ( argv [ arg ][ 2 ])
				{
				case TEXT('0') :
					bNormalize = false; 
					break ;
				case TEXT('2') :
					bFilterDemod = false ;
					break ;
				}
				break ;
			case TEXT('R') :
			case TEXT('r') :
				nInputProcFlags = 2 ;
				break ;
			default :
				CERR << TEXT("Unknown argument \'") << argv [ arg ][ 1 ] << TEXT("\'!\n") ;
				Usage () ;
				return -1 ;
			}
			++nInFileArg ;
			++nOutFileArg ;
		}
		else
		{
			if ( arg > nOutFileArg )
			{
				rpm = F( ::_tstof ( argv [ arg ])) ;
			}
		}
		++arg ;
	}
	if ( bIQ && !bDemodulate )
	{
		CERR << TEXT("Incompatible arguments '-I' and '-C'.\n") ;
		return -1 ;
	}
	// inout buffer.
	//
	std::vector<F> inoutbuf ;
	// IQ buffer
	std::vector<std::complex<F> > iqbuf ;

	if ( bBare )
	{
		// bare file!
		// assume mono unless 'IQ' is set.
		MemoryMappedFile<F> mmf ( argv [ nInFileArg ]) ;
		if ( !mmf )
		{
			CERR << TEXT("Couldn't open <") << argv [ nInFileArg ] << TEXT(">\n") ;

			return -1 ;
		}
		if ( bIQ )
		{
			iqbuf.resize ( mmf.Length () / (sizeof ( F ) * 2 )) ;
			F const* pF = mmf.Ptr () ;
			std::for_each ( iqbuf.begin (), iqbuf.end (), [&pF]( std::complex<F>& vt )
			{
				vt = std::complex<F>( *(pF + 1), *pF ) ;
				pF += 2 ;
			}) ;
			CERR << "Read " << iqbuf.size () << " sample pairs from bare input file. Sample rate is " << sample_rate << ".\n" ;
			// make space for later
			inoutbuf.resize ( iqbuf.size ()) ;
		}
		else
		{
			// sometime refactor to work directly on the memory buffer 
			inoutbuf.resize ( mmf.Length () / sizeof ( F )) ;
			std::copy ( mmf.First (), mmf.Last (), inoutbuf.begin ()) ;

			CERR << "Read " << inoutbuf.size () << " samples from bare input file. Sample rate is " << sample_rate << ".\n" ;
		}
	}
	else
	{
		// wav file!
		MediaProcessor mp ;
		try
		{
			mp.Open ( argv [ nInFileArg ]) ;
			if ( bIQ )
			{
				mp.ReadComplex ( iqbuf ) ;
				// report
				CERR << TEXT("Read ") << iqbuf.size () << TEXT(" sample pairs from input file. Sample rate is ") << mp.SampleRate () << TEXT(".\n") ;
				// make space for later
				inoutbuf.resize ( iqbuf.size ()) ;
			}
			else
			{
				mp.Read ( inoutbuf, nInputProcFlags ) ;
				// report
				CERR << TEXT("Read ") << inoutbuf.size () << TEXT(" samples from input file. Sample rate is ") << mp.SampleRate () << TEXT(".\n") ;
			}
		}
		catch (std::exception& ex )
		{
			std::cerr << "Failed to handle media file <" ;
			std::wcerr << argv [ nInFileArg ] ;
			std::cerr << ">\n" ;
			std::cerr << ex.what () << "\n\n" ;

			return -1 ;
		}
		sample_rate = mp.SampleRate () ;
	}
	
	// filter?
	if ( bIQ )
	{
		if ( nFilterFreq > 0 )
			DoBandFilter     ( &iqbuf [ 0 ], iqbuf.size (), nFilterFreq, sample_rate ) ;
	}
	else
	{
#if 0
		if ( nFilterFreq > 5000 ) // not wow and flutter
			DoLowPassFilter ( &inoutbuf [ 0 ], inoutbuf.size (), nFilterFreq * 7 / 4, sample_rate ) ;
		else
#endif
		if ( nFilterFreq > 0 )
			DoBandFilter     ( &inoutbuf [ 0 ], inoutbuf.size (), nFilterFreq, sample_rate ) ;
	}
	if ( bDemodulate )
	{
		if ( bIQ )
		{
			EqualiseIQ ( &iqbuf[0], iqbuf.size ()) ;
		}
		else
		{
			iqbuf.resize ( inoutbuf.size ()) ;
			GenerateIQ ( &inoutbuf [ 0 ], &iqbuf [ 0 ], iqbuf.size (), sample_rate ) ;
			EqualiseIQ ( &iqbuf[0], iqbuf.size ()) ;
		}
		// IQ derivative buffer
		std::vector<std::complex<F> > iqdiffbuf ;
		iqdiffbuf.resize ( inoutbuf.size ()) ;
		Differentiate3 ( &iqbuf[0], &iqdiffbuf[0], iqdiffbuf.size ()) ;

		// demodulate
		DemodulateFinal ( &iqbuf[0], &iqdiffbuf[0], &inoutbuf[0], inoutbuf.size ()) ;

		if ( inoutbuf.size () < 1536 + 2048 + 1024 + 2048 )
		{
			std::wcerr << L"Insufficient data demodulated!\n" ;
			return -1 ;
		}
		// trim off the end to account for unprocessed points at the end of filter operations
		inoutbuf.resize ( inoutbuf.size () - 2048 - 1024 - 2048 ) ;
		// trim off the beginning
		inoutbuf.erase ( inoutbuf.begin (),  inoutbuf.begin () + 1536 ) ;

		if ( bNormalize )
		{
			// remove the crud?
			if ( bFilterDemod )
				DoLowPassFilter ( &inoutbuf [ 0 ], inoutbuf.size (), sample_rate ) ;
			// normalize
#if 0
			F min = *std::min_element ( inoutbuf.begin (), inoutbuf.end ()) ;
			F max = *std::max_element ( inoutbuf.begin (), inoutbuf.end ()) ;
#else
			auto itmin = std::min_element ( inoutbuf.begin () + 3, inoutbuf.end ()) ;
			auto itmax = std::max_element ( inoutbuf.begin () + 3, inoutbuf.end ()) ;
#endif
			F ave = Mean2 ( inoutbuf.begin (), inoutbuf.end ()) ;
			std::wcerr << L"DC removal : min = " << *itmin << L" at offset "
				<< std::distance ( inoutbuf.begin (), itmin ) << L", max = " << *itmax << L" at offset " 
				<< std::distance ( inoutbuf.begin (), itmax ) << L", av = " << ave << std::endl ;
			std::transform ( inoutbuf.begin (), inoutbuf.end (), inoutbuf.begin (), std::bind2nd ( std::minus<F>(), ave )) ;
//			std::replace ( inoutbuf.begin (), inoutbuf.end (), 0.0, 1.0 ) ;
			ave = Mean2 ( inoutbuf.begin (), inoutbuf.end ()) ;
			CERR << TEXT("Post normal av = ") << ave << TEXT("\n") ;
		}
		else
		{
			F fs = F ( sample_rate ) ;
			std::for_each ( inoutbuf.begin (), inoutbuf.end (), [fs]( F& rf )
			{
				rf = fs * rf / PI<F>::TwoPi ();
			}) ;
		}
		// optional polar output
		if ( rpm > 0.0 )
		{
			bool bTT = rpm < 100.0 ;
			std::vector<F>::iterator it  = inoutbuf.begin () ;
			std::vector<F>::iterator itE = inoutbuf.end () ;
			if ( rpm == 33.0 )
				rpm = F(100.0 / 3.0) ; // correct to 33 1/3
			F fs = F ( sample_rate ) ;
			F angle_inc = PI<F>::TwoPi () * (rpm / 60.0) / fs ;
			F angle = 0.0 ;
			F av = 0.0 ;
			F min = 1.0, max = 1.0 ;
			size_t steps = size_t((PI<F>::TwoPi () / 3600.0) / angle_inc ) ;
			size_t cnt = 0 ;
//			size_t orbits = 1 ;
			// try and start 5 revs in?
#if 1
			size_t start_off = size_t( rpm * 5 / 60.0 * fs ) ;
#else
			size_t start_off = 0;
#endif
			if ( inoutbuf.size () > 2 * start_off )
				it = it + start_off ;
			else
				++it ;
			while ( it != itE )
			{
#if 0
				if ( cnt == steps && (*it) != 0.0 )
				{
					av /= steps ;
					// amplify, to illuminate
					av = (av - 1.0) * 50.0 + 1.0 ;
					std::cout << angle << " " << av << "\n" ;
					cnt = 0 ;
					av = 0.0 ;
				}
				av += *it ;
#else
				if ( cnt >= steps )
				{
					if ((*it) > max )
						max = *it ;
					if ((*it) < min )
						min = *it ;
					if ( bTT )
						std::cout << angle << " " << (*it) * 50.0 + 1.0 << "\n" ;
					else
						std::cout << angle << " " << (*it) * 500.0 + 1.0 << "\n" ;
					cnt = 0 ;
				}
#endif
				angle += angle_inc ;
				F lim = bTT ? (5.0 * PI<F>::TwoPi ()) : (50.0 * PI<F>::TwoPi ()) ;
				if ( angle > lim )
					break ; // 5 or 100 revs...
				++cnt ;
				++it ;
			}
#if 1
			std::wcerr << L"Min = " << min << L", max = " << max << L", range = " << max - min << L".\n" ;
#endif
		}

		// normalise for output
		if ( bNormalize )
		{
		}
	}
	if ( rpm == 0.0 )
	{
		auto it  = inoutbuf.begin () ;
		auto itE = inoutbuf.end () ;
		unsigned n = 0;
		while ( it != itE )
		{
			if ( n == 0 )
				std::cout << *it << "\n" ;	
			++n;
			if (n == 10)
				n = 0;
			++it ;
		}
	}
	BasicFile<GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL> ofdemodphase ( argv [ nOutFileArg ]) ;
	ofdemodphase.WriteFile ( &inoutbuf[0], DWORD(inoutbuf.size () * sizeof ( F ))) ;

	return 0;
}

