//
// Copyright (c) 2026 Paul Ranson, paul@epicyclism.com
//
//

#include <iostream>
#include <string>
#include <string_view>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <charconv>
#include <tuple>
#include <vector>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "mm_file.h"
#include "basic_file.h"
#include "audio_file_reader.h"
#include "fm2_proc.h"

void welcome ()
{
	fmt::println(std::cerr, "fm2 Copyright Paul Ranson (c) 2026") ;
	fmt::println(std::cerr, "email - paul@epicyclism.com\n") ;
}

void Usage ()
{
	fmt::println(std::cerr, "Usage : fm2 [-L|R|M] <inputwav>") ;
	fmt::println(std::cerr, "Options : -L, use left. -R, use right. -M, convert to mono.") ;
	fmt::println(std::cerr, "          (These are all ignored if the input is mono...)") ;
}

int main ( int argc, char **argv )
{
    welcome () ;
    if( argc < 2 )
    {
        Usage () ;
        return -1 ;
    }

    int32_t sample_rate = -1;
    F rpm = thirty_three; // default for 33rpm
    int		nInputProcFlags = 0 ; // 0 = make mono, 1 = left, 2 = right
    int		arg  = 1 ;
    while ( arg < argc )
    {
#if defined(_MSC_VER)
		if ( argv [ arg ][ 0 ] == '-' || argv [ arg ][ 0 ] == '/')
#else
		if ( argv [ arg ][ 0 ] == '-')
#endif
		{
			switch ( argv [ arg ][ 1 ])
			{
            case 'L' :
            case 'l' :
                nInputProcFlags = 1 ;
                break ;
            case 'M' :
            case 'm' :
                nInputProcFlags = 0 ;
                break ;
            case 'R' :
            case 'r' :
                nInputProcFlags = 2 ;
                break ;
            default :
                fmt::println(std::cerr, "Unknown argument \'{}\'!", argv [ arg ][ 1 ]) ;
                Usage () ;
                return -1 ;
            }
        }
        else
        {
            break ;
        }
        ++arg ;
    }
    std::vector<F> audio_buf ;
    std::tie(audio_buf, sample_rate) = read_audio_file(argv[arg], nInputProcFlags);
	if (audio_buf.empty())
	{
		fmt::println(std::cerr, "Failed to read audio file <{}>, error code {}", argv[arg], sample_rate);
		return -1;
	}
    fmt::println("Read {} samples from input file. Sample rate is {}.", audio_buf.size (), sample_rate) ;

    fm2_proc_cfg cfg;
    cfg.sample_rate = sample_rate;
    cfg.rpm = rpm;
    fm2_proc proc;
    proc.process(cfg, audio_buf);
    fmt::println("Frequency detected: {} Hz", proc.frequency_);
}