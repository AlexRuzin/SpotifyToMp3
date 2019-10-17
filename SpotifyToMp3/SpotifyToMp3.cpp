#include <iostream>
#include <Windows.h>
#include <string>
#include <signal.h> 

#include "streamReader.h"

// Default parameters
const unsigned int inputChannels = 1;
const unsigned int outputChannels = 0;
const float sampleRate = 44100;
const unsigned int bufferLength = sizeof(__int32) * 64;

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

	return 0;
}

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