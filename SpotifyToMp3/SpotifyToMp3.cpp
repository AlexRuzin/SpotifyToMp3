#include <iostream>
#include <Windows.h>
#include <pa_win_wasapi.h>
#include <signal.h> 

void signal_handler(int signal);
void debug(const char* d...);

int main()
{
	// Catch SIGINT
	typedef void (*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGABRT, signal_handler);

	PaError paErr = Pa_Initialize();
	if (!paErr)
		ExitProcess(paErr);

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