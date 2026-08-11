#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

#include "mm_file.h"

/*
Positions	        Sample Value	      Description
1 - 4 (4byte)	    "RIFF"	              Marks the file as a riff file. Characters are each 1 byte long.                               -
5 - 8 (4byte)	    File size (integer)	  Size of the overall file - in bytes (32-bit integer).                                         |	RIFF Chunk Descriptor
9 -12 (4byte)	    "WAVE"	              File Type Header.                                                                             -
13-16 (4byte)	    "fmt "	              Format chunk marker. Includes trailing null                                                   -
17-20 (4byte)	    16	                  Length of format data as listed above                                                         |
21-22 (2byte)	    1	                  Type of format (1 is PCM) - 2 byte integer                                                    |
23-24 (2byte)	    2	                  Number of Channels - 2 byte integer                                                           |	fmt Sub Chunk
25-28 (4byte)	    44100	              Sample Rate - 32 bit integer. Common values are 44100 (CD), 48000 (DAT).                      |
29-32 (4byte)	    176400	              (Sample Rate * BitsPerSample * Channels) / 8. -> Bytes per second                             |
33-34 (2byte)	    4	                  (BitsPerSample * Channels) / 8.1 - 8 bit mono2 - 8 bit stereo/16 bit mono4 - 16 bit stereo    |
35-36 (2byte)	    16	                  Bits per sample                                                                               -
37-40 (4byte)	    "data"	              "data" chunk header. Marks the beginning of the data section.                                 -
41-44 (4byte)	    File size (data)	  Size of the data section. -> num bytes of data + 4 bytes of chunk                             |	Data Sub Chunk
45->									  Data		                                                                                    -
*/

struct WAV_HEADER {
	char RIFF[4];
	uint32_t fSize;
	char WAVE[4];

	char fmt[4];
	uint32_t fmtChunkLength;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;

	char data[4];
	uint32_t dataChunkSize;
};


class Wav_File {
public:

	mem_map_file<uint8_t> mmf;
	WAV_HEADER wav_header;

	Wav_File(const char* relFPath);
	void print_header_info();

	//No channel given, convert to mono
	std::pair<float const*, int> get_chunk(int numMS);
	
	/*
	Channel:
		0 - 1st channel (left?)
		1 - 2nd channel (right?)
		.
		.
		.
	*/
	std::pair<float const*, int> get_chunk(int numMS, const int channel);
	
	//Allow choice of specific channels to be converted to mono
	std::pair<float const*, int> get_chunk(int numMS, const std::vector<int> channels);
	
	uint32_t sample_rate() const
	{
		return wav_header.sampleRate;
	}
	uint32_t total_samples() const
	{
		return totalNumSamples;
	}
	bool eof();

private:
	std::vector<float> chunk;
	int totalNumSamples;
	int currentSample;
	int dataoffset = 0;

	template <typename T>
	T read_bytes(uint8_t const* in, int& offset);
	WAV_HEADER initialise_wav_header();
	
	float read_normalise_value(uint8_t const* pt);

};