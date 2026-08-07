#include <vector>
#include <charconv>	
#include <fmt/format.h>

#include "basic_file.h"
#include "FMDemodFunctions.h"

typedef double F ;

template <typename T> void from_chars(char const* arg, T& result)
{
	auto [ptr, ec] = std::from_chars(arg, arg + strlen(arg), result);
}

void usage ()
{
	fmt::print("Generates an FM raw PCM file");
	fmt::print("Usage - FMGenerate <carrier frequency> <modulation frequency> <deviation> <sample rate> <duration> <outputfile>");
	fmt::print("Where duration is in seconds. Output is a packed array of F.");
	fmt::print("(If modulation frequency or deviation are 0 then a pure sine is generated.)");
	fmt::print("For example - FMGenerate 3150 4.5 1 96000 30 3150_4_5.raw");
}

int main(int argc, char* argv[])
{
	if ( argc < 7 )
	{
		usage () ;
		return 1 ;
	}
	F carrier ;
	F modulation ;
	F deviation ;
	size_t sample_rate ;
	size_t duration ;

	from_chars(argv [ 1 ], carrier) ;
	from_chars(argv [ 2 ], modulation) ;
	from_chars(argv [ 3 ], deviation) ;
	from_chars(argv [ 4 ], sample_rate) ;
	from_chars(argv [ 5 ], duration) ;

	// validate
	if ( carrier < 1.0 || carrier > (F)sample_rate/2 )
	{
		fmt::print("Carrier <{}> is out of range.\n", carrier);
		usage();
		return 1;
	}

	// open output file
	log_file_t of ( argv [ 6 ]) ;
	if ( !of.good() )
	{
		fmt::print("Couldn't open output file <{}>\n", argv[6]);
		return -1;
	}

	// allocate
	std::vector<F> buf ;
	buf.resize ( sample_rate * duration ) ;

	// generate
	if ( modulation == F(0) || deviation == F(0))
	{
		FillBufferWithSine ( carrier, &buf[0], buf.size (), sample_rate ) ;
	}
	else
	{
		FillBufferWithFM ( carrier, modulation, deviation, &buf[0], buf.size (), sample_rate ) ;
	}

	// write
	of.write ( &buf[0], buf.size () * sizeof ( F )) ;

	return 0;
}

