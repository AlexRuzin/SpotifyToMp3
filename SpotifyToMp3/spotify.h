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
	std::string clientId;
	std::string clientSecret;
	std::string *refreshToken;

	spotify(std::string clientId, std::string clientSecret) :
		clientId(clientId),
		clientSecret(clientSecret),
		refreshToken(NULL),
		accessToken(NULL),
		curl(NULL)
	{
		
	}

	spotify(std::string clientId, std::string clientSecret, std::string refreshToken) :
		clientId(clientId),
		clientSecret(clientSecret),
		refreshToken(new std::string(refreshToken)),
		accessToken(NULL),
		curl(NULL)
	{

	}

	// Generate a refresh token
	int authRefreshToken();

	// Obtain access token (client_credentials, obsolete)
	int obtainAccessToken();

	// Resume playback on default device
	int cmdResumePlayback();

	// Search for a playlist
	int searchPlaylist(std::string playlist, std::string owner);

	// Determine primary audio device from input default device
	int setPrimaryDevice(std::string deviceName);

	~spotify(void)
	{
		//curl_easy_cleanup(this->curl);
	}

private:
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




