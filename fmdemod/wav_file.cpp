#include <cmath>
#include <cstring>
#include "wav_file.h"

#define INT8_MAX_FLT 255.0
#define INT16_MAX_FLT 32767.0
#define INT32_MAX_FLT 2147483648.0
#define INT64_MAX_FLT 9223372036854775807.0

Wav_File::Wav_File(const char* relFPath) : mmf(relFPath)
{
	//*this->wav_header = reinterpret_cast<WAV_HEADER*>(*this->mmf.ptr());
	std::memcpy(&wav_header, mmf.ptr(), sizeof(wav_header));
	//this->wav_header = this->initialise_wav_header();
	this->totalNumSamples = (this->wav_header.dataChunkSize - 4) / ((this->wav_header.bitsPerSample / 8) * this->wav_header.numChannels);
	this->currentSample = 0;
}

void Wav_File::print_header_info()
{
	std::cout << "FILE TYPE: " << this->wav_header.RIFF[0]
		<< this->wav_header.RIFF[1]
		<< this->wav_header.RIFF[2]
		<< this->wav_header.RIFF[3] << '\n';
	std::cout << "FILE SIZE: " << this->wav_header.fSize << '\n';
	std::cout<<"WAVE: " << this->wav_header.WAVE[0]
		<< this->wav_header.WAVE[1]
		<< this->wav_header.WAVE[2]
		<< this->wav_header.WAVE[3] << '\n';
	std::cout << "fmt: " << this->wav_header.fmt[0]
		<< this->wav_header.fmt[1]
		<< this->wav_header.fmt[2]
		<< this->wav_header.fmt[3] << '\n';
	std::cout << "FMT CHUNK LENGTH: " << this->wav_header.fmtChunkLength << '\n';
	std::cout << "AUDIO FORMAT: " << this->wav_header.audioFormat << '\n';
	std::cout << "NUMBER CHANNELS: " << this->wav_header.numChannels << '\n';
	std::cout << "SAMPLE RATE: " << this->wav_header.sampleRate << '\n';
	std::cout << "BYTE RATE: " << this->wav_header.byteRate << '\n';
	std::cout << "BLOCK ALIGN: " << this->wav_header.blockAlign << '\n';
	std::cout << "BITS PER SAMPLE: " << this->wav_header.bitsPerSample << '\n';
	std::cout << "DATA: " << this->wav_header.data[0]
		<< this->wav_header.data[1]
		<< this->wav_header.data[2]
		<< this->wav_header.data[3] << '\n';
	std::cout << "DATA CHUNK SIZE: " << this->wav_header.dataChunkSize << '\n';
}

//Convert to mono with all channels included
std::pair<float const*, int> Wav_File::get_chunk(int numMS)
{
	//Clear chunk buffer
	this->chunk.clear();
	//Calculate number of samples, check not to read off of end of audio data
	int ns = this->wav_header.sampleRate * numMS / 1000;
	if (ns + this->currentSample > this->totalNumSamples) {
		ns = this->totalNumSamples - this->currentSample;
	}

	uint8_t const* pt = reinterpret_cast<uint8_t const*>(this->mmf.ptr());
	
	for (int i = this->currentSample; i < this->currentSample + ns; i++) {
		float sum = 0.0f;
		for (int j = 0; j < this->wav_header.numChannels; j++) {
			sum += read_normalise_value(pt);
		}
		this->chunk.push_back(sum / this->wav_header.numChannels);
	}

	this->currentSample += ns;

	return std::make_pair(&this->chunk[0], ns);
}

//Use single channel of choice
std::pair<float const*, int> Wav_File::get_chunk(int numMS, const int channel)
{
	//Return if invalid channel chosen
	if (channel < 0 || channel >= this->wav_header.numChannels) { return { nullptr, -1 }; }
	
	//Clear chunk buffer
	this->chunk.clear();
	//Calculate number of samples, check not to read off of end of audio data
	int ns = this->wav_header.sampleRate * numMS / 1000;
	if (ns + this->currentSample > this->totalNumSamples) {
		ns = this->totalNumSamples - this->currentSample;
	}

	uint8_t const* pt = reinterpret_cast<uint8_t const*>(this->mmf.ptr());

	for (int i = this->currentSample; i < this->currentSample + ns; i++) {
		float sum = 0.0f;
		for (int j = 0; j < this->wav_header.numChannels; j++) {
			float val = read_normalise_value(pt);
			if (j == channel) { sum += val; }
		}
		this->chunk.push_back(sum / this->wav_header.numChannels);
	}

	this->currentSample += ns;

	return std::make_pair(&this->chunk[0], ns);
}

//Use multiple channels
std::pair<float const*, int> Wav_File::get_chunk(int numMS, const std::vector<int> channels)
{
	//Return if invalid channel chosen
	for (auto c : channels) {
		if (c < 0 || c >= this->wav_header.numChannels) { return { nullptr, -1 }; }
	}

	//If empty vector sent, assume convert to mono using all channels
	if (channels.size() == 0) { return this->get_chunk(numMS); }

	//Clear chunk buffer
	this->chunk.clear();
	//Calculate number of samples, check not to read off of end of audio data
	int ns = this->wav_header.sampleRate * numMS / 1000;
	if (ns + this->currentSample > this->totalNumSamples) {
		ns = this->totalNumSamples - this->currentSample;
	}

	uint8_t const* pt = reinterpret_cast<uint8_t const*>(this->mmf.ptr());

	for (int i = this->currentSample; i < this->currentSample + ns; i++) {
		float sum = 0.0f;
		int nc = 0; //Manually count channels in case one channel selected twice so length would not work
		for (int j = 0; j < this->wav_header.numChannels; j++) {
			float val = read_normalise_value(pt);
			if (std::find(channels.begin(), channels.end(), j) != channels.end()) { sum += val; nc++; }
			//if (j == channel) { sum += val; }
		}
		this->chunk.push_back(sum / nc);
	}

	this->currentSample += ns;

	return std::make_pair(&this->chunk[0], ns);
}

bool Wav_File::eof()
{
	return this->currentSample >= this->totalNumSamples;
}

template <typename T>
T Wav_File::read_bytes(uint8_t const* in, int& offset) {
	auto ret = (T*)(in + offset);
	offset += sizeof(T);
	return *ret;
}

WAV_HEADER Wav_File::initialise_wav_header()
{
	uint8_t const* pt = reinterpret_cast<uint8_t const*>(this->mmf.ptr());
	this->dataoffset = 0;
	WAV_HEADER wh{
		{this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset)},  // RIFF
		this->read_bytes<uint32_t>(pt, this->dataoffset),                                                                                                                                          //File Size
		{this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset)},  //WAVE
		{this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset)},  //FMT
		this->read_bytes<uint32_t>(pt, this->dataoffset),                                                                                                                                          //fmt Chunk Length
		this->read_bytes<uint16_t>(pt, this->dataoffset),                                                                                                                                          //Audio Format
		this->read_bytes<uint16_t>(pt, this->dataoffset),                                                                                                                                          //Number Channels
		this->read_bytes<uint32_t>(pt, this->dataoffset),                                                                                                                                          //Sample Rate
		this->read_bytes<uint32_t>(pt, this->dataoffset),                                                                                                                                          //Byte Rate
		this->read_bytes<uint16_t>(pt, this->dataoffset),                                                                                                                                          //Block Align
		this->read_bytes<uint16_t>(pt, this->dataoffset),                                                                                                                                          //Bits Per Sample
		{this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset), this->read_bytes<char>(pt, this->dataoffset)},  //data
		this->read_bytes<uint32_t>(pt, this->dataoffset)                                                                                                                                           //Data Chunk Size

	};
	return wh;
}

float Wav_File::read_normalise_value(uint8_t const* pt)
{
	switch (this->wav_header.bitsPerSample) {
		case 8: {
			//[0, 255]
			uint8_t avg = std::round(INT8_MAX_FLT / 2);
			return (static_cast<float>(this->read_bytes<uint8_t>(pt, this->dataoffset)) / avg) - 1.0;
		}break;

		case 16: {
			//[-32767, 32767]
			return this->read_bytes<int16_t>(pt, this->dataoffset) / INT16_MAX_FLT;
		}break;

		case 32: {
			//[-2147483648, 2147483648]
			return this->read_bytes<int32_t>(pt, this->dataoffset) / INT32_MAX_FLT;
		}break;

		case 64: {
			//[-9223372036854775807, 9223372036854775807]
			return this->read_bytes<int64_t>(pt, this->dataoffset) / INT64_MAX_FLT;
		}break;

		default:{
			std::cout << "UNKNOWN BITS PER SAMPLE: " << this->wav_header.bitsPerSample << '\n';
			return 0.0;
		}break;
	}
}
