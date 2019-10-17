#pragma once
#include <pa_win_wasapi.h>

void exitPaError(PaError err);

static bool isPaInitialized = false;

class streamReader
{
private:
	PaDeviceInfo* inDevice;
	PaDeviceInfo* outDevice;

public:
	const unsigned int input_channels;
	const unsigned int output_channels;
	const float sample_rate;
	const unsigned int buffer_length;

	streamReader(	unsigned int num_input_channels, 
					unsigned int num_output_channels,
					float sample_rate,
					unsigned int buffer_length) :
		input_channels(num_input_channels),
		output_channels(num_output_channels),
		sample_rate(sample_rate),
		buffer_length(buffer_length)
	{		
		if (!initializePA())
			exitPaError(paUnanticipatedHostError);
	}

	~streamReader()
	{
		Pa_Terminate();
	}

private:
	bool initializePA(void)
	{
		PaError paErr = Pa_Initialize();
		if (paErr != paNoError)
			exitPaError(paErr);
		isPaInitialized = true;
		return true;
	}


};

