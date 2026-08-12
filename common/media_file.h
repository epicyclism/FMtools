#pragma once

#include <iostream>
#include <fstream>
#include <vector>

#include "exception_sys_err.h"

//FFMPEG includes
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

using ffmpegException = exceptionError<int>;
using mediaFileException = exceptionError<int>;


/*
* TODO
* - TODOS IN getChunk() method
* - More thorough testing
*	- Create equivalent wav and mp3 files and compare text output
* - Video file input and audio extraction
* 
* 
* */


class Media_File {
public:
	/*
	relFPath				Relative path to media file
	*/
//	Media_File(const char* relFPath, uint32_t offsetIntoDataMS_ = 0, uint32_t endPointInDataMS_ = UINT32_MAX, int resampleRate_ = 16000, int defaultBufferLoadMS_ = 30000);
	Media_File(const char* relFPath);

	~Media_File();

	int64_t estimated_duration_ms() const;
	int		get_sample_rate() const { return pCodecContext->sample_rate; }
	std::pair<float const*, int> get_chunk(int numMS, bool includeSamples = true);

	bool eof();

	int fNum = 0;
private:
	int defaultBufferLoadMS = -1;

	uint32_t numSamplesWanted = UINT32_MAX;
	uint32_t totalSamplesConverted = 0;

	std::vector<float> extractedData;
	int nextDataIdx = 0;
	std::vector<float> chunk;

	std::vector<float> largeChunk;


	AVFormatContext* pFormatContext = nullptr;

	const AVCodec* pCodec = nullptr;
	AVCodecParameters* pCodecParameters = nullptr;
	int audio_stream_index = -1;

	AVCodecContext* pCodecContext = nullptr;

	AVPacket* pPacket = nullptr;

	AVFrame* pFrame = nullptr;

	SwrContext* swr = nullptr;

	uint8_t* buffer = nullptr;
	int buffer_linesize = -1;
	int number_of_samples = -1;

	bool atEndOfData = false;

	int totalMS = 0;

	std::pair<float const*, int> get_large_chunk(int numMS, bool includeSamples = true);

	void fill_extracted_data();

};