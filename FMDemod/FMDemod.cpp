// FMDemod.cpp
//

#include <vector>
#include <charconv>	
#include <cmath>
#include <complex>
#include <algorithm>
#include <functional>

#include <fmt/format.h>

#include "mm_file.h"
#include "wav_file.h"
#include "FMDemodFunctions.h"

typedef double F ;

template <typename T> void from_chars(char const* arg, T& result)
{
	auto [ptr, ec] = std::from_chars(arg, arg + strlen(arg), result);
}

void Welcome ()
{
	fmt::println("FMDemod 2.00 Copyright Paul Ranson (c) 2009-2026") ;
	fmt::println("email - paul@epicyclism.com\n") ;
}

void Usage ()
{
	fmt::println("Usage : fmdemod [<-L>|<-R>|<-M>|<-I>] [-F<xxxx>] [-C] [-B] [-N<0|1>] <inputwav> <outputdat> [rpm]") ;
	fmt::println("Options : -L, use left. -R, use right. -M, convert to mono.") ;
	fmt::println("          (These are all ignored if the input is mono...)") ;
	fmt::println("          -Fxxx, apply a narrow band filter centered on xxxxHz.") ;
	fmt::println("          -C, copy the loaded data to the output, no demodulation.") ;
	fmt::println("          -B[nnnn], input file is treated as a raw array of F at (if given) sample rate nnnn.") ;
	fmt::println("          (If also -I then raw array will be treated as stereo IQ pairing)") ;
	fmt::println("          -N, -N1, normalize centering on 0 and filter the result. Default behaviour.") ;
	fmt::println("          -N2,     normalize centering on 0 DO NOT low pass filter the result.") ;
	fmt::println("          -N0, do not normalize, do not filter. Will probably mess up a polar plot...") ;
	fmt::println("If an rpm is supplied then data will also be written to stdout\n0.0 gets you the raw data, anything else sets up for polar plotting\n") ;
	fmt::println("Use 33 for 33 1/3...\n") ;
	fmt::println("For example: fmdemod -M -F3150 LP12.wav LP12.dat 33 > LP12Polar.dat\n\n") ;
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

#if 0
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
#endif

int main(int argc, char* argv[])
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
	size_t sample_rate   = 96000 ;
	int		arg  = 1 ;
	while ( arg < argc )
	{
		if ( argv [ arg ][ 0 ] == '-' || argv [ arg ][ 0 ] == '/')
		{
			switch ( argv [ arg ][ 1 ])
			{
			case 'B' :
			case 'b' :
				bBare = true ;
				from_chars(argv[arg] + 2, sample_rate);
				if ( sample_rate == 0 )
					sample_rate = 96000 ;
				break ;
			case 'C' :
			case 'c' :
				bDemodulate = false ;
				break ;
			case 'F' :
			case 'f' :
				from_chars(argv[arg] + 2, nFilterFreq);
				break ;
			case 'L' :
			case 'l' :
				nInputProcFlags = 1 ;
				break ;
			case 'M' :
			case 'm' :
				nInputProcFlags = 0 ;
				break ;
			case 'N' :
			case 'n' :
				switch ( argv [ arg ][ 2 ])
				{
				case '0' :
					bNormalize = false; 
					break ;
				case '2' :
					bFilterDemod = false ;
					break ;
				}
				break ;
			case 'R' :
			case 'r' :
				nInputProcFlags = 2 ;
				break ;
			default :
				fmt::println("Unknown argument \'{}\'!", argv [ arg ][ 1 ]) ;
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
				from_chars(argv[arg], rpm);
			}
		}
		++arg ;
	}
	// inout buffer.
	//
	std::vector<F> inoutbuf ;
	// IQ buffer
	std::vector<std::complex<F> > iqbuf;

	if ( bBare )
	{
		// bare file!
		// assume mono unless 'IQ' is set.
		mem_map_file<F> mmf ( argv [ nInFileArg ]) ;
		if ( !mmf )
		{
			fmt::println("Couldn't open <{}>", argv [ nInFileArg ]) ;
			return -1 ;
		}
		// sometime refactor to work directly on the memory buffer 
		inoutbuf.resize ( mmf.length () / sizeof ( F )) ;		
		std::copy ( mmf.begin (), mmf.end (), inoutbuf.begin ()) ;

		fmt::println("Read {} samples from bare input file. Sample rate is {}.", inoutbuf.size (), sample_rate) ;
	}
	else
	{
		// wav file!
		Wav_File mp(argv[nInFileArg]);
		if(!mp.mmf)
		{
			fmt::println("Failed to open WAV file <{}>", argv[nInFileArg]);
			return -1;
		}
		inoutbuf.reserve(mp.total_samples());
		if (nInputProcFlags == 0)
		{
			// convert to mono
			do
			{
				auto [p, c] = mp.get_chunk(mp.wav_header.sampleRate * 60); // read 60 seconds
				inoutbuf.insert(inoutbuf.end(), p, p + c);
			} while (!mp.eof());
		}
		else
		{
			int channel = (nInputProcFlags == 1) ? 0 : 1;
			do
			{
				auto [p, c] = mp.get_chunk(mp.wav_header.sampleRate * 60, channel); // read 60 seconds
				inoutbuf.insert(inoutbuf.end(), p, p + c);
			} while (!mp.eof());
		}
		fmt::println("Read {} samples from input file. Sample rate is {}.", inoutbuf.size (), mp.sample_rate ()) ;

		sample_rate = mp.sample_rate () ;
	}
	
#if 0
	if ( nFilterFreq > 5000 ) // not wow and flutter
		DoLowPassFilter ( &inoutbuf [ 0 ], inoutbuf.size (), nFilterFreq * 7 / 4, sample_rate ) ;
	else
#endif
	if ( nFilterFreq > 0 )
		DoBandFilter     ( &inoutbuf [ 0 ], inoutbuf.size (), nFilterFreq, sample_rate ) ;
	if ( bDemodulate )
	{
		iqbuf.resize ( inoutbuf.size ()) ;
		GenerateIQ ( &inoutbuf [ 0 ], &iqbuf [ 0 ], iqbuf.size (), sample_rate ) ;
		EqualiseIQ ( &iqbuf[0], iqbuf.size ()) ;
		// IQ derivative buffer
		std::vector<std::complex<F> > iqdiffbuf ;
		iqdiffbuf.resize ( inoutbuf.size ()) ;
		Differentiate3 ( &iqbuf[0], &iqdiffbuf[0], iqdiffbuf.size ()) ;

		// demodulate
		DemodulateFinal ( &iqbuf[0], &iqdiffbuf[0], &inoutbuf[0], inoutbuf.size ()) ;

		if ( inoutbuf.size () < 1536 + 2048 + 1024 + 2048 )
		{
			fmt::println("Insufficient data demodulated!" );
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
#if 0
			std::wcerr << L"DC removal : min = " << *itmin << L" at offset "
				<< std::distance ( inoutbuf.begin (), itmin ) << L", max = " << *itmax << L" at offset " 
				<< std::distance ( inoutbuf.begin (), itmax ) << L", av = " << ave << std::endl ;
#endif
			std::transform ( inoutbuf.begin (), inoutbuf.end (), inoutbuf.begin (), [=](auto v){ return v - ave;}) ;
//			std::replace ( inoutbuf.begin (), inoutbuf.end (), 0.0, 1.0 ) ;
			ave = Mean2 ( inoutbuf.begin (), inoutbuf.end ()) ;
#if 0
			CERR << TEXT("Post normal av = ") << ave << TEXT("\n") ;
#endif
		}
		else
		{
			F fs = F ( sample_rate ) ;
			std::for_each ( inoutbuf.begin (), inoutbuf.end (), [fs]( F& rf )
			{
				rf = fs * rf / TWO_PI;
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
			F angle_inc = TWO_PI * (rpm / 60.0) / fs ;
			F angle = 0.0 ;
			F av = 0.0 ;
			F min = 1.0, max = 1.0 ;
			size_t steps = size_t((TWO_PI / 3600.0) / angle_inc ) ;
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
				F lim = bTT ? (5.0 * TWO_PI) : (50.0 * TWO_PI) ;
				if ( angle > lim )
					break ; // 5 or 100 revs...
				++cnt ;
				++it ;
			}
#if 1
		fmt::println("Min = {}, max = {}, range = {}.", min, max, max - min);
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
				fmt::print("{:10.6f}", *it) ;
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

