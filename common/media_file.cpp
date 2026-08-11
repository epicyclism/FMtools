
#include "media_file.h"

Media_File::Media_File(const char* relFPath, uint32_t offsetIntoDataMS_, uint32_t endPointInDataMS_, int resampleRate_, int defaultBufferLoadMS_)
    :offsetIntoDataMS(offsetIntoDataMS_),
    endPointInDataMS(endPointInDataMS_),
    resampleRate(resampleRate_),
    defaultBufferLoadMS(defaultBufferLoadMS_)
{
    if(pFormatContext) // already open
        throw mediaFileException(2, "MEDIAFILE STRUCTURE ALREADY INITIALIZED");

    if (this->offsetIntoDataMS > this->endPointInDataMS) {
        std::string err = ("Attempting to start after chosen end of file: (in = " + std::to_string(this->offsetIntoDataMS) + "ms, out = " + std::to_string(this->endPointInDataMS) + "ms)\n");
        throw mediaFileException(1, err.c_str());
    }

	//Create context
	this->pFormatContext = avformat_alloc_context();

    //Set ffmpeg to only output for errors
    av_log_set_level(AV_LOG_ERROR);
    //Set ffmpeg to not log anything
    //av_log_set_level(AV_LOG_QUIET);

	// Attempt to open media file
    int openInputRet = avformat_open_input(&this->pFormatContext, relFPath, NULL, NULL);
	if (openInputRet != 0) {
        throw ffmpegException(openInputRet, "CANNOT OPEN FILE\n");
        //throw std::exception("CANNOT OPEN FILE\n");
	}

	// Get file stream information
    int findStreamRet = avformat_find_stream_info(this->pFormatContext, NULL);
	if (findStreamRet < 0) {
        throw ffmpegException(findStreamRet, "CANNOT FIND STREAM INFORMATION\n");
        //throw std::exception("CANNOT FIND STREAM INFORMATION\n");
	}

    // Loop over streams
    for (unsigned int i = 0; i < this->pFormatContext->nb_streams; i++) {
        AVCodecParameters* pLocalCodecParameters = NULL;
        pLocalCodecParameters = this->pFormatContext->streams[i]->codecpar;
        const AVCodec* pLocalCodec = NULL;

        // Find suitable decoder for stream
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

        //Check if data is audio, get codec info
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (this->audio_stream_index == -1) {
                this->audio_stream_index = i;
                this->pCodec = pLocalCodec;
                this->pCodecParameters = pLocalCodecParameters;
            }
        }
    }

    //Create codec context and set to default values
    this->pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        throw ffmpegException(0, "COULD NOT ALLOCATE MEMORY FOR AVCODECCONTEXT\n");
        //throw std::exception("COULD NOT ALLOCATE MEMORY FOR AVCODECCONTEXT\n");
    }

    // Fill codec context with codec parameters
    int codecParamToCtxRet = avcodec_parameters_to_context(pCodecContext, pCodecParameters);
    if (codecParamToCtxRet < 0) {
        throw ffmpegException(codecParamToCtxRet, "COULD NOT SETUP AVCODECCONTEXT\n");
        //throw std::exception("COULD NOT SETUP AVCODECCONTEXT\n");
    }

    // Initialise AVCodecContext to use the given AVCodec
    int openCodecRet = avcodec_open2(pCodecContext, pCodec, NULL);
    if (openCodecRet < 0) {
        throw ffmpegException(openCodecRet, "COULD NOT OPEN CODEC\n");
        //throw std::exception("COULD NOT OPEN CODEC\n");
    }

    // Allocate an AVPacket and set values to defaults
    this->pPacket = av_packet_alloc();
    if (!pPacket) {
        throw ffmpegException(0, "COULD NOT ALLOCATE MEMORY FOR AVPACKET\n");
        //throw std::exception("COULD NOT ALLOCATE MEMORY FOR AVPACKET\n");
    }

    // Allocate an AVFrame and set values to defaults
    this->pFrame = av_frame_alloc();
    if (!pFrame) {
        throw ffmpegException(0, "COULD NOT ALLOCATE MEMORY FOR AVFRAME\n");
        //throw std::exception("COULD NOT ALLOCATE MEMORY FOR AVFRAME\n");
    }

    // Allocate Software Resampler
    this->swr = swr_alloc();
    if (!swr) {
        throw ffmpegException(0, "COULD NOT ALLOCATE RESAMPLER CONTEXT\n");
        //throw std::exception("COULD NOT ALLOCATE RESAMPLER CONTEXT\n");
    }

    // Set resampler options
//    av_opt_set_int(swr, "in_channel_layout", av_get_default_channel_layout(pCodecContext->ch_layout.nb_channels), NULL); // Set input channel layout to correct layout (deprecated, should use av_channel_layout_default()?)
    AVChannelLayout src ;
    av_channel_layout_default(&src, pCodecContext->ch_layout.nb_channels);
    av_opt_set_chlayout(swr, "ichl", &src, NULL);                                      // Set input channel layout to correct layout(?)
    AVChannelLayout dst = AV_CHANNEL_LAYOUT_MONO;
    av_opt_set_chlayout(swr, "ochl", &dst, NULL);                                      // Set output channel layout to correct layout(?)
    av_opt_set_int(swr, "in_sample_rate", pCodecContext->sample_rate, NULL);                                             // Set input sample rate
    av_opt_set_int(swr, "out_sample_rate", this->resampleRate, NULL);                                                    // Resample audio to 16kHz
    av_opt_set_sample_fmt(swr, "in_sample_fmt", pCodecContext->sample_fmt, NULL);                                        // Set input sample format
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLT, NULL);                                               // Set output sample format to 32-bit floating point (float)

    // Initialise the software resampler
    int swrInitRet = swr_init(swr);
    if (swrInitRet < 0) {
        throw ffmpegException(swrInitRet, "COULD NOT INITIALISE THE RESAMPLER CONTEXT\n");
        //throw std::exception("COULD NOT INITIALISE THE RESAMPLER CONTEXT\n");
    }


    this->number_of_samples = this->pCodecContext->sample_rate;
    // Allocate buffer to hold converted audio
    int allocateBufferRet = av_samples_alloc(&buffer, &buffer_linesize, pCodecContext->ch_layout.nb_channels, number_of_samples, AV_SAMPLE_FMT_FLT, 0);
    if (allocateBufferRet < 0) {
        throw ffmpegException(allocateBufferRet, "COULD NOT ALLOCATE CONVERTED INPUT SAMPLE BUFFERS\n");
        //throw std::exception("COULD NOT ALLOCATE CONVERTED INPUT SAMPLE BUFFERS\n");
    }

    //std::cout << "1: TSC: " << this->totalSamplesConverted << "  NSW: " << this->numSamplesWanted << '\n';

    if (offsetIntoDataMS) {
        auto _ = this->get_chunk(offsetIntoDataMS, false);
    }

    //std::cout << "2: TSC: " << this->totalSamplesConverted << "  NSW: " << this->numSamplesWanted << '\n';

    if (endPointInDataMS != UINT32_MAX) {
        this->numSamplesWanted = ((this->endPointInDataMS - this->offsetIntoDataMS) / 1000) * this->resampleRate;
    }

    if (this->eof()) {
        std::string err = "Offset into audio file after end of file (in = " + std::to_string(this->offsetIntoDataMS) + ")\n";
        throw mediaFileException(2, err.c_str());
    }

    //std::cout << "3: TSC: " << this->totalSamplesConverted << "  NSW: " << this->numSamplesWanted << '\n';

}

Media_File::~Media_File(){
    av_freep(&this->buffer);
    swr_free(&this->swr);
    av_frame_free(&this->pFrame);
    av_packet_free(&this->pPacket);
    avcodec_free_context(&this->pCodecContext);
    avformat_close_input(&this->pFormatContext);
}

/*TODO
* EDGE CASES:
* - Get to end of audio file
* - Probably more
* */
std::pair<float const*, int> Media_File::get_chunk(int numMS, bool includeSamples){
    
    //std::cout << "1.1: TSC: " << this->totalSamplesConverted << "  NSW: " << this->numSamplesWanted << '\n';

    if (numMS > this->defaultBufferLoadMS) {
        return this->get_large_chunk(numMS, includeSamples);
    }

//    std::ofstream f("out.bin", std::ios::binary | std::ios::app);
    
    int numSamples = this->resampleRate * (numMS / 1000.0);

    // If sufficient samples have been extracted
    if (!this->extractedData.size() || numSamples > this->extractedData.size() - this->nextDataIdx) {
        //Clear out chunk
        this->chunk.clear();
        // Move data into chunk
        this->chunk = this->extractedData;
        //Empty extracted data
        this->extractedData.clear();
        //Fill extracted data with unused data from previous extractedData
        this->extractedData.insert(this->extractedData.end(), this->chunk.begin() + this->nextDataIdx, this->chunk.end());
        // Reset newData index
        this->nextDataIdx = 0;

        // Fill up extractedData
        this->fill_extracted_data();
    }

    //Return next chunk of data
    std::pair<float const*, int> data{ this->extractedData.data() + this->nextDataIdx, numSamples };
    this->nextDataIdx += numSamples;

    if (includeSamples) {
        this->totalSamplesConverted += numSamples;
//        f.write(reinterpret_cast<const char*>(data.first), numSamples);
    }

    return data;
}

int64_t Media_File::estimated_duration_ms() const
{
    if(!pFormatContext)
        return 0;
    int64_t dur = pFormatContext->duration * 1000 / AV_TIME_BASE;
    return dur;
}

std::pair<float const*, int> Media_File::get_large_chunk(int numMS, bool includeSamples){
    //std::cout << "1.2: TSC: " << this->totalSamplesConverted << "  NSW: " << this->numSamplesWanted << '\n';
    this->largeChunk.clear();
    int numSamples = this->resampleRate * (numMS / 1000.0);
    int numLeft = numSamples;
    while (!this->eof() && this->largeChunk.size() < numSamples) {
        auto smallChunkData = this->get_chunk(((numLeft > (this->defaultBufferLoadMS / 1000) * this->resampleRate) ? this->defaultBufferLoadMS : (numLeft * 1000/this->resampleRate)), includeSamples);
        if (smallChunkData.second == 0)
            break;
        this->largeChunk.insert(this->largeChunk.end(), smallChunkData.first, smallChunkData.first + smallChunkData.second);
        numLeft -= smallChunkData.second;
    }

    if (includeSamples) {
        this->totalSamplesConverted += numSamples - numLeft;
    }

    return { this->largeChunk.data(), numSamples - numLeft};
}

bool Media_File::eof(){
    //std::cout << "TSC: " << this->totalSamplesConverted << "  NSW: " << this->numSamplesWanted << '\n';
    return (this->atEndOfData && (this->nextDataIdx >= this->extractedData.size())) || (this->totalSamplesConverted >= this->numSamplesWanted);
}

void Media_File::fill_extracted_data(){
    if (!this->eof()) {
        int numSamplesWanted = this->resampleRate * (this->defaultBufferLoadMS / 1000) - this->extractedData.size();
        int numSamplesConverted = 0;
        //this->extractedData.clear();
        int readFrameRet = 0;


        //std::ofstream file("out" + std::to_string((this->fNum++)) + ".bin", std::ios::binary);
        this->fNum++;
        while ((numSamplesConverted <= numSamplesWanted) && (readFrameRet = av_read_frame(this->pFormatContext, this->pPacket), readFrameRet >= 0)) {
            if (this->pPacket->stream_index == this->audio_stream_index) {
                int sendPacketRet = avcodec_send_packet(this->pCodecContext, pPacket);
                if (sendPacketRet < 0) {
                    throw ffmpegException(sendPacketRet, "ERROR WHILE SENDING A PACKET TO THE DECODER\n");
                    //throw std::exception("ERROR WHILE SENDING A PACKET TO THE DECODER\n");
                }

                while (avcodec_receive_frame(this->pCodecContext, this->pFrame) == 0) {
                    // Convert audio to float
                    int n = swr_convert(this->swr, &this->buffer, this->pFrame->nb_samples, (const uint8_t**)this->pFrame->data, this->pFrame->nb_samples);
                    if (n) {
                        float* fpBuff = (float*)this->buffer;
                        this->extractedData.insert(this->extractedData.end(), fpBuff, fpBuff + n);
                        //std::cout << n << '\n';
                        //file.write(reinterpret_cast<const char*>(&this->extractedData[this->extractedData.size() - n]), sizeof(float) * n);
                        numSamplesConverted += n;
                    }
                }
            }
            av_packet_unref(pPacket);
        }

        if ((numSamplesConverted <= numSamplesWanted) && readFrameRet) {
            this->atEndOfData = true;
        }
    }
    
}

