#include "streamReader.h"
#include <string>
#include <iostream>
#include <Windows.h>

void exitPaError(PaError err)
{
	if (isPaInitialized) {
		Pa_Terminate();
	}
	if (err == paUnanticipatedHostError) {
		const PaHostErrorInfo* e = Pa_GetLastHostErrorInfo();
		std::string out = "PAERR: Type: " + std::to_string(e->hostApiType) + ", Code: " + std::to_string(e->errorCode) + ", Message: " + e->errorText;
		std::cout << out;
		ExitProcess(1);
	}
	else {
		std::cout << "Unknown PA error";
	}
}