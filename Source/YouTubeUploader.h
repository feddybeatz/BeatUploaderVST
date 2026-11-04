#pragma once
#include <JuceHeader.h>

class YouTubeUploader
{
public:
    struct Result
    {
        bool ok = false;
        juce::String message;
        juce::String warning;
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
