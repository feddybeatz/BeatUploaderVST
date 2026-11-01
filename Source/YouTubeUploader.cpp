#include "YouTubeUploader.h"

namespace
{
    const juce::String tokenUrl      { "https://oauth2.googleapis.com/token" };
    const juce::String authUrl       { "https://accounts.google.com/o/oauth2/v2/auth" };
    const juce::String channelUrl    { "https://www.googleapis.com/youtube/v3/channels?part=snippet&mine=true" };
    const juce::String uploadInitUrl { "https://www.googleapis.com/upload/youtube/v3/videos?uploadType=resumable&part=snippet,status" };

    juce::String jsonString (const juce::var& object, const juce::Identifier& key)
    {
        if (auto* o = object.getDynamicObject())
            return o->getProperty (key).toString();

        return {};
    }

    juce::String googleError (const juce::String& body)
    {
        auto parsed = juce::JSON::parse (body);

        if (auto* root = parsed.getDynamicObject())
        {
            const auto description = root->getProperty ("error_description").toString();

            if (description.isNotEmpty())
                return description;

            const auto error = root->getProperty ("error");

            if (auto* errorObj = error.getDynamicObject())
            {
                const auto message = errorObj->getProperty ("message").toString();

                if (message.isNotEmpty())
                    return message;
            }

            if (error.isString() && error.toString().isNotEmpty())
                return error.toString();
        }

        auto trimmed = body.trim();
        return trimmed.isEmpty() ? juce::String ("Unknown YouTube error")
                                 : trimmed.substring (0, 300);
    }

    juce::String headerValue (const juce::String& headers, const juce::String& name)
    {
        auto lines = juce::StringArray::fromLines (headers.replace ("\r\n", "\n"));

        for (auto& line : lines)
        {
            if (line.startsWithIgnoreCase (name + ":"))
                return line.fromFirstOccurrenceOf (":", false, false).trim();
        }

        return {};
    }

    juce::String formEncode (const juce::StringPairArray& fields)
    {
        juce::String body;

        for (int i = 0; i < fields.size(); ++i)
        {
            if (i > 0)
                body << "&";

            body << juce::URL::addEscapeChars (fields.getAllKeys()[i], true) << "="
                 << juce::URL::addEscapeChars (fields.getAllValues()[i], true);
        }

        return body;
    }

    juce::String queryValue (const juce::String& query, const juce::String& key)
    {
        for (auto& part : juce::StringArray::fromTokens (query, "&", {}))
        {
            if (part.upToFirstOccurrenceOf ("=", false, false) == key)
                return juce::URL::removeEscapeChars (part.fromFirstOccurrenceOf ("=", false, false)
                                                        .replace ("+", " "));
        }

        return {};
    }

    juce::File curlExecutable()
    {
        juce::File systemCurl ("C:\\Windows\\System32\\curl.exe");
        return systemCurl.existsAsFile() ? systemCurl : juce::File();
    }
}

juce::File YouTubeUploader::getConfigFolder() const
{
    auto folder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                    .getChildFile ("FeddyBeatz")
                    .getChildFile ("BeatUploader");
    folder.createDirectory();
    return folder;
}

juce::File YouTubeUploader::getClientFile() const
{
    return getConfigFolder().getChildFile ("youtube_client.json");
}

juce::File YouTubeUploader::getTokenFile() const
{
    return getConfigFolder().getChildFile ("youtube_tokens.json");
}

bool YouTubeUploader::loadClient (juce::String& clientId, juce::String& clientSecret) const
{
    auto parsed = juce::JSON::parse (getClientFile().loadFileAsString());
    auto* root = parsed.getDynamicObject();

    if (root == nullptr)
        return false;

    auto section = root->getProperty ("installed");

    if (section.isVoid())
        section = root->getProperty ("web");

    auto* obj = section.getDynamicObject();

    if (obj == nullptr)
        return false;

    clientId     = obj->getProperty ("client_id").toString().trim();
    clientSecret = obj->getProperty ("client_secret").toString().trim();
    return clientId.isNotEmpty();
}

bool YouTubeUploader::hasClientConfig() const
{
    juce::String id, secret;
    return loadClient (id, secret);
}

bool YouTubeUploader::isConnected() const
{
    auto parsed = juce::JSON::parse (getTokenFile().loadFileAsString());
    return jsonString (parsed, "refresh_token").isNotEmpty();
}

juce::String YouTubeUploader::getChannelName() const
{
    return jsonString (juce::JSON::parse (getTokenFile().loadFileAsString()), "channel_name");
}

void YouTubeUploader::disconnect()
{
    getTokenFile().deleteFile();
}

YouTubeUploader::CurlResult YouTubeUploader::runCurl (const juce::StringArray& extraArgs, int timeoutMs)
{
    auto stamp = juce::Uuid().toDashedString();
    auto headerFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                        .getChildFile ("beatuploader-headers-" + stamp + ".txt");
    auto bodyFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                        .getChildFile ("beatuploader-body-" + stamp + ".txt");
    auto errorFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("beatuploader-curl-error-" + stamp + ".txt");

    headerFile.deleteFile();
    bodyFile.deleteFile();
    errorFile.deleteFile();

    juce::StringArray args;
    auto curl = curlExecutable();
    args.add (curl.existsAsFile() ? curl.getFullPathName() : juce::String ("curl.exe"));
    args.add ("-sS");
    args.add ("-D"); args.add (headerFile.getFullPathName());
    args.add ("-o"); args.add (bodyFile.getFullPathName());
    args.add ("--stderr"); args.add (errorFile.getFullPathName());
    args.add ("-w"); args.add ("\\n%{http_code}");
    args.addArray (extraArgs);

    CurlResult result;
    juce::ChildProcess proc;

    if (! proc.start (args, juce::ChildProcess::wantStdOut))
    {
        result.body = "Could not start curl.exe. It should be at C:\\Windows\\System32\\curl.exe.";
        errorFile.deleteFile();
        return result;
    }

    auto codeText = proc.readAllProcessOutput();
    proc.waitForProcessToFinish (timeoutMs);

    result.status  = codeText.trim().fromLastOccurrenceOf ("\n", false, false).trim().getIntValue();
    result.body    = bodyFile.loadFileAsString();
    result.headers = headerFile.loadFileAsString();
    result.curlError = errorFile.loadFileAsString().trim();
    result.ok      = result.status >= 200 && result.status < 300;

    headerFile.deleteFile();
    bodyFile.deleteFile();
    errorFile.deleteFile();
    return result;
}

YouTubeUploader::CurlResult YouTubeUploader::postForm (const juce::String& url, const juce::String& body)
{
    return runCurl ({ "-X", "POST",
                      "-H", "Content-Type: application/x-www-form-urlencoded",
                      "--data", body,
                      url });
}

bool YouTubeUploader::saveTokens (const juce::var& tokenJson, const juce::String& channelName) const
{
    auto* incoming = tokenJson.getDynamicObject();

    if (incoming == nullptr)
        return false;

    auto existing = juce::JSON::parse (getTokenFile().loadFileAsString());
    auto* out = new juce::DynamicObject();

    const auto access  = incoming->getProperty ("access_token").toString();
    auto refresh       = incoming->getProperty ("refresh_token").toString();

    if (refresh.isEmpty())
        refresh = jsonString (existing, "refresh_token");

    const auto expiresIn = (juce::int64) incoming->getProperty ("expires_in");
    const auto expiryMs  = juce::Time::currentTimeMillis()
                         + juce::jmax ((juce::int64) 60, expiresIn) * 1000;

    out->setProperty ("access_token",  access);
    out->setProperty ("refresh_token", refresh);
    out->setProperty ("expiry_ms",     expiryMs);
    out->setProperty ("channel_name",  channelName.isNotEmpty() ? channelName
                                                                : jsonString (existing, "channel_name"));

    return getTokenFile().replaceWithText (juce::JSON::toString (juce::var (out), true));
}

juce::String YouTubeUploader::getAccessToken (juce::String& error)
{
    auto stored = juce::JSON::parse (getTokenFile().loadFileAsString());
    const auto access   = jsonString (stored, "access_token");
    const auto refresh  = jsonString (stored, "refresh_token");
    const auto expiryMs = (juce::int64) stored.getProperty ("expiry_ms", 0);

    if (access.isNotEmpty() && juce::Time::currentTimeMillis() + 60000 < expiryMs)
        return access;

    if (refresh.isEmpty())
    {
        error = "YouTube is not connected. Press Connect YouTube Channel first.";
        return {};
    }

    juce::String clientId, clientSecret;

    if (! loadClient (clientId, clientSecret))
    {
        error = "Missing youtube_client.json in " + getConfigFolder().getFullPathName();
        return {};
    }

    juce::StringPairArray fields;
    fields.set ("client_id",     clientId);
    fields.set ("client_secret", clientSecret);
    fields.set ("refresh_token", refresh);
    fields.set ("grant_type",    "refresh_token");

    auto result = postForm (tokenUrl, formEncode (fields));

    if (! result.ok)
    {
        error = "Could not refresh YouTube login: " + googleError (result.body)
              + ". Disconnect and connect again.";
        return {};
    }

    auto parsed = juce::JSON::parse (result.body);
    const auto name = jsonString (stored, "channel_name");
    saveTokens (parsed, name);
    const auto next = jsonString (parsed, "access_token");

    if (next.isEmpty())
    {
        error = "YouTube did not return an access token.";
        return {};
    }

    return next;
}

juce::String YouTubeUploader::fetchChannelName (const juce::String& accessToken, juce::String& error)
{
    auto result = runCurl ({ "-H", "Authorization: Bearer " + accessToken, channelUrl });

    if (! result.ok)
    {
        error = googleError (result.body);
        return {};
    }

    auto parsed = juce::JSON::parse (result.body);
    auto* root = parsed.getDynamicObject();

    if (root == nullptr)
        return {};

    if (auto* items = root->getProperty ("items").getArray(); items != nullptr && ! items->isEmpty())
        if (auto* item = items->getFirst().getDynamicObject())
            if (auto* snippet = item->getProperty ("snippet").getDynamicObject())
                return snippet->getProperty ("title").toString();

    error = "That Google account has no YouTube channel.";
    return {};
}

juce::String YouTubeUploader::waitForAuthCode (juce::StreamingSocket& listener,
                                               const juce::String& expectedState,
                                               juce::String& error)
{
    const auto deadline = juce::Time::getMillisecondCounterHiRes() + 180000.0;
    juce::String code;

    while (juce::Time::getMillisecondCounterHiRes() < deadline && code.isEmpty())
    {
        if (listener.waitUntilReady (true, 500) != 1)
            continue;

        std::unique_ptr<juce::StreamingSocket> client (listener.waitForNextConnection());

        if (client == nullptr)
            continue;

        juce::MemoryOutputStream request;
        char buffer[1024];

        for (int i = 0; i < 40; ++i)
        {
            if (client->waitUntilReady (true, 250) != 1)
                continue;

            const int n = client->read (buffer, (int) sizeof (buffer), false);

            if (n <= 0)
                break;

            request.write (buffer, n);

            if (request.toString().contains ("\r\n\r\n"))
                break;
        }

        const auto firstLine = request.toString().upToFirstOccurrenceOf ("\r\n", false, false);
        const auto path = firstLine.fromFirstOccurrenceOf (" ", false, false)
                                   .upToFirstOccurrenceOf (" ", false, false);
        const auto query = path.fromFirstOccurrenceOf ("?", false, false);

        juce::String page = "<html><body style='background:#050806;color:#C8FF2C;"
                            "font-family:sans-serif;padding:40px'>"
                            "<h2>Beat Uploader</h2><p>You can close this tab and return to the plugin.</p>"
                            "</body></html>";

        if (path.startsWith ("/favicon"))
        {
            page = {};
        }
        else if (const auto denied = queryValue (query, "error"); denied.isNotEmpty())
        {
            error = "Google sign-in was cancelled.";
            page  = "<html><body style='font-family:sans-serif;padding:40px'>Sign-in cancelled.</body></html>";
        }
        else if (query.contains ("code=") || query.contains ("error=") || query.contains ("state="))
        {
            const auto state = queryValue (query, "state");
            const auto found = queryValue (query, "code");

            if (state != expectedState)
                error = "YouTube sign-in failed (bad state). Try Connect again.";
            else if (found.isEmpty())
                error = "Google did not return an auth code.";
            else
                code = found;
        }

        const auto response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html; charset=utf-8\r\n"
                              "Connection: close\r\n"
                              "Content-Length: " + juce::String (page.getNumBytesAsUTF8()) + "\r\n\r\n"
                              + page;
        const auto utf8 = response.toRawUTF8();
        client->write (utf8, (int) strlen (utf8));

        if (error.isNotEmpty())
            return {};
    }

    if (code.isEmpty() && error.isEmpty())
        error = "Timed out waiting for Google sign-in (3 minutes).";

    return code;
}

YouTubeUploader::Result YouTubeUploader::connect()
{
    Result result;
    juce::String clientId, clientSecret;

    if (! loadClient (clientId, clientSecret))
    {
        result.message = "Put your Google OAuth Desktop-app JSON here:\n"
                       + getClientFile().getFullPathName();
        return result;
    }

    juce::StreamingSocket listener;

    // Let the OS choose an available loopback port to avoid conflicts with other apps
    // or other instances of the plugin. Bind before opening the browser to avoid a race.
    if (! listener.createListener (0, "127.0.0.1"))
    {
        result.message = "Could not open a local sign-in callback. Restart the DAW and try again.";
        return result;
    }

    const int port = listener.getBoundPort();

    if (port <= 0)
    {
        result.message = "Could not determine the local sign-in callback port.";
        return result;
    }

    const auto redirect = "http://127.0.0.1:" + juce::String (port);
    const auto state = juce::Uuid().toDashedString();
    const auto scope = "https://www.googleapis.com/auth/youtube.upload "
                       "https://www.googleapis.com/auth/youtube.readonly";

    auto url = juce::URL (authUrl)
                 .withParameter ("client_id",     clientId)
                 .withParameter ("redirect_uri",  redirect)
                 .withParameter ("response_type", "code")
                 .withParameter ("scope",         scope)
                 .withParameter ("access_type",   "offline")
                 .withParameter ("prompt",        "consent select_account")
                 .withParameter ("state",         state);

    if (! url.launchInDefaultBrowser())
    {
        result.message = "Could not open the browser for Google sign-in.";
        return result;
    }

    juce::String error;
    const auto code = waitForAuthCode (listener, state, error);

    if (code.isEmpty())
    {
        result.message = error;
        return result;
    }

    juce::StringPairArray fields;
    fields.set ("code",          code);
    fields.set ("client_id",     clientId);
    fields.set ("client_secret", clientSecret);
    fields.set ("redirect_uri",  redirect);
    fields.set ("grant_type",    "authorization_code");

    auto tokenResult = postForm (tokenUrl, formEncode (fields));

    if (! tokenResult.ok)
    {
        result.message = "Google token exchange failed: " + googleError (tokenResult.body);
        return result;
    }

    auto tokens = juce::JSON::parse (tokenResult.body);
    const auto access = jsonString (tokens, "access_token");

    if (access.isEmpty())
    {
        result.message = "Google did not return an access token.";
        return result;
    }

    auto channel = fetchChannelName (access, error);
    saveTokens (tokens, channel);

    if (jsonString (tokens, "refresh_token").isEmpty()
        && jsonString (juce::JSON::parse (getTokenFile().loadFileAsString()), "refresh_token").isEmpty())
    {
        result.message = "Google did not return a refresh token. Disconnect and connect again.";
        return result;
    }

    if (channel.isEmpty())
        channel = "YouTube channel";

    result.ok = true;
    result.message = channel;
    return result;
}

YouTubeUploader::Result YouTubeUploader::upload (const juce::File& video,
                                                 const juce::String& title,
                                                 const juce::String& description,
                                                 const juce::StringArray& tags,
                                                 const juce::String& privacy)
{
    Result result;

    if (! video.existsAsFile())
    {
        result.message = "Video file is missing.";
        return result;
    }

    juce::String error;
    const auto access = getAccessToken (error);

    if (access.isEmpty())
    {
        result.message = error;
        return result;
    }

    auto* snippet = new juce::DynamicObject();
    snippet->setProperty ("title",       title.substring (0, 100));
    snippet->setProperty ("description", description.substring (0, 5000));
    snippet->setProperty ("categoryId",  "10");

    juce::Array<juce::var> tagVars;
    int tagChars = 0;

    for (auto& tag : tags)
    {
        const auto trimmed = tag.trim();

        if (trimmed.isEmpty())
            continue;

        tagChars += trimmed.length() + 1;

        if (tagChars > 490)
            break;

        tagVars.add (trimmed);
    }

    snippet->setProperty ("tags", tagVars);

    auto* status = new juce::DynamicObject();
    status->setProperty ("privacyStatus", privacy.isEmpty() ? juce::String ("unlisted") : privacy);
    status->setProperty ("selfDeclaredMadeForKids", false);

    auto* root = new juce::DynamicObject();
    root->setProperty ("snippet", juce::var (snippet));
    root->setProperty ("status",  juce::var (status));

    auto metaFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("beatuploader-meta-" + juce::Uuid().toDashedString() + ".json");
    metaFile.replaceWithText (juce::JSON::toString (juce::var (root), true));

    auto init = runCurl ({
        "-X", "POST",
        "-H", "Authorization: Bearer " + access,
        "-H", "Content-Type: application/json; charset=UTF-8",
        "-H", "X-Upload-Content-Length: " + juce::String (video.getSize()),
        "-H", "X-Upload-Content-Type: video/mp4",
        "--data-binary", "@" + metaFile.getFullPathName(),
        uploadInitUrl
    });

    metaFile.deleteFile();

    if (! init.ok)
    {
        auto detail = googleError (init.body);

        if (init.body.trim().isEmpty())
            detail = init.curlError.isNotEmpty() ? init.curlError : "The API returned an empty error response.";

        result.message = "YouTube rejected the upload (HTTP " + juce::String (init.status) + "): " + detail;
        return result;
    }

    const auto location = headerValue (init.headers, "Location");

    if (location.isEmpty())
    {
        result.message = "YouTube did not return an upload URL.";
        return result;
    }

    auto put = runCurl ({
        "-X", "PUT",
        "-H", "Authorization: Bearer " + access,
        "-H", "Content-Type: video/mp4",
        "-T", video.getFullPathName(),
        location
    }, 60 * 60 * 1000);

    if (! put.ok)
    {
        auto detail = googleError (put.body);

        if (put.body.trim().isEmpty())
            detail = put.curlError.isNotEmpty() ? put.curlError : "The API returned an empty error response.";

        result.message = "Video transfer failed (HTTP " + juce::String (put.status) + "): " + detail;
        return result;
    }

    const auto videoId = jsonString (juce::JSON::parse (put.body), "id");

    if (videoId.isEmpty())
    {
        result.message = "Upload finished but YouTube did not return a video id.";
        return result;
    }

    result.ok = true;
    result.message = videoId;
    return result;
}
