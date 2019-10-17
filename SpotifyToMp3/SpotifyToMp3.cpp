#include <iostream>
#include <Windows.h>
#include <string>
#include <signal.h> 

#include "streamReader.h"

// Default parameters
const unsigned int input_channels = 1;
const unsigned int output_channels = 0;
const float sample_rate = 44100;
const unsigned int buf_len = sizeof(__int32) * 64;

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

	streamReader reader{ input_channels, output_channels, sample_rate, buf_len };

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