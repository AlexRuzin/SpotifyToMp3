#pragma once

#include <iostream>
#include <Windows.h>
#include <string>
#include <signal.h> 
#include <fstream>
#include <vector>

#include <lame/lame.h>
#include <portaudio.h>

// Default parameters
const int lameEncoderQuality = 0;

// Prototyping
static void debug(const char* d...);

class mp3Encoder {
public:
	int sampleRate;
	int channels;
	int bitrate;
	std::string fileName;

	std::ofstream onf;

private:
	std::vector<char> buf;
	lame_global_flags* lameFlags;

public:
	mp3Encoder(int sampleRate, int channels, int bitrate, std::string fileName) :
		onf(fileName.c_str(), std::ios::binary)
	{
		std::ifstream f(fileName.c_str());
		if (f.good()) {
			f.close();
			const int fResult = remove(fileName.c_str());
			if (!fResult) {
				std::cout << "[+] Failed to remove file: " + fileName << std::endl;
				ExitProcess(1);
			}
			std::cout << "[+] Cleaned file: " + fileName << std::endl;
		}

		sampleRate = sampleRate;
		channels = channels;
		bitrate = bitrate;
		fileName = fileName;

		buf.resize(1024 * 128);
		lameFlags = lame_init();
		if (!lame_init()) {
			ExitProcess(-1);
		}

		lame_set_in_samplerate(lameFlags, sampleRate);
		lame_set_brate(lameFlags, bitrate);
		lame_set_num_channels(lameFlags, channels);
		lame_set_out_samplerate(lameFlags, sampleRate);

		lame_init_params(this->lameFlags);
	}

	static int paCallback_i32(const void* input, void* output,
		unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags, void* userData)
	{
		mp3Encoder* encPtr = (mp3Encoder*)userData;
		const int32_t** pc = (const int32_t**)input;
		encPtr->encode_sep(pc[0], pc[1], frameCount);

		return 0;
	}

	void encode_inter(const  short* s, int samples)
	{
		int n = lame_encode_buffer_interleaved(lameFlags, (short int*)s, samples, (unsigned char*)&buf[0], buf.size());
		onf.write(&buf[0], n);
	}

	void encode_sep(const int32_t* L, const int32_t* R, int samples)
	{
		int n = lame_encode_buffer_int(lameFlags, (int32_t*)L, (int32_t*)R, samples, (unsigned char*)&buf[0], buf.size());
		onf.write(&buf[0], n);
	}

	void encode_sep(const int16_t* L, const int16_t* R, int samples)
	{
		int n = lame_encode_buffer(lameFlags, (int16_t*)L, (int16_t*)R, samples, (unsigned char*)&buf[0], buf.size());
		onf.write(&buf[0], n);
	}

	int writeId3(
		std::string artistName,
		std::string trackName,
		std::string albumName,
		unsigned int year
	)
	{
		id3tag_init(lameFlags);
		id3tag_set_title(lameFlags, trackName.c_str());
		id3tag_set_artist(lameFlags, artistName.c_str());
		id3tag_set_album(lameFlags, albumName.c_str());
		id3tag_set_year(lameFlags, std::to_string(year).c_str());
		id3tag_set_comment(lameFlags, "https://github.com/AlexRuzin/SpotifyToMp3");

		return 0;
	}

	void flush()
	{
		int n = lame_encode_flush(lameFlags, (unsigned char*)&buf[0], buf.size());
		onf.write(&buf[0], n);
	}
};

class recordToMp3 {
public:
	std::string filename;

	std::string deviceName;
	int targetDevice;
	double streamRate;

	PaDeviceInfo* device;
	PaStream* pAStream;
	PaStreamParameters* streamParams;

	mp3Encoder* encoder;

public:
	recordToMp3(std::string filename) :
		targetDevice(0),
		pAStream(NULL),
		filename(filename)
	{
		if (filename == "") {
			debug("[!] filename cannot be NULL");
			ExitProcess(1);
		}
		filename = filename;
	}

	~recordToMp3(void)
	{
		Pa_CloseStream(this->pAStream);
	}

	int selectPrimaryDevice(std::string deviceName)
	{
		this->deviceName = deviceName;
		const int deviceCount = Pa_GetDeviceCount();
		for (int i = 0; i < deviceCount; i++) {
			if (std::string(Pa_GetDeviceInfo(i)->name).find(deviceName) != std::string::npos) {
				this->device = const_cast<PaDeviceInfo*>(Pa_GetDeviceInfo(i));
					std::cout << "[+] Using device: " << this->device << " Name: " << this->device->name << std::endl;
				this->targetDevice = i;
				break;
			}
			//std::cout << "[+] Skipping device" << Pa_GetDeviceInfo(i)->name << std::endl;
		}

		this->streamRate = this->device->defaultSampleRate;
		std::cout << "[+] Default device sample rate: " + std::to_string(device->defaultSampleRate) << std::endl;
		Sleep(3000);

		return 0;
	}

	int openIntoStream(void)
	{
		this->streamParams = new(PaStreamParameters);
		streamParams->channelCount = this->device->maxInputChannels;
		streamParams->device = targetDevice;
		streamParams->hostApiSpecificStreamInfo = NULL;
		streamParams->sampleFormat = paInt32 | paNonInterleaved;
		streamParams->suggestedLatency = this->device->defaultLowInputLatency;

		this->encoder = new mp3Encoder(48000, streamParams->channelCount, 320000, this->filename);

		int err = Pa_OpenStream(&this->pAStream,
			streamParams,
			NULL,
			this->streamRate,
			paFramesPerBufferUnspecified,
			paNoFlag,
			this->encoder->paCallback_i32,
			(void *)this->encoder);
		if (err) {
			debug("[!] Error opening PA stream");
			ExitProcess(0);
		}
		err = Pa_StartStream(this->pAStream);
		if (err) {
			debug("[!] Error in starting PA stream");
			ExitProcess(0);
		}

		return 0;
	}

	int writeId3(
		std::string artistName,
		std::string trackName,
		std::string albumName,
		unsigned int year
	)
	{
		return encoder->writeId3(artistName, trackName, albumName, year);
	}

	int closeStreamAndWrite(void)
	{
		debug("[+] Gracefully closing current stream");
		Pa_CloseStream(this->pAStream);

		encoder->flush();
		delete encoder;

		return 0;
	}

private:

};

static void debug(const char* d...)
{
	std::cout << d;
}