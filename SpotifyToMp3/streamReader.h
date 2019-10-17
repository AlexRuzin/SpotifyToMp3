#pragma once
#include <vector>

#include <pa_win_wasapi.h>


void exitPaError(PaError err);
void debugS(const char* d...);

static bool isPaInitialized = false;

class streamReader
{
private:
	PaDeviceInfo* inDevice;
	PaDeviceInfo* outDevice;

	std::vector<PaHostApiInfo *> hostApis;
	std::vector<PaDeviceInfo *> devices;

public:
	const unsigned int inputChannels;
	const unsigned int outputChannels;
	const float sampleRate;
	const unsigned int bufferLength;

	streamReader(	unsigned int numInputChannels,
					unsigned int numOutputChannels,
					float sampleRate,
					unsigned int bufferLength) :
		inputChannels(numInputChannels),
		outputChannels(numOutputChannels),
		sampleRate(sampleRate),
		bufferLength(bufferLength)
	{		
		if (!initializePA())
			exitPaError(paUnanticipatedHostError);
	}

	~streamReader()
	{
		Pa_Terminate();
	}

	int getDefaultDevices(void);

private:
	int getHostsDevices(std::vector<PaHostApiInfo *> *hostApis, std::vector<PaDeviceInfo *> *devices);

	bool initializePA(void)
	{
		PaError paErr = Pa_Initialize();
		if (paErr != paNoError)
			exitPaError(paErr);
		isPaInitialized = true;
		return true;
	}


};

