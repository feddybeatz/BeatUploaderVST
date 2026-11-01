#include "YouTubeUploader.h"

namespace
{
    juce::String formEncode (const juce::String& s)
    {
        return juce::URL::addEscapeChars (s, true);
    }

    juce::String randomString (int length)
    {
        static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
        auto& rng = juce::Random::getSystemRandom();
        juce::String s;

        for (int i = 0; i < length; ++i)
            s += juce::String::charToString ((juce::juce_wchar) chars[rng.nextInt ((int) sizeof (chars) - 1)]);

        return s;
    }

    // PKCE code challenge: BASE64URL (SHA256 (verifier))
    juce::String pkceChallenge (const juce::String& verifier)
    {
        juce::SHA256 sha (verifier.toRawUTF8(), verifier.getNumBytesAsUTF8());
        auto raw = sha.getRawData();

        return juce::Base64::toBase64 (raw.getData(), raw.getSize())
                   .replace ("+", "-").replace ("/", "_").replace ("=", "");
    }

    juce::String getQueryParam (const juce::URL& url, const juce::String& name)
    {
        auto names  = url.getParameterNames();
        auto values = url.getParameterValues();
        auto index  = names.indexOf (name);

        return index >= 0 ? values[index] : juce::String();
    }

    juce::String describeError (const juce::String& body, int status)
    {
        auto json = juce::JSON::parse (body);
        auto message = json["error"]["message"].toString();

        if (message.isEmpty())
            message = json["error_description"].toString();

        if (message.isEmpty())
            message = body.trim().substring (0, 200);

        if (status == 0 && message.isEmpty())
            message = "no response";

        return "HTTP " + juce::String (status) + ": " + message;
    }
}

//==============================================================================
juce::File YouTubeUploader::getConfigFolder() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("FeddyBeatz")
                   .getChildFile ("BeatUploader");
    dir.createDirectory();
    return dir;
}

juce::File YouTubeUploader::getClientFile() const { return getConfigFolder().getChildFile ("youtube_client.json"); }
juce::File YouTubeUploader::getTokenFile()  const { return getConfigFolder().getChildFile ("youtube_token.json"); }

bool YouTubeUploader::loadClient (juce::String& clientId, juce::String& clientSecret) const
{
    // accepts the file exactly as downloaded from Google Cloud ({"installed": {...}}) or a flat {"client_id", "client_secret"}
    auto json = juce::JSON::parse (getClientFile());
    auto node = json.hasProperty ("installed") ? json["installed"] : json;

    clientId     = node["client_id"].toString();
    clientSecret = node["client_secret"].toString();

    return clientId.isNotEmpty() && clientSecret.isNotEmpty();
}

bool YouTubeUploader::hasClientConfig() const
{
    juce::String id, secret;
    return loadClient (id, secret);
}

bool YouTubeUploader::isConnected() const
{
    return juce::JSON::parse (getTokenFile())["refresh_token"].toString().isNotEmpty();
}

juce::String YouTubeUploader::getChannelName() const
{
    return juce::JSON::parse (getTokenFile())["channel_name"].toString();
}

void YouTubeUploader::disconnect()
{
    getTokenFile().deleteFile();
}

//==============================================================================
YouTubeUploader::CurlResult YouTubeUploader::runCurl (const juce::StringArray& extraArgs)
{
    CurlResult r;

    juce::StringArray args;
    args.add ("curl.exe");
    args.add ("-s");
    args.add ("-S");
    args.add ("-w");
    args.add ("<<STATUS>>%{http_code}");
    args.addArray (extraArgs);

    juce::ChildProcess proc;

    if (! proc.start (args))
    {
        r.body = "Could not start curl.exe (it ships with Windows 10 and 11).";
        return r;
    }

    auto out = proc.readAllProcessOutput();
    proc.waitForProcessToFinish (60 * 60 * 1000);

    const auto marker = out.lastIndexOf ("<<STATUS>>");

    if (marker >= 0)
    {
        r.status = out.substring (marker + 10).getIntValue();
        r.body   = out.substring (0, marker);
    }
    else
    {
        r.body = out;
    }

    r.ok = r.status >= 200 && r.status < 300;
    return r;
}

YouTubeUploader::CurlResult YouTubeUploader::postForm (const juce::String& url, const juce::String& body)
{
    juce::TemporaryFile bodyFile (".txt");
    bodyFile.getFile().replaceWithText (body);

    juce::StringArray args;
    args.add ("-X");              args.add ("POST");
    args.add (url);
    args.add ("-H");              args.add ("Content-Type: application/x-www-form-urlencoded");
    args.add ("--data-binary");   args.add ("@" + bodyFile.getFile().getFullPathName());

    return runCurl (args);
}

juce::String YouTubeUploader::getAccessToken (juce::String& error)
{
    juce::String clientId, clientSecret;

    if (! loadClient (clientId, clientSecret))
    {
        error = "youtube_client.json is missing in " + getConfigFolder().getFullPathName();
        return {};
    }

    auto refresh = juce::JSON::parse (getTokenFile())["refresh_token"].toString();

    if (refresh.isEmpty())
    {
        error = "No channel connected yet";
        return {};
    }

    auto res = postForm ("https://oauth2.googleapis.com/token",
                         "client_id=" + formEncode (clientId)
                         + "&client_secret=" + formEncode (clientSecret)
                         + "&refresh_token=" + formEncode (refresh)
                         + "&grant_type=refresh_token");

    auto access = juce::JSON::parse (res.body)["access_token"].toString();

    if (! res.ok || access.isEmpty())
    {
        error = describeError (res.body, res.status) + " (use Switch Channel to sign in again)";
        return {};
    }

    return access;
}

//==============================================================================
YouTubeUploader::Result YouTubeUploader::connect()
{
    Result r;
    juce::String clientId, clientSecret;

    if (! loadClient (clientId, clientSecret))
    {
        r.message = "Put youtube_client.json in " + getConfigFolder().getFullPathName();
        return r;
    }

    const auto verifier  = randomString (64);
    const auto challenge = pkceChallenge (verifier);
    const auto state     = randomString (24);

    // tiny local web server that catches Google's redirect after you sign in
    juce::StreamingSocket server;

    if (! server.createListener (0, "127.0.0.1"))
    {
        r.message = "Could not open a local port for the sign-in callback.";
        return r;
    }

    const int port = server.getBoundPort();

    if (port <= 0)
    {
        r.message = "Could not find the local sign-in port.";
        return r;
    }

    const auto redirect = "http://127.0.0.1:" + juce::String (port);

    juce::URL authUrl ("https://accounts.google.com/o/oauth2/v2/auth");
    authUrl = authUrl.withParameter ("client_id", clientId)
                     .withParameter ("redirect_uri", redirect)
                     .withParameter ("response_type", "code")
                     .withParameter ("scope", "https://www.googleapis.com/auth/youtube.upload "
                                              "https://www.googleapis.com/auth/youtube.readonly")
                     .withParameter ("code_challenge", challenge)
                     .withParameter ("code_challenge_method", "S256")
                     .withParameter ("state", state)
                     .withParameter ("access_type", "offline")
                     .withParameter ("prompt", "consent select_account");

    authUrl.launchInDefaultBrowser();

    juce::String code, authError;
    bool gotCallback = false;
    const auto deadline = juce::Time::getMillisecondCounter() + 180000u;

    while (! gotCallback && juce::Time::getMillisecondCounter() < deadline)
    {
        if (server.waitUntilReady (true, 1000) != 1)
            continue;

        std::unique_ptr<juce::StreamingSocket> client (server.waitForNextConnection());

        if (client == nullptr)
            continue;

        juce::String request;
        char buffer[4096];

        while (client->waitUntilReady (true, 3000) == 1)
        {
            auto n = client->read (buffer, (int) sizeof (buffer), false);

            if (n <= 0)
                break;

            request += juce::String::fromUTF8 (buffer, n);

            if (request.contains ("\r\n\r\n") || request.length() > 16384)
                break;
        }

        auto firstLine = request.upToFirstOccurrenceOf ("\r\n", false, false);
        auto path = firstLine.fromFirstOccurrenceOf ("GET ", false, false)
                             .upToLastOccurrenceOf (" HTTP", false, false).trim();

        juce::URL callback ("http://127.0.0.1" + path);
        auto gotCode  = getQueryParam (callback, "code");
        auto gotError = getQueryParam (callback, "error");
        auto gotState = getQueryParam (callback, "state");

        const bool isCallback = gotCode.isNotEmpty() || gotError.isNotEmpty();

        juce::String html = isCallback
            ? "<html><body style=\"font-family:sans-serif;background:#050806;color:#c8ff2c;text-align:center;padding-top:80px\">"
              "<h2>Sign-in finished</h2><p>You can close this tab and go back to FL Studio.</p></body></html>"
            : juce::String();

        juce::String response = juce::String (isCallback ? "HTTP/1.1 200 OK" : "HTTP/1.1 404 Not Found")
                              + "\r\nContent-Type: text/html; charset=utf-8"
                              + "\r\nConnection: close"
                              + "\r\nContent-Length: " + juce::String ((int) html.getNumBytesAsUTF8())
                              + "\r\n\r\n" + html;

        client->write (response.toRawUTF8(), (int) response.getNumBytesAsUTF8());
        client->close();

        if (! isCallback)
            continue;

        gotCallback = true;

        if (gotError.isNotEmpty())
            authError = "Google said: " + gotError;
        else if (gotState != state)
            authError = "Sign-in response did not match (try again).";
        else
            code = gotCode;
    }

    server.close();

    if (code.isEmpty())
    {
        r.message = authError.isNotEmpty() ? authError : "Sign-in timed out.";
        return r;
    }

    // trade the one-time code for tokens
    auto tokenRes = postForm ("https://oauth2.googleapis.com/token",
                              "code=" + formEncode (code)
                              + "&client_id=" + formEncode (clientId)
                              + "&client_secret=" + formEncode (clientSecret)
                              + "&code_verifier=" + formEncode (verifier)
                              + "&redirect_uri=" + formEncode (redirect)
                              + "&grant_type=authorization_code");

    auto tokenJson = juce::JSON::parse (tokenRes.body);
    auto accessToken  = tokenJson["access_token"].toString();
    auto refreshToken = tokenJson["refresh_token"].toString();

    if (! tokenRes.ok || accessToken.isEmpty())
    {
        r.message = describeError (tokenRes.body, tokenRes.status);
        return r;
    }

    if (refreshToken.isEmpty())
    {
        r.message = "Google did not return a refresh token. Try Connect Channel again.";
        return r;
    }

    // which channel did you pick?
    juce::TemporaryFile authFile (".txt");
    authFile.getFile().replaceWithText ("Authorization: Bearer " + accessToken);

    juce::StringArray chanArgs;
    chanArgs.add ("-H");
    chanArgs.add ("@" + authFile.getFile().getFullPathName());
    chanArgs.add ("https://www.googleapis.com/youtube/v3/channels?part=snippet&mine=true");

    auto chan = runCurl (chanArgs);
    auto channelName = juce::JSON::parse (chan.body)["items"][0]["snippet"]["title"].toString();

    if (! chan.ok || channelName.isEmpty())
    {
        r.message = chan.ok ? juce::String ("No YouTube channel found on that Google account.")
                            : describeError (chan.body, chan.status);
        return r;
    }

    auto* saved = new juce::DynamicObject();
    saved->setProperty ("refresh_token", refreshToken);
    saved->setProperty ("channel_name", channelName);
    getTokenFile().replaceWithText (juce::JSON::toString (juce::var (saved)));

    r.ok = true;
    r.message = channelName;
    return r;
}

//==============================================================================
juce::String YouTubeUploader::uploadThumbnail (const juce::String& videoId, const juce::File& image,
                                                const juce::File& authHeaderFile)
{
    const juce::String mime = image.hasFileExtension ("png") ? "image/png" : "image/jpeg";

    juce::StringArray args;
    args.add ("-X");              args.add ("POST");
    args.add ("https://www.googleapis.com/upload/youtube/v3/thumbnails/set?videoId=" + videoId);
    args.add ("-H");              args.add ("@" + authHeaderFile.getFullPathName());
    args.add ("-H");              args.add ("Content-Type: " + mime);
    args.add ("--data-binary");   args.add ("@" + image.getFullPathName());

    auto res = runCurl (args);
    return res.ok ? juce::String() : describeError (res.body, res.status);
}

YouTubeUploader::Result YouTubeUploader::upload (const juce::File& video,
                                                 const juce::File& thumbnail,
                                                 const juce::String& title,
                                                 const juce::String& description,
                                                 const juce::StringArray& tags,
                                                 const juce::String& privacy)
{
    Result r;
    if (! video.existsAsFile())
    {
        r.message = "Video file not found.";
        return r;
    }

    juce::String error;
    const auto access = getAccessToken (error);
    if (access.isEmpty())
    {
        r.message = error;
        return r;
    }

    juce::TemporaryFile authFile (".txt"), metaFile (".json"), headerDump (".txt");
    authFile.getFile().replaceWithText ("Authorization: Bearer " + access);

    juce::Array<juce::var> tagArray;
    for (auto& tag : tags)
        tagArray.add (tag);

    auto* snippet = new juce::DynamicObject();
    snippet->setProperty ("title", title);
    snippet->setProperty ("description", description);
    snippet->setProperty ("tags", tagArray);
    snippet->setProperty ("categoryId", "10");

    auto* status = new juce::DynamicObject();
    status->setProperty ("privacyStatus", privacy);
    status->setProperty ("selfDeclaredMadeForKids", false);

    auto* root = new juce::DynamicObject();
    root->setProperty ("snippet", juce::var (snippet));
    root->setProperty ("status", juce::var (status));
    metaFile.getFile().replaceWithText (juce::JSON::toString (juce::var (root), true));

    juce::StringArray initArgs;
    initArgs.add ("-D");   initArgs.add (headerDump.getFile().getFullPathName());
    initArgs.add ("-X");   initArgs.add ("POST");
    initArgs.add ("https://www.googleapis.com/upload/youtube/v3/videos?uploadType=resumable&part=snippet,status");
    initArgs.add ("-H");   initArgs.add ("@" + authFile.getFile().getFullPathName());
    initArgs.add ("-H");   initArgs.add ("Content-Type: application/json; charset=UTF-8");
    initArgs.add ("-H");   initArgs.add ("X-Upload-Content-Type: video/mp4");
    initArgs.add ("-H");   initArgs.add ("X-Upload-Content-Length: " + juce::String (video.getSize()));
    initArgs.add ("--data-binary");
    initArgs.add ("@" + metaFile.getFile().getFullPathName());

    auto init = runCurl (initArgs);
    if (! init.ok)
    {
        r.message = describeError (init.body, init.status);
        return r;
    }

    juce::String sessionUrl;
    juce::StringArray headerLines;
    headerLines.addLines (headerDump.getFile().loadFileAsString());
    for (auto& line : headerLines)
        if (line.startsWithIgnoreCase ("location:"))
            sessionUrl = line.fromFirstOccurrenceOf (":", false, false).trim();

    if (sessionUrl.isEmpty())
    {
        r.message = "YouTube did not return an upload address.";
        return r;
    }

    juce::StringArray putArgs;
    putArgs.add ("-X");   putArgs.add ("PUT");
    putArgs.add (sessionUrl);
    putArgs.add ("-H");   putArgs.add ("@" + authFile.getFile().getFullPathName());
    putArgs.add ("-H");   putArgs.add ("Content-Type: video/mp4");
    putArgs.add ("-T");   putArgs.add (video.getFullPathName());

    auto put = runCurl (putArgs);
    auto videoId = juce::JSON::parse (put.body)["id"].toString();
    if (! put.ok || videoId.isEmpty())
    {
        r.message = describeError (put.body, put.status);
        return r;
    }

    if (thumbnail.existsAsFile())
    {
        const auto thumbError = uploadThumbnail (videoId, thumbnail, authFile.getFile());
        if (thumbError.isNotEmpty())
            r.warning = "thumbnail not set (" + thumbError + ")";
    }

    r.ok = true;
    r.message = videoId;
    return r;
}
