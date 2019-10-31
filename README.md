# SpotifyToMp3
Uses the Spotify Web API to control playback on a certain device (i.e. local Spotify player), binds to an audio device using WASAPI / PortAudio and records stream to MP3 using LAME encoder.

## HowTo
This application will allow you to search for playlists, enumerate the tracks, and write them to an MP3. It is highly recommended to set 320kbps on the Spotify player (requires premium). 

Make sure to set ```defaultSpotifyDevice```. It's recommended to use something like VB-Audio Cable to route audio signals from Spotify directly into the SpotifyToMp3.

Compile in x86 to prevent issues with LAME encoder. 

Spotify API uses Oauth2 to identify endpoints, so use another script to authenticate and grab the refresh token, and simply pass it into the config file. This app doesn't contain code for the entire Oauth2 flow.

Oauth2 access tokens are refreshed in a side thread every ```accessTokenRefresh``` minutes. 15 minutes should suffice.

## Config.ini Sample
```
[api]
mode=search

clientId=<client ID>
clientSecret=<client Secret>

refreshToken=<refresh token, can obtain this using numberous scripts>

# Time (in minutes) to refresh access token
accessTokenRefresh=15

# Playlist configuration
playlistSearch=Melodic Techno
playlistAuthor=Silhouettes
playlistTrackOffset=0

# Default spotify device
defaultSpotifyDevice=<device hostname>

# Input device like Virtual Cable works best
defaultAudioDevice=CABLE Output
```
