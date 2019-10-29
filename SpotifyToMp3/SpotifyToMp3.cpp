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

typedef struct {
	std::string clientId;
	std::string clientSecret;
	std::string refreshToken;
	std::string defaultDevice;
	std::string playlist;
} INICFG, * PINICFG;

// Prototypes
void signal_handler(int signal);
void exitPaError(PaError err);
int processIni(std::string filename, PINICFG* cfg);


int main()
{
	// Catch SIGINT
	typedef void (*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGABRT, signal_handler);

	static_assert(sizeof(int) == 4, "lame support requires 32 bit");
	PINICFG cfg = NULL;
	if (processIni(configFilename, &cfg)) {
		std::cout << "[!] Failed to parse config: " << configFilename << std::endl;
		ExitProcess(1);
	}

	spotify spot{cfg->clientId, cfg->clientSecret, cfg->refreshToken};
	if (spot.authRefreshToken()) {
		std::cout << "[!] Failed to obtain access token from refresh token" << std::endl;
		ExitProcess(1);
	}

	if (spot.searchPlaylist(cfg->playlist, "Silhouettes")) {
		std::cout << "[!] Failed to find playlist: " << cfg->playlist << std::endl;
		ExitProcess(0);
	}

	if (spot.enumTracksPlaylist(spot.getTargetPlaylistId())) {
		std::cout << "[!] Failed to enum playlist" << std::endl;
		ExitProcess(0);
	}

	if (spot.setPrimaryDevice(cfg->defaultDevice)) {
		ExitProcess(1);
	}

	if (spot.cmdResumePlayback()) {
		std::cout << "[!] Failed to resume playback" << std::endl;
		ExitProcess(0);
	}

	if (spot.obtainAccessToken()) {
		std::cout << "[!] Failed to obtain access token";
		ExitProcess(1);
	}
	std::cout << "[+] Successfully obtained API token";
	
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

int processIni(std::string filename, PINICFG *cfg)
{
	INIReader reader(filename);

	if (reader.ParseError() < 0) {
		std::cout << "[!] Failed to load ini" << std::endl;
		return -1;
	}

	*cfg = new INICFG;

	const std::string sec = "api";
	(*cfg)->clientId = reader.Get(sec, "clientId", "ffff");
	(*cfg)->clientSecret = reader.Get(sec, "clientSecret", "ffff");
	(*cfg)->refreshToken = reader.Get(sec, "refreshToken", "ffff");
	(*cfg)->defaultDevice = reader.Get(sec, "defaultDevice", "ffff");
	(*cfg)->playlist = reader.Get(sec, "playlistSearch", "ffff");

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

