//
// Copyright (c) 2026 Paul Ranson, paul@epicyclism.com
//
//

#include "miniaudio.h"
#include "audio_file_reader.h"
#include "constants.h"

void monoize(std::vector<F>& buf)
{
    if (buf.size() < 2)
        return;
    auto i0 = buf.begin();
    auto i1 = i0;
    auto ie = buf.end();
    while (i1 != ie)
    {
        *i0 = *i1 + *(i1 + 1) / TWO;
        ++i0;
        i1 += 2;
    }
    buf.resize(buf.size() / 2);
}

void keep_left(std::vector<F>& buf)
{
    if (buf.size() < 2)
        return;
    auto i0 = buf.begin();
    auto i1 = i0;
    auto ie = buf.end();
    while (i1 != ie)
    {
        *i0 = *i1;
        ++i0;
        i1 += 2;
    }
    buf.resize(buf.size() / 2);
}

void keep_right(std::vector<F>& buf)
{
    if (buf.size() < 2)
        return;
    auto i0 = buf.begin();
    auto i2 = i0 + 1;
	auto ie = buf.end() - 1;
    do
    {
        *i0 = *i2;
        ++i0;
        i2 += 2;
    }
    while (i2 != ie);
    buf.resize(buf.size() / 2);
}

std::pair<std::vector<F>, uint32_t> read_audio_file(const char* filename, uint32_t flags)
{
    std::vector<F> buf;
    uint32_t       sr;

    ma_decoder decoder;
    ma_result result;
    ma_decoder_config cfg = ma_decoder_config_init_default();
	cfg.format = ma_format_f32; // Request float output
    // Initialize decoder (auto-detect format from file extension)
    result = ma_decoder_init_file(filename, &cfg, &decoder);
    if (result != MA_SUCCESS)
        return { buf, result };

    // Get total frame count
    ma_uint64 frameCount;
    result = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (result != MA_SUCCESS)
    {
        ma_decoder_uninit(&decoder);
        return { buf, result };
    }

    // Allocate buffer for all samples (interleaved)
    buf.resize((size_t)(frameCount* decoder.outputChannels));

    // Read PCM frames into buffer
    ma_uint64 framesRead;
    result = ma_decoder_read_pcm_frames(&decoder, buf.data(), frameCount, &framesRead);
    if (framesRead != frameCount)
    {
        buf.clear();
        ma_decoder_uninit(&decoder);
        return { buf, result };
    }

    if (decoder.outputChannels > 1)
    {
        switch (flags)
        {
        case 0:
            // make mono
            monoize(buf);
            break;
        case 1:
            // keep left
            keep_left(buf);
            break;
        case 2:
            // keep right
            keep_right(buf);
            break;
        default:
            // can't happen
            break;
        }
    }

    sr = decoder.outputSampleRate;
    ma_decoder_uninit(&decoder);
    return { buf, sr };
}