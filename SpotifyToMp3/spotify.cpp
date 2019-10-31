#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <regex>

#include "spotify.h"

std::string url_encode(const std::string& value);

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
	if (this->accessToken != NULL) {
		delete accessToken;
		accessToken = NULL;
	}
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

int spotify::cmdPausePlayback()
{
	assert(this->primaryDeviceId != "");
	this->apiSync.lock();
	assert(this->curl == NULL);

	this->curl = curl_easy_init();

	setCurlResponseBuffer();
	setCurlUri(curl, "https://api.spotify.com/v1/me/player/pause?device_id=" + this->primaryDeviceId, "PUT");

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

int spotify::cmdResumePlaybackTrack(std::string trackId)
{
	assert(this->primaryDeviceId != "");
	this->apiSync.lock();
	assert(this->curl == NULL);

	this->curl = curl_easy_init();

	setCurlResponseBuffer();
	setCurlUri(curl, "https://api.spotify.com/v1/me/player/play?device_id=" + this->primaryDeviceId, "PUT");

	struct curl_slist* chunk = returnAuthHeader();
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

	//{"uris": ["spotify:track:4iV5W9uYEdYUVa79Axb7Rh", "spotify:track:1301WleyT98MSxVHPZCA6M"] }
	std::string postData = "{ \"uris\": [\"spotify:track:" + trackId + "\"] }";
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());

	int rc = curl_easy_perform(curl);
	if (rc != CURLE_OK) {
		curlError(rc);
	}
	curl_easy_cleanup(curl);
	this->curl = NULL;

	if (this->responseBuffer == NULL || *this->responseBuffer != "") {
		std::cout << "[!] Invalid response buffer for resume playback" << std::endl;
		std::cout << *responseBuffer;
		this->apiSync.unlock();
		return -1;
	}

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

int spotify::searchPlaylist(std::string playlist, std::string owner)
{
	this->apiSync.lock();
	assert(this->curl == NULL);

	// curl -X GET "https://api.spotify.com/v1/search?q=bob&type=artist&offset=20&limit=2" -H "Authorization: Bearer {your access token}"
	this->curl = curl_easy_init();

	setCurlResponseBuffer();
	setCurlUri(curl, "https://api.spotify.com/v1/search?q=" + url_encode(playlist) + "&type=playlist", "GET");
	struct curl_slist* chunk = returnAuthHeader();
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

	int rc = curl_easy_perform(curl);
	if (rc != CURLE_OK) {
		curlError(rc);
	}
	curl_easy_cleanup(curl);
	this->curl = NULL;

	rapidjson::Document jsonDoc;
	jsonDoc.Parse(responseBuffer->c_str());
	assert(jsonDoc.HasMember("playlists"));
	const rapidjson::Value& itemArray = jsonDoc["playlists"]["items"];
	assert(itemArray.IsArray());

	// Iterate playlists
	for (rapidjson::SizeType i = 0; i < itemArray.Size(); i++) {
		if (itemArray[i]["name"].GetString() == playlist && 
			itemArray[i]["owner"]["display_name"].GetString() == owner) {
			this->targetPlaylistId = itemArray[i]["id"].GetString();
			this->apiSync.unlock();
			return 0;
		}
	}

	this->apiSync.unlock();
	return -1;
}

const std::vector<spotify::PLAYLIST> *spotify::returnAllPlaylists(std::string playlistQueryString)
{
	this->apiSync.lock();
	assert(this->curl == NULL);

	// curl -X GET "https://api.spotify.com/v1/search?q=bob&type=artist&offset=20&limit=2" -H "Authorization: Bearer {your access token}"
	this->curl = curl_easy_init();

	setCurlResponseBuffer();
	setCurlUri(curl, "https://api.spotify.com/v1/search?q=" + url_encode(playlistQueryString) + "&type=playlist", "GET");
	struct curl_slist* chunk = returnAuthHeader();
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

	int rc = curl_easy_perform(curl);
	if (rc != CURLE_OK) {
		curlError(rc);
	}
	curl_easy_cleanup(curl);
	this->curl = NULL;

	rapidjson::Document jsonDoc;
	jsonDoc.Parse(responseBuffer->c_str());
	assert(jsonDoc.HasMember("playlists"));
	const rapidjson::Value& itemArray = jsonDoc["playlists"]["items"];
	assert(itemArray.IsArray());

	std::vector<spotify::PLAYLIST>* o = new std::vector<spotify::PLAYLIST>();
	for (rapidjson::SizeType i = 0; i < itemArray.Size(); i++) {
		o->push_back(spotify::PLAYLIST{
				itemArray[i]["name"].GetString(),
				itemArray[i]["owner"]["display_name"].GetString(),
				itemArray[i]["id"].GetString(),
				itemArray[i]["href"].GetString()
			});
	}

	this->apiSync.unlock();
	return const_cast<const std::vector<spotify::PLAYLIST>*>(o);
}

int spotify::enumTracksPlaylist(std::string playlistId)
{
	this->apiSync.lock();
	assert(this->curl == NULL);

	// curl -X GET "https://api.spotify.com/v1/playlists/21THa8j9TaSGuXYNBU5tsC/tracks" -H "Authorization: Bearer {your access token}"

	this->curl = curl_easy_init();
	setCurlResponseBuffer();
	setCurlUri(curl, "https://api.spotify.com/v1/playlists/" + playlistId + "/tracks", "GET");
	struct curl_slist* chunk = returnAuthHeader();
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

	int rc = curl_easy_perform(curl);
	if (rc != CURLE_OK) {
		curlError(rc);
	}
	curl_easy_cleanup(curl);
	this->curl = NULL;

	rapidjson::Document jsonDoc;
	jsonDoc.Parse(responseBuffer->c_str());
	assert(jsonDoc.HasMember("items"));
	const rapidjson::Value& itemArray = jsonDoc["items"];
	assert(itemArray.IsArray());
	for (rapidjson::SizeType i = 0; i < itemArray.Size(); i++) {
		const std::string dateAdded = std::string(itemArray[i]["added_at"].GetString());
		std::regex reg("(\\d{4})-\\d{1,2}-\\d{1,2}.*");
		std::smatch m;
		std::regex_search(dateAdded, m, reg);
		std::string* out = NULL;
		for (auto x : m) out = new std::string(x);

		const rapidjson::Value& track = itemArray[i]["track"];
		this->trackList.push_back(TRACK{
				track["id"].GetString(),
				track["artists"][0]["name"].GetString(),
				track["name"].GetString(),
				track["duration_ms"].GetUint(),
				track["album"]["name"].GetString(),
				(unsigned int)std::stoi(*out),
				track["track_number"].GetUint(),
				"unknown"
			});
	}

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

std::string url_encode(const std::string& value) {
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
		std::string::value_type c = (*i);

		// Keep alphanumeric and other accepted characters intact
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			escaped << c;
			continue;
		}

		// Any other characters are percent-encoded
		escaped << std::uppercase;
		escaped << '%' << std::setw(2) << int((unsigned char)c);
		escaped << std::nouppercase;
	}

	return escaped.str();
}