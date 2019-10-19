// https://gist.github.com/eruffaldi/86755065f4c777f01f972abf51890a6e

#include <iostream>
#include <Windows.h>
#include <string>
#include <signal.h> 

#include "recordToMp3.h"
#include "streamReader.h"

// Prototypes
void signal_handler(int signal);
void exitPaError(PaError err);

int main()
{
	// Catch SIGINT
	typedef void (*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGABRT, signal_handler);

	static_assert(sizeof(int) == 4, "lame support requires 32 bit");
	
	Pa_Initialize();
	
	recordToMp3 currMp3("test.mp3");
	
	Pa_Terminate();
	return 0;
}



void signal_handler(int signal)
{
	switch (signal)
	{
	case SIGINT:
	default:
		std::cout << "[+] Caught exit signal" << std::endl;
		ExitProcess(0);
	}
}

