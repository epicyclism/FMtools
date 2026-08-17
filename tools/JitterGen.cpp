// JitterGen.cpp : Defines the entry point for the console application.
//

#include <charconv>

#include <fmt/format.h>

#include "basic_file.h"
#include "constants.h"
#include "FMDemodFunctions.h"
#include "jtestfns.h"

template <typename T> void from_chars(char const* arg, T& result)
{
	auto [ptr, ec] = std::from_chars(arg, arg + strlen(arg), result);
}

void usage ()
{
	fmt::println("Generates a jittered sine wave in a raw file.") ;
	fmt::println("Usage -" ) ;
	fmt::println("JitterGen [-S] <carrier frequency> <jitter frequency> <jitter depth> <sample rate> <duration> <outputfile>") ;
	fmt::println("produces sinusoidal jitter, depth is the desired height of the jitter sidebands in dbfs.") ;
	fmt::println("JitterGen -R <carrier frequency> <max offset> <sample rate> <duration> <outputfile>") ;
	fmt::println("produces random jitter with a rectangular pdf and maximum offset is pico seconds.") ;
	fmt::println("JitterGen -G <carrier frequency> <offset> <sample rate> <duration> <outputfile>") ;
	fmt::println("produces random jitter with a Gaussian pdf and offset is std dev in pico seconds.") ;
	fmt::println("JitterGen -I <frequency> <sample rate> <duration> <outputfile>") ;
	fmt::println("produces an IQ quadrature pair.") ;
	fmt::println("For these options duration is in seconds and the output is a packed array of F.\n" ) ;
	fmt::println("JitterGen -J[16|24|F] <sample rate> <duration> <outputfile>") ;
	fmt::println("Creates a JTest signal of 16 or 24 bits matched to the sample rate.") ;
	fmt::println("JitterGen -Q[16|24|F] <sample rate> <duration> <outputfile>") ;
	fmt::println("Creates a 1/4 fs square wave (Jtest without the wobble) of 16 or 24 bits.") ;
	fmt::println("JitterGen -QI[16|24] <sample rate> <duration> <outputfile>") ;
	fmt::println("Creates a 1/4 fs square wave of 16 or 24 bits, in QUADRATURE.") ;
	fmt::println("JitterGen -9[16|24] <sample rate> <duration> <outputfile>") ;
	fmt::println("Creates a -90.31dB 1k sine of 16 or 24 bits.") ;
	fmt::println("For these options duration is in seconds and the output is a packed array of either 16 or 32 bit ints or F.") ;
}

enum DataType { I16, I32, FLT};

DataType DoDT(char ch)
{
	DataType dt = I16;

	switch (ch)
	{
	case '1':
		dt = I16;
		break;
	case '2':
		dt = I32;
		break;
	case 'f':
	case 'F':
		dt = FLT;
		break;
	default:
		break;
	}

	return dt;
}

int main(int argc, char** argv)
{
	if ( argc < 5 )
	{
		usage () ;
		return -1 ;
	}
	F carrier = F(0) ;
	F jitter  = F(0) ;
	F depth   = F(0) ;
	size_t sample_rate = 0 ;
	size_t duration    = 0 ;
	DataType dt = I16;

	enum JitterType { SINE, RANDOM, GAUSSIAN, JTEST, QTEST, QTEST_IQ, _1BIT, IQ  } ;
	JitterType jt = SINE ;
	size_t an = 1 ;
	if ( argv [ 1 ][ 0 ] == '-' || argv [ 1 ][ 0 ] == '/')
	{
		switch ( argv [ 1 ][ 1 ])
		{
		case 'S' :
		case 's' :
			jt = SINE ;
			break ;
		case 'R' :
		case 'r' :
			jt = RANDOM ;
			break ;
		case 'G' :
		case 'g' :
			jt = GAUSSIAN ;
			break ;
		case 'I' :
		case 'i' :
			jt = IQ ;
			break ;
		case 'J' :
		case 'j' :
			jt = JTEST ;
			dt = DoDT(argv[1][2]);
			break ;
		case 'Q' :
		case 'q' :
			if ( argv [ 1 ][ 2 ] == 'I' || argv [ 1 ][ 2 ] == 'i')
			{
				jt = QTEST_IQ ;
			}
			else
			{
				jt = QTEST ;
			}
			dt = DoDT(argv[1][2]);
			break ;
		case '9' :
			jt = _1BIT ;
			dt = DoDT(argv[1][2]);
			break ;
		default :
			usage () ;
			return -1 ;
		}
		an = 2 ;
	}

	switch ( jt )
	{
	case SINE :
		from_chars(argv [ an ], carrier) ; ++an ;
		from_chars(argv [ an ], jitter) ; ++an ;
		from_chars(argv [ an ], depth) ; ++an ;
		break ;	
	case RANDOM :
		from_chars(argv [ an ], carrier) ; ++an ;
		from_chars(argv [ an ], depth) ; ++an ;
		break ;
	case GAUSSIAN :
		from_chars(argv [ an ], carrier) ; ++an ;
		from_chars(argv [ an ], depth) ; ++an ;
		break ;
	case IQ :
		from_chars(argv [ an ], carrier) ; ++an ;
		break ;
	default :
		break ;
	}
	from_chars(argv [ an ], sample_rate) ; ++an ;
	from_chars(argv [ an ], duration) ; ++an ;

	switch ( jt )
	{
	case SINE :
	case RANDOM :
	case GAUSSIAN :
		// validate
		if ( carrier < 1.0 || carrier > (F)sample_rate/2 )
		{
			fmt::println("Carrier <{}> is out of range.", carrier);
			usage () ;
			return -1 ;
		}
		if ( depth < F(0.0) || depth > F(1.0E9)) // 1x10^9ps = 1ms?
		{
			fmt::println("Jitter depth <{}> is out of range for random options.", depth);
			usage () ;
			return -1 ;
		}
		break ;
	default :
		break ;
	}

	// open output file
	log_file_t of ( argv [ an ]) ;
	if ( !of.good() )
	{
		fmt::println("Couldn't open output file <{}>.", argv[an]);
		return -1 ;
	}
	if ( jt == SINE || jt == RANDOM || jt == GAUSSIAN )
	{
		// allocate
		std::vector<F> buf ;
		buf.resize ( sample_rate * duration ) ;

		// generate
		switch ( jt )
		{
		case SINE :
			JitterBufSineDirect ( carrier, jitter, depth, &buf[0], buf.size (), sample_rate ) ;
			break ;
		case RANDOM :
			JitterBufRandom ( carrier, depth, &buf[0], buf.size (), sample_rate ) ;
			break ;
		case GAUSSIAN :
			JitterBufGaussian ( carrier, depth, &buf[0], buf.size (), sample_rate ) ;
			break ;
		default :
		// can't happen, see above
			break ;
		}

		// write
		of.write( &buf[0], buf.size () * sizeof ( F )) ;
	}
	else
	if ( jt == IQ )
	{
		// allocate
		std::vector<F> buf ;
		buf.resize ( sample_rate * duration * 2 ) ; // stereo by definition
		FillBufferWithIQ ( carrier, &buf [ 0 ], buf.size () / 2, sample_rate ) ;
		// write
		of.write ( &buf[0], buf.size () * sizeof ( F )) ;
	}
	else
	{
		switch (dt)
		{
		case I16:
		{
			std::vector<uint16_t> buf;
			buf.resize(sample_rate * duration * 2); // these are stereo by design
			switch (jt)
			{
			case JTEST:
				CreateJTest16(buf);
				break;
			case QTEST:
				CreateQTest16(buf);
				break;
			case QTEST_IQ:
				CreateQTestIQ16(buf);
				break;
			case _1BIT:
				Create1Bit16(buf, sample_rate);
				break;
			default:
				break;
			}
			// write
			of.write(&buf[0], buf.size() * sizeof(uint16_t));
		}
		break;
		case I32:
		{
			std::vector<uint32_t> buf;
			buf.resize(sample_rate * duration * 2); // these are stereo by design
			switch (jt)
			{
			case JTEST:
				CreateJTest24(buf);
				break;
			case QTEST:
				CreateQTest24(buf);
				break;
			case QTEST_IQ:
				CreateQTestIQ24(buf);
				break;
			case _1BIT:
				Create1Bit24(buf, sample_rate);
				break;
			default:
				break;	
			}
			// write
			of.write(&buf[0], buf.size() * sizeof(uint32_t));
		}
		break;
		case FLT:
		{
			std::vector<F> buf;
			buf.resize(sample_rate * duration); // these are mono by design
			switch (jt)
			{
			case JTEST:
				CreateKTestF(buf, 174);
				break;
			case QTEST:
				CreateQTestF(buf);
				break;
			case QTEST_IQ:
				CreateQTestIQF(buf);
				break;
			case _1BIT:
				Create1BitF(buf, sample_rate);
				break;
			default:		
				break;
			}
			// write
			of.write(&buf[0], buf.size() * sizeof(F));
		}
		break;
		default:
			fmt::println("Unknown type for J type test.");
			break;
		}

	}

	return 0;
}