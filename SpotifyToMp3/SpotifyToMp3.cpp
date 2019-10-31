// https://gist.github.com/eruffaldi/86755065f4c777f01f972abf51890a6e

#include <iostream>
#include <Windows.h>
#include <string>
#include <signal.h> 
#include <chrono>
#include <thread>
#include <boost/filesystem/operations.hpp>

#include <INIReader.h>

#include "recordToMp3.h"
#include "streamReader.h"
#include "spotify.h"
#include "progressBar.h"

std::string configFilename = "..\\config.ini";

typedef enum {
	// Use ini to configure a static playlist
	STATIC,

	// Interactively select a playlist
	SEARCH,

	// Specify a playlist ID
	ID_ONLY
} MODE;

typedef struct {
	MODE mode;

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
void searchConsole(std::string* playlistName, std::string* owner, spotify* s);
void removeForbiddenChar(std::string* s);

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

	std::string playlistName, owner;
	std::string playlistId;
	switch (cfg->mode) {
	case (MODE::STATIC):
		playlistName = cfg->playlist;
		owner = cfg->author;

		break;
	case (MODE::SEARCH):
		searchConsole(&playlistName, &owner, &spot);
		std::cout << "[+] Selecting: " << playlistName << " by " << owner << std::endl;
		break;

	case (MODE::ID_ONLY):
		std::cout << "-> Enter playlist ID: ";
		std::getline(std::cin, playlistId);

		break;
	}

	if (cfg->mode != MODE::ID_ONLY) {
		if (spot.searchPlaylist(playlistName, owner)) {
			std::cout << "[!] Failed to find playlist: " << playlistName << std::endl;
			ExitProcess(0);
		}
	}

	std::cout << "[+] Enumerating tracks for playlist: " + playlistName << std::endl;
	const std::string activeId = (cfg->mode != MODE::ID_ONLY) ? spot.getTargetPlaylistId() : playlistId;
	const spotify::PLAYLIST* playlistDetail = spot.getPlaylistDetails(activeId);
	if (cfg->mode == MODE::ID_ONLY) {
		playlistName = playlistDetail->playlistName;
		owner = playlistDetail->owner;
	}
	if (spot.enumTracksPlaylist(activeId, playlistDetail->numOfTracks)) {
		std::cout << "[!] Failed to enum playlist" << std::endl;
		ExitProcess(0);
	}

	std::cout << "[+] Getting tracks for playlist: " + playlistName  + ", by author: " + owner << std::endl;
	const std::vector<spotify::TRACK> &trackList = spot.getTrackList();

	std::cout << "[+] Querying default device: " + cfg->defaultSpotifyDevice << std::endl;
	if (spot.setPrimaryDevice(cfg->defaultSpotifyDevice)) {
		ExitProcess(1);
	}

	assert(trackList.size() > 0);
	assert(trackList.size() > cfg->playlistOffset);
	std::cout << "[+] We have " + std::to_string(trackList.size()) + " tracks available" << std::endl;
	if (cfg->playlistOffset > 0) {
		std::cout << "[+] Playlist offset: " + std::to_string(cfg->playlistOffset) << std::endl;
	}
	
	std::cout << "[+] Initializing PA for audio device: " + cfg->defaultAudioDevice << std::endl;
	Sleep(1000);
	Pa_Initialize();
	std::cout << "[+] PortAudio successfully initialized" << std::endl;
	Sleep(1000);

	std::string playlistDir = playlistName + " - " + owner;
	removeForbiddenChar(&playlistDir);
	playlistDir = playlistDir;
	boost::filesystem::remove_all(playlistDir);
	std::cout << "[+] Creating directory: " << playlistDir;
	boost::filesystem::create_directory(playlistDir);

	for (std::vector<spotify::TRACK>::const_iterator i = trackList.begin(); i != trackList.end(); i++) {
		std::cout << i->artistName << " - " << i->trackName << " (" << i->album << ") " << std::endl;
	}

	std::cout << "[+] Total playlist play time: " << getTotalPlaylistTime(trackList) << std::endl;
	Sleep(2000);

	for (std::vector<spotify::TRACK>::const_iterator currTrack =
		trackList.begin() + cfg->playlistOffset; currTrack != trackList.end(); currTrack++) {
		std::cout << "[+] Track " + std::to_string(std::distance(trackList.begin(), currTrack)) + "/" +
			std::to_string(trackList.size()) + " Title: " + currTrack->trackName + " Artist: " + currTrack->artistName +
			" (" + currTrack->album + ")" << std::endl;
		std::cout << "[+] Track Length: " << getTrackLength(*currTrack) << std::endl;

		std::string filename = currTrack->artistName + " - " + currTrack->trackName;
		removeForbiddenChar(&filename);
		filename = ".\\" + playlistDir + "\\" + filename + ".mp3";
		recordToMp3 currMp3(filename);
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

void removeForbiddenChar(std::string* s)
{
	std::string illegalChars = "\\/:?\"<>|";

	for (char c : illegalChars) {
		s->erase(std::remove(s->begin(), s->end(), c), s->end());
	}
	s = new std::string(illegalChars);
}

void searchConsole(std::string* playlistName, std::string* owner, spotify *s)
{
	for (;;) {
		std::cout << "-> Enter playlist search term: ";		
		std::string term;
		std::getline(std::cin, term);

		Sleep(1000);
		const std::vector<spotify::PLAYLIST>* playlists = s->returnAllPlaylists(term);
		if (playlists->size() == 0) {
			std::cout << "[!] No results" << std::endl;
			continue;
		}
		std::cout << "[+] Results: " << std::endl;
		for (std::vector<spotify::PLAYLIST>::const_iterator i = playlists->begin();
			i != playlists->end(); i++) {

			std::cout << "[" << std::to_string(std::distance(playlists->begin(), i) + 1) << "] " <<
				i->playlistName << " by " << i->owner << std::endl;
		}

		std::cout << "-> Select playlist number (\"Q\" to search again)" << std::endl;
		std::getline(std::cin, term);
		if (term == "Q" || term == "q") {
			continue;
		}

		const unsigned int userIndex = atoi(term.c_str());
		if (userIndex > playlists->size()) {
			std::cout << "[!] Invalid value" << std::endl;
			continue;
		}
		
		*playlistName = playlists->at(userIndex - 1).playlistName;
		*owner = playlists->at(userIndex - 1).owner;
		return;
	}
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

	const std::string m = reader.GetString(sec, "mode", "search");
	if (m == "search") {
		(*cfg)->mode = MODE::SEARCH;
	} else if (m == "static") {
		(*cfg)->mode = MODE::STATIC;
	} else if (m == "id") {
		(*cfg)->mode = MODE::ID_ONLY;
	}
	else {
		std::cout << "[!] Unknown mode: " + m << std::endl;
		return -1;
	}

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

