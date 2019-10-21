#pragma once

#include <Windows.h>
#include <string>
#include <sstream>
#include <cpprest/json.h>

#include <curl/curl.h>
#include "base64.h"

using namespace web;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
static void curlError(int errorCode);

class spotify
{
private:
	CURL *curl;

	std::string accessToken;

public:
	std::string clientId;
	std::string clientSecret;

	spotify(std::string clientId, std::string clientSecret) :
		clientId(clientId),
		clientSecret(clientSecret)
	{
		
	}

	int obtainAccessToken()
	{
		this->curl = curl_easy_init();

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

		std::string readBuffer;
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.spotify.com/api/token");
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

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

		utility::stringstream_t ss;
		ss << &readBuffer;
		json::value output = json::value::parse(ss);
		
		auto accessToken = output[U("access_token")];

		return 0;
	}

	~spotify(void)
	{
		//curl_easy_cleanup(this->curl);
	}

};

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




