#pragma once

#include <Windows.h>
#include <string>
#include <locale>
#include <codecvt>
#include <sstream>
#include <vector>
#include <assert.h>
#include <mutex>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

#include <curl/curl.h>
#include "base64.h"

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
static void curlError(int errorCode);
static std::string ws2s(const std::wstring& wstr);

class spotify
{
private:
	CURL *curl;

	std::string *accessToken;

	// API sync mutext
	std::mutex apiSync;

	std::string *responseBuffer;

	typedef struct {
		std::string id;
		std::string name;
		std::string type;
		bool isActive;
		bool isPrivateSession;
		bool isRestricted;
		unsigned int volumePercent;

	} SDEVICES, *PSDEVICES;
	std::vector<SDEVICES> *sDevices;
	std::string primaryDevice;
	std::string primaryDeviceId;

	std::string targetPlaylistId;

public:

	typedef struct {
		std::string		id;
		std::string		artistName;
		std::string		trackName;
		unsigned int	duration;
		std::string		album;
		unsigned int	year;
		unsigned int	trackNumber;
		std::string		genre;
	} TRACK, * PTRACK;

private:
	std::vector<TRACK> trackList;

	std::thread *refreshThread;

public:
	std::string clientId;
	std::string clientSecret;
	std::string *refreshToken;
	unsigned int accessTokenRefreshRate;

	spotify(std::string clientId, std::string clientSecret, unsigned int accessRefreshRate) :
		clientId(clientId),
		clientSecret(clientSecret),
		accessTokenRefreshRate(accessRefreshRate),
		refreshToken(NULL),
		accessToken(NULL),
		curl(NULL)
	{
		
	}

	spotify(std::string clientId, std::string clientSecret, std::string refreshToken, unsigned int accessRefreshRate) :
		clientId(clientId),
		clientSecret(clientSecret),
		refreshToken(new std::string(refreshToken)),
		accessTokenRefreshRate(accessRefreshRate),
		accessToken(NULL),
		curl(NULL)
	{

	}

	// Start periodic refresh of access token
	int startAccessTokenRefresh()
	{
		if (authRefreshToken()) {
			std::cout << "[!] Error in refreshing token" << std::endl;
			ExitProcess(1);
		}

		this->refreshThread = new std::thread(refreshTokenThread, accessTokenRefreshRate, this);

		return 0;
	}

	// Obtain access token (client_credentials, obsolete)
	int obtainAccessToken();

	// Resume playback on default device
	int cmdResumePlayback();

	// Start playback of a specific track
	int cmdResumePlaybackTrack(std::string trackId);

	// Pause playback of a track
	int cmdPausePlayback();

	// Take string input, and return a list of playlists
	typedef struct {
		std::string playlistName;
		std::string owner;
		std::string id;
		std::string URI;

		unsigned int numOfTracks;
	} PLAYLIST, *PPLAYLIST;
	const PLAYLIST *getPlaylistDetails(std::string playlistId);
	const std::vector<PLAYLIST> *returnAllPlaylists(std::string playlistQueryString);

	// Search for a playlist
	int searchPlaylist(std::string playlist, std::string owner);

	// Enum tracks in a playlist
	int enumTracksPlaylist(std::string playlistId, unsigned int numOfTracks);

	// Determine primary audio device from input default device
	int setPrimaryDevice(std::string deviceName);

	const std::vector<TRACK> getTrackList()
	{
		return this->trackList;
	}

	// Get target playlist Id
	const std::string getTargetPlaylistId()
	{
		return this->targetPlaylistId;
	}

	~spotify(void)
	{
		//curl_easy_cleanup(this->curl);
	}

	void lockApi()
	{
		this->apiSync.lock();
	}
	void unlockApi()
	{
		this->apiSync.unlock();
	}

private:
	// Generate an access token from a refresh token
	int authRefreshToken();

	static void refreshTokenThread(unsigned int refreshRate, spotify *s)
	{
		for(;;) {
			Sleep(refreshRate * 1000 * 60);
			if (s->authRefreshToken()) {
				std::cout << "[!] Error in refreshing token" << std::endl;
				ExitProcess(1);
			}
			std::cout << "[+] Access token refreshed" << std::endl;
		}
	}

	std::vector<SDEVICES>* getDeviceList();

	void setCurlResponseBuffer()
	{
		if (this->responseBuffer != NULL) {
			delete this->responseBuffer;
			this->responseBuffer = NULL;
		}
		this->responseBuffer = new std::string;

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseBuffer);
	}

	void setCurlUri(CURL* c, std::string URI, std::string verb)
	{
		curl_easy_setopt(curl, CURLOPT_URL, URI.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, verb.c_str());
	}

	struct curl_slist* returnAuthHeader()
	{
		std::string authBearer = "Authorization: Bearer " + *this->accessToken + "";
		struct curl_slist* chunk = NULL;
		return curl_slist_append(chunk, authBearer.c_str());
	}
};

static std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

static void curlError(int errorCode)
{
	std::stringstream ss;
	ss << errorCode;
	std::string s;
	ss >> s;
	std::cout << "[!] CURL error: " << s << std::endl;
	ExitProcess(errorCode);
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}




