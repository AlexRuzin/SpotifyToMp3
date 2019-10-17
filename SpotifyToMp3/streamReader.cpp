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

int streamReader::getHostsDevices(std::vector<PaHostApiInfo *>* hostApis, std::vector<PaDeviceInfo *>* devices)
{
	PaHostApiIndex hostCount = Pa_GetHostApiCount();
	PaHostApiIndex deviceCount = Pa_GetDeviceCount();

	if (hostCount < 0) {
		debugS("[!] hostCount cannot be zero");
	}
	if (deviceCount < 0) {
		debugS("[!] deviceCount cannot be zero");
	}
		
	devices = new(std::vector <PaDeviceInfo *>);
	for (int i = 0; i < deviceCount; i++) {
		devices->push_back(const_cast<PaDeviceInfo *>(Pa_GetDeviceInfo((PaDeviceIndex)i)));
	}

	hostApis = new(std::vector<PaHostApiInfo*>);
	for (int i = 0; i < hostCount; i++) {
		
	}

	return 0;
}

int streamReader::getDefaultDevices(void)
{
	int err = getHostsDevices(&this->hostApis, &this->devices);
	if (!err)
		exitPaError(err);
	


	return 0;
}

void debugS(const char* d...)
{
	std::cout << d;
}
