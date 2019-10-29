#include "spotify.h"

int spotify::authRefreshToken()
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

int spotify::obtainAccessToken()
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

int spotify::cmdResumePlayback()
{
	assert(this->primaryDeviceId != "");
	this->apiSync.lock();
	assert(this->curl == NULL);

	// curl -X PUT "https://api.spotify.com/v1/me/player/play" -H "Authorization: Bearer {your access token}"

	this->curl = curl_easy_init();

	setCurlResponseBuffer();
	setCurlUri(curl, "https://api.spotify.com/v1/me/player/play?device_id=" + this->primaryDeviceId, "PUT");

	struct curl_slist* chunk = returnAuthHeader();
	chunk = curl_slist_append(chunk, "Content-Length: 0");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

	int rc = curl_easy_perform(curl);
	if (rc != CURLE_OK) {
		curlError(rc);
	}
	curl_easy_cleanup(curl);
	this->curl = NULL;

	this->apiSync.unlock();
	return 0;
}

int spotify::setPrimaryDevice(std::string deviceName)
{
	this->sDevices = getDeviceList();
	if (sDevices->size() == 0) {
		std::cout << "[!] Returned 0 physical devices" << std::endl;
		return -1;
	}

	for (std::vector<SDEVICES>::iterator i = sDevices->begin(); i != sDevices->end(); ++i) {
		if ((*i).name == deviceName) {
			this->primaryDevice = deviceName;
			this->primaryDeviceId = (*i).id;
			return 0;
		}
	}

	std::cout << "[!] Failed to find device: " << deviceName << std::endl;
	return -1;
}

std::vector<spotify::SDEVICES>* spotify::getDeviceList()
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