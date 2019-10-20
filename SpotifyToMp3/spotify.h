#pragma once

#include "cpprest/http_client.h"
#include "cpprest/http_listener.h"


class spotify
{
public:
	std::string clientId;
	std::string clientSecret;

	spotify(std::string clientId, std::string clientSecret) :
		clientId(clientId),
		clientSecret(clientSecret)
	{

	}


};

