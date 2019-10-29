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

	int authRefreshToken()
	{
		if (this->refreshToken == NULL) {
			std::cout << "[!] refreshToken cannot be NULL with authRefreshToken()" << std::endl;
			return -1;
		}

		this->apiSync.lock();
		assert(this->curl == NULL);

		this->curl = curl_easy_init();

		setCurlResponseBuffer();
		setCurlUri(curl, "https://accounts.spotify.com/api/token", "POST");

		std::string postData = "grant_type=refresh_token&refresh_token=" + *refreshToken + 
			"&client_id=" + clientId + "&client_secret=" + clientSecret;
		std::cout << postData << std::endl;
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());

		int rc = curl_easy_perform(curl);
		if (rc != CURLE_OK) {
			curlError(rc);
			return rc;
		}
		curl_easy_cleanup(curl);
		this->curl = NULL;

		rapidjson::Document jsonDoc;
		jsonDoc.Parse(responseBuffer->c_str());
		assert(jsonDoc.HasMember("access_token"));
		this->accessToken = new std::string(jsonDoc["access_token"].GetString());

		std::cout << "[+] Access Token: " << *this->accessToken << std::endl;

		this->apiSync.unlock();

		return 0;
	}

	int obtainAccessToken()
	{
		this->apiSync.lock();
		assert(this->curl == NULL);

		this->curl = curl_easy_init();

		setCurlResponseBuffer();
		setCurlUri(curl, "https://accounts.spotify.com/api/token", "POST");

		std::string raw = clientId + ":" + clientSecret;
		std::string encoded = "Authorization: Basic " + base64_encode((const BYTE*)raw.c_str(), raw.length());
		struct curl_slist* chunk = NULL;
		chunk = curl_slist_append(chunk, encoded.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		std::string postData = "grant_type=client_credentials";
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());

		int rc = curl_easy_perform(curl);
		if (rc != CURLE_OK) {
			curlError(rc);
		}
		curl_easy_cleanup(curl);
		this->curl = NULL;

		rapidjson::Document jsonDoc;
		jsonDoc.Parse(responseBuffer->c_str());
		assert(jsonDoc.HasMember("access_token"));
		this->accessToken = new std::string(jsonDoc["access_token"].GetString());

		this->apiSync.unlock();

		return 0;
	}

	int cmdResumePlayback()
	{
		assert(this->primaryDevice != "");
		this->apiSync.lock();
		assert(this->curl == NULL);

		// curl -X PUT "https://api.spotify.com/v1/me/player/play" -H "Authorization: Bearer {your access token}"

		this->curl = curl_easy_init();

		setCurlUri(curl, "https://api.spotify.com/v1/me/player/play", "POST");

		struct curl_slist* chunk = returnAuthHeader();
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		std::string postData = "device=" + this->primaryDevice;
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());

		int rc = curl_easy_perform(curl);
		if (rc != CURLE_OK) {
			curlError(rc);
		}
		curl_easy_cleanup(curl);
		this->curl = NULL;

		this->apiSync.unlock();
		return 0;
	}

	int setPrimaryDevice(std::string deviceName)
	{
		this->sDevices = getDeviceList();
		if (sDevices->size() == 0) {
			std::cout << "[!] Returned 0 physical devices" << std::endl;
			return -1;
		}

		for (std::vector<SDEVICES>::iterator i = sDevices->begin(); i != sDevices->end(); ++i) {
			if ((*i).name == deviceName) {
				this->primaryDevice = deviceName;
				return 0;
			}
		}

		std::cout << "[!] Failed to find device: " << deviceName << std::endl;
		return -1;
	}

	~spotify(void)
	{
		//curl_easy_cleanup(this->curl);
	}

private:
	std::vector<SDEVICES>* getDeviceList()
	{
		this->apiSync.lock();
		assert(this->accessToken != NULL);
		assert(this->curl == NULL);

		this->curl = curl_easy_init();

		setCurlResponseBuffer();
		setCurlUri(curl, "https://api.spotify.com/v1/me/player/devices", "GET");

		struct curl_slist* chunk = returnAuthHeader();
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		int rc = curl_easy_perform(curl);
		if (rc != CURLE_OK) {
			curlError(rc);
		}
		curl_easy_cleanup(curl);
		this->curl = NULL;

		std::vector<SDEVICES>* out = new(std::vector<SDEVICES>);

		rapidjson::Document jsonDoc;
		jsonDoc.Parse(responseBuffer->c_str());
		assert(jsonDoc.HasMember("devices"));
		const rapidjson::Value& deviceArray = jsonDoc["devices"];
		assert(deviceArray.IsArray());
		for (rapidjson::SizeType i = 0; i < deviceArray.Size(); i++) {
			out->push_back(SDEVICES{
				std::string(deviceArray[i]["id"].GetString()),
				std::string(deviceArray[i]["name"].GetString()),
				std::string(deviceArray[i]["type"].GetString()),
				deviceArray[i]["is_active"].GetBool(),
				deviceArray[i]["is_private_session"].GetBool(),
				deviceArray[i]["is_restricted"].GetBool(),
				deviceArray[i]["volume_percent"].GetUint()
				});
		}

		this->apiSync.unlock();
		return out;
	}

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




