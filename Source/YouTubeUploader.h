#pragma once
#include <JuceHeader.h>

// Uploads videos to YouTube through the Data API.
// Uses Windows' built-in curl.exe for the HTTPS calls, so no extra libraries are needed.
//
// Setup (once): put the OAuth "Desktop app" JSON you download from Google Cloud Console at
//   %APPDATA%\FeddyBeatz\BeatUploader\youtube_client.json
// Every blocking call below must run on a background thread, never the UI thread.
class YouTubeUploader
{
public:
    struct Result
    {
        bool ok = false;
        juce::String message;   // on success: channel name (connect) or video id (upload); otherwise the error
    };

    YouTubeUploader() = default;

    juce::File getConfigFolder() const;
    bool hasClientConfig() const;           // youtube_client.json present and valid
    bool isConnected() const;               // a channel has been connected before
    juce::String getChannelName() const;
    void disconnect();

    // Opens the browser for Google sign-in and waits for the result (up to 3 minutes).
    // Whichever channel you pick in the Google channel chooser becomes the upload target.
    Result connect();

    // Uploads the video. privacy: "private", "unlisted" or "public". On success, message = video id.
    Result upload (const juce::File& video,
                   const juce::String& title,
                   const juce::String& description,
                   const juce::StringArray& tags,
                   const juce::String& privacy);

private:
    struct CurlResult
    {
        bool ok = false;
        int status = 0;
        juce::String body;
        juce::String headers;
        juce::String curlError;
    };

    juce::File getClientFile() const;
    juce::File getTokenFile() const;
    bool loadClient (juce::String& clientId, juce::String& clientSecret) const;

    CurlResult runCurl (const juce::StringArray& extraArgs, int timeoutMs = 60000);
    CurlResult postForm (const juce::String& url, const juce::String& body);
    juce::String getAccessToken (juce::String& error);

    bool saveTokens (const juce::var& tokenJson, const juce::String& channelName) const;
    juce::String fetchChannelName (const juce::String& accessToken, juce::String& error);
    juce::String waitForAuthCode (juce::StreamingSocket& listener,
                                  const juce::String& expectedState,
                                  juce::String& error);
};
