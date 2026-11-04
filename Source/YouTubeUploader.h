#pragma once
#include <JuceHeader.h>

/**
    Minimal YouTube uploader (OAuth 2.0 loopback + PKCE, resumable upload).

    All network work is done by shelling out to curl, so every method that talks
    to the network blocks and must be called from a background thread, never the
    message thread.

    Setup: put youtube_client.json (downloaded from Google Cloud Console, "Desktop app"
    credentials) into getConfigFolder(). connect() then stores a refresh token there.
*/
class YouTubeUploader
{
public:
    struct Result
    {
        bool ok = false;
        juce::String message;   // channel name (connect) / video id (upload) / error text
        juce::String warning;   // non-fatal problems, e.g. thumbnail failed
    };

    YouTubeUploader() = default;

    juce::File getConfigFolder() const;
    bool hasClientConfig() const;
    bool isConnected() const;
    juce::String getChannelName() const;
    void disconnect();

    Result connect();

    Result upload (const juce::File& video,
                   const juce::File& thumbnail,
                   const juce::String& title,
                   const juce::String& description,
                   const juce::StringArray& tags,
                   const juce::String& privacy,
                   std::function<void (int)> onStep = {});

private:
    struct CurlResult
    {
        bool ok = false;
        int status = 0;
        juce::String body;
    };

    juce::File getClientFile() const;
    juce::File getTokenFile() const;
    bool loadClient (juce::String& clientId, juce::String& clientSecret) const;

    CurlResult runCurl (const juce::StringArray& extraArgs);
    CurlResult postForm (const juce::String& url, const juce::String& body);
    juce::String getAccessToken (juce::String& error);

    juce::String uploadThumbnail (const juce::String& videoId, const juce::File& image,
                                  const juce::File& authHeaderFile);
};