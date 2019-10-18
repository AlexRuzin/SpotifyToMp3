// https://gist.github.com/eruffaldi/86755065f4c777f01f972abf51890a6e

#include <iostream>
#include <Windows.h>
#include <string>
#include <signal.h> 

#include "streamReader.h"

// Default parameters
const int lameEncoderQuality = 2;

// Prototypes
void signal_handler(int signal);
void debug(const char* d...);
void exitPaError(PaError err);

int main()
{
	// Catch SIGINT
	typedef void (*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGABRT, signal_handler);

	streamReader reader{ inputChannels, outputChannels, sampleRate, bufferLength };

	static_assert(sizeof(int) == 4, "lame support requires 32 bit");
	
	Pa_Initialize();
	
	recordToMp3 currMp3("test.mp3");
	
	Pa_Terminate();
	return 0;
}

class recordToMp3 {
public:
	const char *filename;
	
friend:
	char *deviceName;
	int targetDevice;
	double streamRate;
	
	PaDeviceInfo *device;
	PaStream *pAStream;
	PaStreamParameters *streamParams;
	
	mp3Encoder *encoder;
	
public:
	recordToMp3(const char *filename) :
		targetDevice(0),
		pAStream(NULL)
	{
		if (filename == NULL) {
			debug("[!] filename cannot be NULL");
			ExitProcess(1);
		}
		filename = filename;
	}
	
	~recordToMp3()
	{
		Pa_CloseStream(this->pAStream);
	}
	
	int selectPrimaryDevice(const char *deviceName)
	{
		this->deviceName = const_cast<char *>(deviceName);
		const int deviceCount = Pa_GetDeviceCount();
		for (int i = 0; i < deviceCount; i++) {
			if (std::string(Pa_GetDeviceInfo(i)->name).find(deviceName) != std::string::npos) {
				this->device = Pa_GetDeviceInfo(i)
				cout << "[+] Using device: " << this->device << std::endl;
				this->targetDevice = i;
			}
		}
		
		this->streamRate = this->device->defaultSampleRate;
		
		return 0;
	}
	
	int openIntoStream()
	{
		this->streamParams = new(PaStreamParameters);
		streamParams->channelCount = this->device->maxInputChannels;
		streamParams->device = targetDevice;
		streamParams->hostApiSpecificsStreamInfo = NULL;
		streamParams->sampleFormat = paInt32|paNonInterleaved;
		streamParams->suggestedLatency = this->device->defaultLowInputLatency;
		
		this->encoder = new mp3Encoder(sampleRate, streamParams->channelCount, 128000, this->fileName);
		
		int err = Pa_OpenStream(this->pAStream, 
					streamParams, 
					NULL, 
					this->streamRate, 
					paNoFlag, 
					this->encoder->paCallback_i32);
		if (!err) {
			debug("[!] Error opening PA stream");
			ExitProcess(0);
		} 
		err = Pa_StartStream(this->pAStream);
		if (!err) {
			debug("[!] Error in starting PA stream");
			ExitProcess(0);
		}
		
		return 0;
	}
		
private:
	
};

class mp3Encoder : public recordToMp3 {
public:
	int sampleRate;
	int channels;
	int bitrate;
	std::string fileName;
	
private:
	std::vector<char> buf;
	lame_global_flags *lameFlags;
	
public:	
	mp3Encoder(int sampleRate, int channels, int bitrate, std::string fileName) :
	sampleRate(sampleRate),
	channels(channels),
	bitrate(bitrate),
	fileName(fileName)
	{
		buf.resize(1024 * 64);
		lameFlags = lame_init();
		if (!lame_init) {
			ExitProcess(-1);	
		}
		
		lame_set_in_samplerate(lameFlags, sampleRate);
		lame_set_num_channels(lameFlags, channels);
		lame_set_out_samplerate(lameFlags, 0);
		
		lame_set_brate(lameFlags, lameEncoderQuality);
		
		lame_init_params(this->lameFlags);
	}
	
	static paCallback_i32(const void * input, void *output,
			      unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
			      PaStreamCallbackFlags statusFlags, void *userData)
	{
		mp3Encoder *encPtr = (mp3Encoder *)userData;
		const int32_t **pc = (const int32_t**)input;
		enc->encode_sep(pc[0],pc[1],frameCount);
		return 0;
	}
};

void signal_handler(int signal)
{
	switch (signal)
	{
	case SIGINT:
	default:
		debug("[+] Caught exit signal");
		ExitProcess(0);
	}
}

void debug(const char* d...)
{
	std::cout << d;
}
