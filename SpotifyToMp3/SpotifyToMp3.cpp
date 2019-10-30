// copied a lot from https://gist.github.com/eruffaldi/86755065f4c777f01f972abf51890a6e

#include <iostream>
#include <Windows.h>
#include <string>
#include <signal.h> 
#include <chrono>
#include <thread>

#include <INIReader.h>

#include "recordToMp3.h"
#include "streamReader.h"
#include "spotify.h"
#include "progressBar.h"

std::string configFilename = "..\\config.ini";

typedef struct {
	std::string clientId;
	std::string clientSecret;
	std::string refreshToken;
	std::string defaultSpotifyDevice;

	std::string playlist;
	std::string author;
	unsigned int playlistOffset;

	std::string defaultAudioDevice;

	unsigned int accessTokenRefresh;
} INICFG, * PINICFG;

void signal_handler(int signal);
void exitPaError(PaError err);
int processIni(std::string filename, PINICFG* cfg);
std::string getTotalPlaylistTime(const std::vector<spotify::TRACK> trackList);
std::string getTrackLength(spotify::TRACK track);
void displayTrackProgress(unsigned int ms, spotify *s);

int main()
{
	// Catch SIGINT
	typedef void (*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGABRT, signal_handler);

	static_assert(sizeof(int) == 4, "lame support requires 32 bit");
	PINICFG cfg = NULL;
	std::cout << "[+] Parsing config: " + configFilename << std::endl;
	if (processIni(configFilename, &cfg)) {
		std::cout << "[!] Failed to parse config: " << configFilename << std::endl;
		ExitProcess(1);
	}

	std::cout << "[+] Attempting authentication using refresh token" << std::endl;
	spotify spot{cfg->clientId, cfg->clientSecret, cfg->refreshToken, cfg->accessTokenRefresh};
	if (spot.startAccessTokenRefresh()) {
		std::cout << "[!] Failed to obtain access token from refresh token" << std::endl;
		ExitProcess(1);
	}

	std::cout << "[+] Searching playlist: " + cfg->playlist + ", by author: " + cfg->author << std::endl;
	if (spot.searchPlaylist(cfg->playlist, cfg->author)) {
		std::cout << "[!] Failed to find playlist: " << cfg->playlist << std::endl;
		ExitProcess(0);
	}

	std::cout << "[+] Enumerating tracks for playlist: " + cfg->playlist << std::endl;
	if (spot.enumTracksPlaylist(spot.getTargetPlaylistId())) {
		std::cout << "[!] Failed to enum playlist" << std::endl;
		ExitProcess(0);
	}

	std::cout << "[+] Querying default device: " + cfg->defaultSpotifyDevice << std::endl;
	if (spot.setPrimaryDevice(cfg->defaultSpotifyDevice)) {
		ExitProcess(1);
	}

	const std::vector<spotify::TRACK> trackList = spot.getTrackList();
	assert(trackList.size() > 0);
	assert(trackList.size() > cfg->playlistOffset);
	std::cout << "[+] We have " + std::to_string(trackList.size()) + " tracks available" << std::endl;
	if (cfg->playlistOffset > 0) {
		std::cout << "[+] Playlist offset: " + std::to_string(cfg->playlistOffset) << std::endl;
	}
	std::cout << "[+] Total playlist play time: " << getTotalPlaylistTime(trackList) << std::endl;

	std::cout << "[+] Initializing PA for audio device: " + cfg->defaultAudioDevice << std::endl;
	Sleep(1000);
	Pa_Initialize();
	std::cout << "[+] PortAudio successfully initialized" << std::endl;
	Sleep(1000);

	for (std::vector<spotify::TRACK>::const_iterator currTrack = 
		trackList.begin() + cfg->playlistOffset; currTrack != trackList.end(); currTrack++) {
		std::cout << "[+] Track " + std::to_string(std::distance(trackList.begin(), currTrack)) + "/" +
			std::to_string(trackList.size()) + " Title: " + currTrack->trackName + " Artist: " + currTrack->artistName +
			" (" + currTrack->album + ")" << std::endl;
		std::cout << "[+] Track Length: " << getTrackLength(*currTrack) << std::endl;

		std::string fileName = currTrack->artistName + " - " + currTrack->trackName + ".mp3";
		recordToMp3 currMp3(fileName);
		if (currMp3.selectPrimaryDevice(cfg->defaultAudioDevice)) {
			std::cout << "[!] Failed to determine primary output device" << std::endl;
			ExitProcess(1);
		}
		if (currMp3.openIntoStream()) {
			std::cout << "[!] Failed to open PA stream" << std::endl;
			ExitProcess(1);
		}

		if (spot.cmdResumePlaybackTrack(currTrack->id)) {
			std::cout << "[!] Failed to resume playback" << std::endl;
			ExitProcess(1);
		}

		std::thread disp(displayTrackProgress, currTrack->duration, &spot);
		disp.join();

		if (spot.cmdPausePlayback()) {
			std::cout << "[!] Failed to pause playback" << std::endl;
			ExitProcess(1);
		}

		if (currMp3.writeId3(currTrack->artistName, currTrack->trackName, currTrack->album, currTrack->year)) {
			std::cout << "[!] Failed to write id3" << std::endl;
			ExitProcess(1);
		}

		currMp3.closeStreamAndWrite();
	}	

	std::cout << "[+] All operations done, closing PA" << std::endl;
	Pa_Terminate();
	return 0;
}

static void displayTrackProgress(unsigned int ms, spotify *s)
{
	s->lockApi();
	ProgressBar pb(ms / 1000, 40);
	for (int i = 0; i < ms / 1000; i++) {
		++pb;

		pb.display();
		Sleep(1000);
	}
	pb.done();
	s->unlockApi();
	return;
}

static std::string getTrackLength(spotify::TRACK track)
{
	std::chrono::milliseconds ms{ track.duration };
	auto hours = std::chrono::duration_cast<std::chrono::hours>(ms);
	auto minutes = std::chrono::duration_cast<std::chrono::minutes>(ms);
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(ms);

	return std::string(std::to_string(minutes.count()) + ":" + 
		std::to_string(seconds.count() - (minutes.count() * 60)));
}

static std::string getTotalPlaylistTime(const std::vector<spotify::TRACK> trackList)
{
	unsigned int totalTimeMs = 0;
	for (std::vector<spotify::TRACK>::const_iterator i = trackList.begin(); i != trackList.end(); i++) {
		totalTimeMs += i->duration;
	}

	std::chrono::milliseconds ms{ totalTimeMs };
	auto hours = std::chrono::duration_cast<std::chrono::hours>(ms);
	auto minutes = std::chrono::duration_cast<std::chrono::minutes>(ms);
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(ms);

	return std::string(std::to_string(hours.count()) + "h " + 
		std::to_string(minutes.count() - (hours.count() * 60)) + "m " + 
		std::to_string(seconds.count() - (minutes.count() * 60)) + "s ");
}

static int processIni(std::string filename, PINICFG *cfg)
{
	INIReader reader(filename);

	if (reader.ParseError() < 0) {
		std::cout << "[!] Failed to load ini" << std::endl;
		return -1;
	}

	*cfg = new INICFG;

	const std::string sec = "api";
	(*cfg)->clientId = reader.Get(sec, "clientId", "unknown");
	(*cfg)->clientSecret = reader.Get(sec, "clientSecret", "unknown");
	(*cfg)->refreshToken = reader.Get(sec, "refreshToken", "unknown");
	(*cfg)->defaultSpotifyDevice = reader.Get(sec, "defaultSpotifyDevice", "unknown");
	(*cfg)->playlist = reader.Get(sec, "playlistSearch", "unknown");
	(*cfg)->author = reader.Get(sec, "playlistAuthor", "unknown");
	(*cfg)->defaultAudioDevice = reader.Get(sec, "defaultAudioDevice", "unknown");
	(*cfg)->playlistOffset = reader.GetInteger(sec, "playlistTrackOffset", 0);
	(*cfg)->accessTokenRefresh = reader.GetInteger(sec, "accessTokenRefresh", 0);

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

