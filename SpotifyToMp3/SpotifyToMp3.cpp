// copied a lot from https://gist.github.com/eruffaldi/86755065f4c777f01f972abf51890a6e

#include <iostream>
#include <Windows.h>
#include <string>
#include <signal.h> 

#include <INIReader.h>

#include "recordToMp3.h"
#include "streamReader.h"
#include "spotify.h"

const char* defaultVirtualCableDevice = "CABLE Output";
std::string configFilename = "..\\config.ini";

// Prototypes
void signal_handler(int signal);
void exitPaError(PaError err);
int processIni(std::string filename, std::string* clientId, std::string* clientSecret);

int main()
{
	// Catch SIGINT
	typedef void (*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGABRT, signal_handler);

	static_assert(sizeof(int) == 4, "lame support requires 32 bit");

	std::string clientId, clientSecret;
	if (processIni(configFilename, &clientId, &clientSecret)) {
		std::cout << "[!] Failed to parse config: " << configFilename << std::endl;
		ExitProcess(1);
	}

	spotify spot{clientId, clientSecret};
	if (spot.obtainAccessToken()) {
		std::cout << "[!] Failed to obtain access token";
		ExitProcess(1);
	}
	
	Pa_Initialize();
	
	recordToMp3 currMp3("test3.mp3");
	if (currMp3.selectPrimaryDevice(defaultVirtualCableDevice)) {
		std::cout << "[!] Failed to determine primary output device" << std::endl;
		ExitProcess(1);
	}
	if (currMp3.openIntoStream()) {
		std::cout << "[!] Failed to open PA stream" << std::endl;
		ExitProcess(1);
	}	

	Sleep(10000);
	currMp3.closeStreamAndWrite();
	
	Pa_Terminate();
	return 0;
}

int processIni(std::string filename, std::string* clientId, std::string* clientSecret)
{
	INIReader reader(filename);

	if (reader.ParseError() < 0) {
		std::cout << "[!] Failed to load ini" << std::endl;
		return -1;
	}

	const std::string sec = "api";
	*clientId = reader.Get(sec, "clientId", "ffff");
	*clientSecret = reader.Get(sec, "clientSecret", "ffff");

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

