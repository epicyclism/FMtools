#include <vector>
#include <charconv>	

#include <fmt/format.h>

#include "basic_file.h"
#include "constants.h"
#include "FMDemodFunctions.h"

template <typename T> void from_chars(char const* arg, T& result)
{
	auto [ptr, ec] = std::from_chars(arg, arg + strlen(arg), result);
}

void usage ()
{
	fmt::println("Generates a tri-sine wave in a raw file.") ;
	fmt::println("Usage -" ) ;
	fmt::println("SigGen [-S] <frequency1> <frequency2> <frequency3> <sample rate> <duration> <outputfile>") ;
	fmt::println("produces a linear combination of three sine waves at the three frequencies.") ;
	fmt::println("For these options duration is in seconds and the output is a packed array of F.") ;
	fmt::println("(sizeof F is {})" , sizeof(F)) ;
}

int main(int argc, char* argv[])
{
	if ( argc < 7 )
	{
		usage() ;
		return -1 ;
	}
	F f1 = F(0) ;
	F f2 = F(0) ;
	F f3 = F(0) ;
	size_t sample_rate = 0 ;
	size_t duration    = 0 ;
	size_t an = 1 ;

	if ( argv [ 1 ][ 0 ] == '-' || argv [ 1 ][ 0 ] == '/')
	{
		switch ( argv [ 1 ][ 1 ])
		{
		case 'S' :
		case 's' :
			break ;
		default :
			usage () ;
			return -1 ;
		}
		an = 2 ;
	}
	from_chars(argv [ an ], f1) ; ++an ;
	from_chars(argv [ an ], f2) ; ++an ;
	from_chars(argv [ an ], f3) ; ++an ;
	from_chars(argv [ an ], sample_rate) ; ++an ;
	from_chars(argv [ an ], duration) ; ++an ;

	// open output file
	log_file_t of ( argv [ an ]) ;
	if ( !of.good() )
	{
		fmt::println("Couldn't open output file <{}>", argv [ 6 ]) ;
		return -1 ;
	}

	// allocate
	std::vector<F> buf ;
	buf.resize ( sample_rate * duration ) ;

	// generate
	FillBufferWithTriSine(f1, f2, f3, &buf[0], buf.size(), sample_rate);

	// write
	of.write ( &buf[0], buf.size () * sizeof ( F )) ;

	return 0;
}
