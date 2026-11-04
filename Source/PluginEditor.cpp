#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::String uploadPrivacy ("private");
    const bool videoFillsFrame = true;

    juce::String cleanName (const juce::String& raw)
    {
        auto name = raw.trim();
        return name.isEmpty() ? juce::String ("Beat Name") : name;
    }

    juce::String makeTitle (const juce::String& rawName)
    {
        auto title = "(FREE) Kehlani x Aaliyah R&B Type Beat - \"" + cleanName (rawName) + "\"";
        return title.substring (0, 100);
    }

    juce::String makeTags()
    {
        const juce::StringArray tags
        {
            "kehlani type beat", "free kehlani type beat",
            "kehlani type beat 2026", "free kehlani type beat 2026",
            "kehlani type beat free 2026", "kehlani type beat free",
            "r&b type beat", "free r&b type beat", "r&b type beat 2026",
            "free r&b type beat 2026", "r&b type beat free 2026",
            "aaliyah type beat", "free aaliyah type beat", "aaliyah type beat 2026",
            "type beat", "free type beat", "type beat 2026", "free type beat 2026",
            "type beat free", "beat", "beats", "type beats", "free type beats"
        };
        return tags.joinIntoString (",");
    }

    juce::String makeDescription (const juce::String& rawName)
    {
        const auto money  = juce::String::fromUTF8 ("\xF0\x9F\x92\xB0");
        const auto mail   = juce::String::fromUTF8 ("\xF0\x9F\x93\xA7");
        const auto camera = juce::String::fromUTF8 ("\xF0\x9F\x93\xB7");
        const auto cash   = juce::String::fromUTF8 ("\xF0\x9F\x92\xB8");

        juce::String d;
        d << "Watch this next:  link\n\n"
          << money << " Purchase This Beat : DM me on Insta https://www.instagram.com/feddy_beatz_/\n\n"
          << "Connect with me:\n"
          << mail << "Email: beatzfeddy@gmail.com\n"
          << camera << " Instagram: https://www.instagram.com/feddy_beatz_/\n"
          << cash << " Beatstore: https://tr.ee/W2vNiP\n\n"
          << makeTitle (rawName) << "\n\n"
          << "This is a smooth, emotional Kehlani type beat inspired by her warm r&b style and the nostalgic soul of Aaliyah. "
             "Perfect for artists looking for a free Kehlani type beat, a Kehlani type beat 2026, or a free r&b type beat 2026. "
             "This beat works great as a Kehlani type beat free 2026, a free aaliyah type beat 2026, or a polished r&b type beat for writing or recording. "
             "If you need a free r&b type beat 2026, a free type beat 2026, or just free type beats, this Kehlani instrumental delivers that signature warm melodic tone. "
             "Whether you're searching for beats, type beats, or a free Kehlani type beat free, this is the beat for you.\n\n"
          << "#kehlanitypebeat #aaliyahtypebeat  #rnbtypebeat #freetypebeat #typebeat\n";
        return d;
    }

    enum class DroppedKind { audio, video, image, other };

    DroppedKind classify (const juce::File& f)
    {
        const auto ext = f.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".aiff") return DroppedKind::audio;
        if (ext == ".mp4" || ext == ".mov" || ext == ".mkv" || ext == ".avi" || ext == ".webm") return DroppedKind::video;
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") return DroppedKind::image;
        return DroppedKind::other;
    }

    double getAudioSeconds (const juce::File& f)
    {
        juce::AudioFormatManager manager;
        manager.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (manager.createReaderFor (f));
        if (reader == nullptr || reader->sampleRate <= 0.0)
            return 0.0;
        return (double) reader->lengthInSamples / reader->sampleRate;
    }

    bool isProgressLine (const juce::String& line)
    {
        static const char* keys[] = { "frame=", "fps=", "stream_", "bitrate=", "total_size=", "out_time",
                                      "dup_frames=", "drop_frames=", "speed=", "progress=" };
        for (auto* k : keys)
            if (line.startsWith (k)) return true;
        return false;
    }

       // DAWs often launch plugins without the shell PATH, so look in the usual install spots first.
    juce::String findFfmpeg()
    {
       #if JUCE_WINDOWS
        // Installed by the BeatUploader installer
        auto common = juce::SystemStats::getEnvironmentVariable ("CommonProgramW6432", {});
        if (common.isEmpty())
            common = juce::SystemStats::getEnvironmentVariable ("CommonProgramFiles", {});

        juce::File bundled (common + "\\BeatUploader\\ffmpeg.exe");
        return bundled.existsAsFile() ? bundled.getFullPathName() : juce::String ("ffmpeg");
       #else
        for (auto* p : { "/opt/homebrew/bin/ffmpeg", "/usr/local/bin/ffmpeg", "/usr/bin/ffmpeg" })
            if (juce::File (p).existsAsFile()) return p;
        return "ffmpeg";
       #endif
    }
}



//==============================================================================
// FileTile
FileTile::FileTile (Kind kindToUse, const juce::String& titleText, bool isOptional)
    : kind (kindToUse), title (titleText), optional (isOptional)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

juce::Rectangle<float> FileTile::getClearBounds() const
{
    return { (float) getWidth() - 34.0f, 8.0f, 24.0f, 24.0f };
}

void FileTile::paint (juce::Graphics& g)
{
    using namespace ToxicColours;
    const auto r = getLocalBounds().toFloat().reduced (2.0f);
    const bool has = file.existsAsFile();

    g.setColour (hover || dragOver ? raised() : card());
    g.fillRoundedRectangle (r, 12.0f);

    if (has && ! dragOver)
    {
        g.setColour (violet());
        g.drawRoundedRectangle (r, 12.0f, 1.5f);
    }
    else
    {
        // dashed outline for empty tiles / drag target
        juce::Path p;
        p.addRoundedRectangle (r, 12.0f);
        juce::Path dashedPath;
        const float dashes[] = { 6.0f, 4.0f };
        juce::PathStrokeType (1.5f).createDashedStroke (dashedPath, p, dashes, 2);
        g.setColour (dragOver ? lime() : dashed());
        g.fillPath (dashedPath);
    }

    g.setColour (violetText());
    g.setFont (ToxicFonts::ui (11.0f, true));
    g.drawText (title + (optional ? juce::String (" (OPTIONAL)") : juce::String()),
                14, 12, getWidth() - 56, 14, juce::Justification::centredLeft);

    g.setColour (has ? juce::Colours::white : muted());
    g.setFont (ToxicFonts::ui (13.0f));
    g.drawText (has ? file.getFileName() : juce::String ("Drop or browse"),
                14, getHeight() - 34, getWidth() - 28, 18, juce::Justification::centredLeft, true);

    if (optional && has)
    {
        auto c = getClearBounds().reduced (7.0f);
        g.setColour (muted());
        g.drawLine (c.getX(), c.getY(), c.getRight(), c.getBottom(), 1.5f);
        g.drawLine (c.getX(), c.getBottom(), c.getRight(), c.getY(), 1.5f);
    }

    juce::ignoreUnused (kind);
}

void FileTile::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasDraggedSinceMouseDown() || ! getLocalBounds().contains (e.getPosition()))
        return;
    if (optional && file.existsAsFile() && getClearBounds().contains (e.position))
    {
        if (onClear) onClear();
        return;
    }
    if (onClick) onClick();
}

//==============================================================================
// ChannelChip
void ChannelChip::paint (juce::Graphics& g)
{
    using namespace ToxicColours;
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (hover && isEnabled() ? border() : raised());
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (hover && isEnabled() ? lime() : violet());
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.5f);

    const bool connected = name.isNotEmpty();
    if (connected)
    {
        g.setColour (green());
        g.fillEllipse (r.getX() + 14.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);
    }

    g.setColour (isEnabled() ? juce::Colours::white : muted().withAlpha (0.5f));
    g.setFont (ToxicFonts::ui (14.0f, true));
    g.drawText (connected ? name : juce::String ("Connect channel"),
                r.toNearestInt().withTrimmedLeft (connected ? 20 : 0), juce::Justification::centred, true);
}

void ChannelChip::mouseUp (const juce::MouseEvent& e)
{
    if (isEnabled() && getLocalBounds().contains (e.getPosition()) && onClick)
        onClick();
}

//==============================================================================
// StepStrip
void StepStrip::updateTimer()
{
    if (stage >= 0 && stage <= 2 && progress < 0.0)
    {
        if (! isTimerRunning()) startTimerHz (30);
    }
    else
    {
        stopTimer();
    }
}

void StepStrip::timerCallback()
{
    phase += 0.025;
    if (phase > 1.4) phase = 0.0;
    repaint();
}

void StepStrip::setStage (int newStage, double newProgress)
{
    stage = newStage;
    progress = newProgress;
    updateTimer();
    repaint();
}

void StepStrip::setProgress (double newProgress)
{
    progress = newProgress;
    updateTimer();
    repaint();
}

void StepStrip::setMessage (const juce::String& text, bool isError)
{
    message = text;
    error = isError;
    repaint();
}

void StepStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    auto bar = r.removeFromBottom (3).toFloat();

    // Step labels on the right, current one highlighted.
    juce::StringArray names;
    juce::Array<int> ids;
    names.add ("Render");  ids.add (0);
    names.add ("Upload");  ids.add (1);
    if (thumbUsed) { names.add ("Thumbnail"); ids.add (2); }
    names.add ("Done");    ids.add (3);

    const int labelW = 70;
    auto labels = r.removeFromRight (labelW * names.size());
    g.setFont (ToxicFonts::ui (11.0f, true));
    for (int i = 0; i < names.size(); ++i)
    {
        auto cell = labels.removeFromLeft (labelW);
        const bool active = stage >= 0 && ids[i] == stage;
        const bool done   = stage >= 0 && ids[i] < stage;
        g.setColour (active ? ToxicColours::lime() : (done ? ToxicColours::violetText() : ToxicColours::dashed()));
        g.drawText (names[i], cell, juce::Justification::centred);
    }

    g.setColour (error ? ToxicColours::danger() : ToxicColours::muted());
    g.setFont (ToxicFonts::ui (13.0f));
    g.drawText (message, r.withTrimmedRight (12), juce::Justification::centredLeft, true);

    if (stage >= 0 && stage <= 2)
    {
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRoundedRectangle (bar, 1.5f);
        g.setColour (juce::Colours::white);
        if (progress >= 0.0)
        {
            g.fillRoundedRectangle (bar.withWidth ((float) juce::jlimit (0.0, 1.0, progress) * bar.getWidth()), 1.5f);
        }
        else
        {
            const float start = (float) (phase - 0.4);
            const float x0 = juce::jlimit (0.0f, 1.0f, start);
            const float x1 = juce::jlimit (0.0f, 1.0f, start + 0.4f);
            if (x1 > x0)
                g.fillRoundedRectangle (bar.getX() + x0 * bar.getWidth(), bar.getY(), (x1 - x0) * bar.getWidth(), bar.getHeight(), 1.5f);
        }
    }
    else if (stage == 3)
    {
        g.setColour (juce::Colours::white);
        g.fillRoundedRectangle (bar, 1.5f);
    }
}

//==============================================================================
BeatUploaderVSTAudioProcessorEditor::BeatUploaderVSTAudioProcessorEditor (BeatUploaderVSTAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    using namespace ToxicColours;
    setLookAndFeel (&toxicTheme);
    setResizeLimits (1000, 640, 1600, 1000);
    setSize (1120, 700);

    uploadButton.setButtonText ("Upload Beat");
    resetTitleButton.setButtonText ("Reset");
    resetDescButton.setButtonText ("Reset");
    uploadButton.getProperties().set ("primary", true);
    resetTitleButton.getProperties().set ("link", true);
    resetDescButton.getProperties().set ("link", true);

    for (auto* b : { &uploadButton, &resetTitleButton, &resetDescButton }) addAndMakeVisible (*b);
    for (auto* t : { &beatTile, &coverTile, &videoTile, &thumbTile }) addAndMakeVisible (*t);

    addAndMakeVisible (channelChip);
    addAndMakeVisible (stepStrip);

    nameEditor.setTextToShowWhenEmpty ("e.g. Midnight Drive", muted().withAlpha (0.7f));
    nameEditor.onTextChange = [this] { applyTemplates(); };
    titleEditor.setInputRestrictions (100);
    titleEditor.onTextChange = [this] { titleEdited = true; updateTitleCount(); };
    descriptionEditor.setMultiLine (true, true);
    descriptionEditor.setReturnKeyStartsNewLine (true);
    descriptionEditor.setInputRestrictions (5000);
    descriptionEditor.onTextChange = [this] { descEdited = true; };

    for (auto* c : { &nameEditor, &titleEditor, &descriptionEditor }) addAndMakeVisible (*c);
    for (auto* l : { &nameLabel, &titleLabel, &descLabel, &titleCountLabel }) addAndMakeVisible (*l);
    nameLabel.setText ("Beat name", juce::dontSendNotification);
    titleLabel.setText ("YouTube title", juce::dontSendNotification);
    descLabel.setText ("Description", juce::dontSendNotification);
    titleCountLabel.setJustificationType (juce::Justification::centredRight);

    beatTile.onClick  = [this] { if (! busy) chooseBeat(); };
    coverTile.onClick = [this] { if (! busy) chooseCover(); };
    videoTile.onClick = [this] { if (! busy) chooseVideo(); };
    thumbTile.onClick = [this] { if (! busy) chooseThumbnail(); };

    videoTile.onClear = [this]
    {
        if (busy) return;
        videoFile = juce::File();
        videoTile.setFile (videoFile);
        repaint();
        updateReadyStatus();
    };
    thumbTile.onClear = [this]
    {
        if (busy) return;
        thumbFile = juce::File();
        thumbTile.setFile (thumbFile);
        updateReadyStatus();
    };

    channelChip.onClick  = [this] { connectChannel(); };
    uploadButton.onClick = [this] { startUpload(); };
    resetTitleButton.onClick = [this] { titleEdited = false; applyTemplates(); };
    resetDescButton.onClick  = [this] { descEdited  = false; applyTemplates(); };

    applyTemplates();
    refreshChannelLabel();
    updateReadyStatus();
}

BeatUploaderVSTAudioProcessorEditor::~BeatUploaderVSTAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void BeatUploaderVSTAudioProcessorEditor::parentHierarchyChanged()
{
}

void BeatUploaderVSTAudioProcessorEditor::applyTemplates()
{
    auto name = nameEditor.getText();
    if (! titleEdited) titleEditor.setText (makeTitle (name), false);
    if (! descEdited) descriptionEditor.setText (makeDescription (name), false);
    updateTitleCount();
}

void BeatUploaderVSTAudioProcessorEditor::updateTitleCount()
{
    titleCountLabel.setText (juce::String (titleEditor.getText().length()) + " / 100", juce::dontSendNotification);
}

void BeatUploaderVSTAudioProcessorEditor::refreshChannelLabel()
{
    channelChip.setChannel (uploader->isConnected() ? uploader->getChannelName() : juce::String());
}

void BeatUploaderVSTAudioProcessorEditor::updateReadyStatus()
{
    if (busy) return;
    stepStrip.setStage (-1);
    stepStrip.setThumbnailStepUsed (thumbFile.existsAsFile());
    if (! beatFile.existsAsFile()) stepStrip.setMessage ("Drop your exported beat here, or click a tile to browse.");
    else if (! videoFile.existsAsFile() && ! coverImage.existsAsFile()) stepStrip.setMessage ("Add a cover image or a background video.");
    else if (! uploader->isConnected()) stepStrip.setMessage ("Ready. No channel connected, so this will make the video for a manual upload.");
    else stepStrip.setMessage ("Ready. Press Upload Beat.");
}

void BeatUploaderVSTAudioProcessorEditor::setBusy (bool shouldBeBusy)
{
    busy = shouldBeBusy;
    uploadButton.setEnabled (! busy);
    channelChip.setEnabled (! busy);
}

void BeatUploaderVSTAudioProcessorEditor::pickFile (const juce::String& prompt, const juce::File& startDir,
                                                    const juce::String& wildcards,
                                                    std::function<void (const juce::File&)> onPicked)
{
    chooser = std::make_unique<juce::FileChooser> (prompt, startDir, wildcards);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [onPicked] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f.existsAsFile()) onPicked (f);
    });
}

void BeatUploaderVSTAudioProcessorEditor::chooseBeat()
{
    auto startDir = beatFile.existsAsFile() ? beatFile.getParentDirectory()
                                            : juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    pickFile ("Select your exported beat", startDir, "*.wav;*.mp3;*.flac",
              [this] (const juce::File& f) { setBeat (f); });
}

void BeatUploaderVSTAudioProcessorEditor::chooseCover()
{
    auto startDir = coverImage.existsAsFile() ? coverImage.getParentDirectory()
                                              : juce::File::getSpecialLocation (juce::File::userPicturesDirectory);
    pickFile ("Select your cover image", startDir, "*.png;*.jpg;*.jpeg",
              [this] (const juce::File& f) { setCover (f); });
}

void BeatUploaderVSTAudioProcessorEditor::chooseVideo()
{
    auto startDir = videoFile.existsAsFile() ? videoFile.getParentDirectory()
                                             : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    pickFile ("Select a background video", startDir, "*.mp4;*.mov;*.mkv;*.avi;*.webm",
              [this] (const juce::File& f) { setVideo (f); });
}

void BeatUploaderVSTAudioProcessorEditor::chooseThumbnail()
{
    auto startDir = thumbFile.existsAsFile() ? thumbFile.getParentDirectory()
                                             : juce::File::getSpecialLocation (juce::File::userPicturesDirectory);
    pickFile ("Select a thumbnail", startDir, "*.png;*.jpg;*.jpeg",
              [this] (const juce::File& f) { setThumbnail (f); });
}

void BeatUploaderVSTAudioProcessorEditor::setBeat (const juce::File& f)
{
    beatFile = f;
    beatTile.setFile (f);
    nameEditor.setText (f.getFileNameWithoutExtension(), true);
    updateReadyStatus();
}

void BeatUploaderVSTAudioProcessorEditor::setCover (const juce::File& f)
{
    coverImage = f;
    coverThumb = juce::ImageFileFormat::loadFrom (f);
    coverTile.setFile (f);
    repaint();
    updateReadyStatus();
}

void BeatUploaderVSTAudioProcessorEditor::setVideo (const juce::File& f)
{
    videoFile = f;
    videoTile.setFile (f);
    repaint();
    updateReadyStatus();
}

void BeatUploaderVSTAudioProcessorEditor::setThumbnail (const juce::File& f)
{
    if (f.getSize() > 2 * 1024 * 1024)
    {
        stepStrip.setMessage ("That thumbnail is over 2 MB, YouTube won't accept it.", true);
        return;
    }
    thumbFile = f;
    thumbTile.setFile (f);
    updateReadyStatus();
}

bool BeatUploaderVSTAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    if (busy) return false;
    for (auto& path : files)
        if (classify (juce::File (path)) != DroppedKind::other) return true;
    return false;
}

void BeatUploaderVSTAudioProcessorEditor::filesDropped (const juce::StringArray& files, int x, int y)
{
    dragging = false;
    for (auto& path : files)
    {
        juce::File f (path);
        switch (classify (f))
        {
            case DroppedKind::audio: setBeat (f); break;
            case DroppedKind::video: setVideo (f); break;
            case DroppedKind::image:
                if (thumbTile.getBounds().contains (x, y)) setThumbnail (f);
                else setCover (f);
                break;
            case DroppedKind::other: break;
        }
    }
    repaint();
}

void BeatUploaderVSTAudioProcessorEditor::connectChannel()
{
    if (busy) return;
    if (! uploader->hasClientConfig())
    {
        stepStrip.setMessage ("Missing youtube_client.json in " + uploader->getConfigFolder().getFullPathName(), true);
        return;
    }
    setBusy (true);
    stepStrip.setMessage ("Sign in with Google in your browser...");

    auto api = uploader;
    juce::Component::SafePointer<BeatUploaderVSTAudioProcessorEditor> safeThis (this);

    juce::Thread::launch ([safeThis, api]
    {
        auto result = api->connect();
        juce::MessageManager::callAsync ([safeThis, result]
        {
            if (safeThis == nullptr) return;
            safeThis->setBusy (false);
            safeThis->refreshChannelLabel();
            safeThis->updateReadyStatus();
            if (result.ok) safeThis->stepStrip.setMessage ("Connected to " + result.message + ". Ready.");
            else safeThis->stepStrip.setMessage ("Sign-in failed: " + result.message, true);
        });
    });
}

void BeatUploaderVSTAudioProcessorEditor::startUpload()
{
    if (busy) return;
    const bool useVideo = videoFile.existsAsFile();
    if (! beatFile.existsAsFile()) { stepStrip.setMessage ("Add your beat first.", true); return; }
    if (! useVideo && ! coverImage.existsAsFile()) { stepStrip.setMessage ("Add a cover image or a background video first.", true); return; }
    auto title = titleEditor.getText().trim();
    if (title.isEmpty()) { stepStrip.setMessage ("The title is empty.", true); return; }

    const auto description = descriptionEditor.getText();
    const bool useApi = uploader->isConnected();
    const double seconds = getAudioSeconds (beatFile);
    auto api = uploader;

    juce::StringArray tags;
    tags.addTokens (makeTags(), ",", "");

    auto beat = beatFile;
    auto image = coverImage;
    auto video = videoFile;
    auto thumb = thumbFile;
    auto mp4 = beatFile.withFileExtension ("mp4");
    if (mp4 == video) mp4 = beatFile.getSiblingFile (beatFile.getFileNameWithoutExtension() + " - youtube.mp4");

    stepStrip.setThumbnailStepUsed (thumb.existsAsFile());
    setBusy (true);
    stepStrip.setStage (0, seconds > 0.0 ? 0.0 : -1.0);
    stepStrip.setMessage ("Rendering video...");

    juce::Component::SafePointer<BeatUploaderVSTAudioProcessorEditor> safeThis (this);

    juce::Thread::launch ([safeThis, beat, image, video, thumb, mp4, useVideo, useApi, api, title, description, tags, seconds]
    {
        juce::StringArray args;
        args.add (findFfmpeg()); args.add ("-y"); args.add ("-loglevel"); args.add ("error");
        args.add ("-nostats"); args.add ("-progress"); args.add ("pipe:1");
        if (useVideo) args.addArray ({ "-stream_loop", "-1", "-i", video.getFullPathName() });
        else args.addArray ({ "-loop", "1", "-framerate", "2", "-i", image.getFullPathName() });
        args.addArray ({ "-i", beat.getFullPathName(), "-map", "0:v:0", "-map", "1:a:0" });

        juce::String filter = useVideo
            ? (videoFillsFrame ? "scale=1920:1080:force_original_aspect_ratio=increase,crop=1920:1080,setsar=1"
                               : "scale=1080:1080:force_original_aspect_ratio=increase,crop=1080:1080,pad=1920:1080:420:0:black,setsar=1")
            : "scale=1080:1080:force_original_aspect_ratio=increase,crop=1080:1080,pad=1920:1080:420:0:black";
        args.addArray ({ "-vf", filter, "-r", "30", "-c:v", "libx264", "-pix_fmt", "yuv420p" });
        if (useVideo) args.addArray ({ "-preset", "veryfast", "-crf", "20" });
        else args.addArray ({ "-tune", "stillimage" });
        args.addArray ({ "-c:a", "aac", "-b:a", "320k", "-shortest", "-movflags", "+faststart", mp4.getFullPathName() });

        juce::ChildProcess proc;
        juce::String log;
        int exitCode = -1;
        const bool started = proc.start (args);
        if (started)
        {
            char buffer[4096];
            juce::String pending;
            double lastReported = -1.0;

            for (;;)
            {
                const int n = proc.readProcessOutput (buffer, (int) sizeof (buffer));
                if (n <= 0) break;
                pending += juce::String::fromUTF8 (buffer, n);

                int nl;
                while ((nl = pending.indexOfChar ('\n')) >= 0)
                {
                    auto line = pending.substring (0, nl).trim();
                    pending = pending.substring (nl + 1);

                    if (line.startsWith ("out_time_us=") && seconds > 0.0)
                    {
                        const auto us = (double) line.fromFirstOccurrenceOf ("=", false, false).getLargeIntValue();
                        const auto frac = juce::jlimit (0.0, 1.0, us / 1.0e6 / seconds);
                        if (frac - lastReported >= 0.01)
                        {
                            lastReported = frac;
                            juce::MessageManager::callAsync ([safeThis, frac]
                            {
                                if (safeThis != nullptr) safeThis->stepStrip.setProgress (frac);
                            });
                        }
                    }
                    else if (line.isNotEmpty() && ! isProgressLine (line))
                    {
                        log << line << "\n";
                    }
                }
            }

            proc.waitForProcessToFinish (10000);
            exitCode = (int) proc.getExitCode();
        }
        else
        {
            log = "Could not start ffmpeg. Install it and make sure it is on your PATH.";
        }

        const bool renderOk = started && exitCode == 0 && mp4.existsAsFile();
        if (! renderOk || ! useApi)
        {
            juce::MessageManager::callAsync ([safeThis, mp4, renderOk, log]
            {
                if (safeThis != nullptr) safeThis->onRenderFinished (renderOk, mp4, log);
            });
            return;
        }

        juce::MessageManager::callAsync ([safeThis]
        {
            if (safeThis == nullptr) return;
            safeThis->stepStrip.setStage (1, -1.0);
            safeThis->stepStrip.setMessage ("Uploading to YouTube...");
        });

        // Lets the uploader move the step strip on to the thumbnail stage.
        auto onStep = [safeThis] (int step)
        {
            juce::MessageManager::callAsync ([safeThis, step]
            {
                if (safeThis == nullptr) return;
                safeThis->stepStrip.setStage (step, -1.0);
                safeThis->stepStrip.setMessage (step == 2 ? "Setting thumbnail..." : "Uploading to YouTube...");
            });
        };

        const auto result = api->upload (mp4, thumb, title, description, tags, uploadPrivacy, onStep);
        juce::MessageManager::callAsync ([safeThis, result]
        {
            if (safeThis != nullptr) safeThis->onUploadFinished (result.ok, result.message, result.warning);
        });
    });
}

void BeatUploaderVSTAudioProcessorEditor::onRenderFinished (bool ok, const juce::File& video, const juce::String& log)
{
    setBusy (false);
    if (! ok) { stepStrip.setStage (-1); stepStrip.setMessage ("ffmpeg failed: " + log.trim().substring (0, 160), true); return; }
    juce::SystemClipboard::copyTextToClipboard (titleEditor.getText());
    auto infoFile = video.getSiblingFile (video.getFileNameWithoutExtension() + " - youtube.txt");
    infoFile.replaceWithText ("TITLE\n" + titleEditor.getText() + "\n\nDESCRIPTION\n" + descriptionEditor.getText() + "\n\nTAGS\n" + makeTags() + "\n");
    video.revealToUser();
    juce::URL ("https://www.youtube.com/upload").launchInDefaultBrowser();
    stepStrip.setStage (3, 1.0);
    stepStrip.setMessage ("Done! Title copied, description saved next to the video.");
}

void BeatUploaderVSTAudioProcessorEditor::onUploadFinished (bool ok, const juce::String& message, const juce::String& warning)
{
    setBusy (false);
    if (! ok) { stepStrip.setStage (-1); stepStrip.setMessage ("Upload failed: " + message.substring (0, 200), true); return; }
    juce::SystemClipboard::copyTextToClipboard ("https://youtu.be/" + message);
    juce::URL ("https://studio.youtube.com/video/" + message + "/edit").launchInDefaultBrowser();
    auto status = "Uploaded as " + uploadPrivacy + ". Link copied.";
    if (warning.isNotEmpty()) status += " Warning: " + warning;
    stepStrip.setStage (3, 1.0);
    stepStrip.setMessage (status, warning.isNotEmpty());
}

void BeatUploaderVSTAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace ToxicColours;
    g.fillAll (background());

    g.setColour (juce::Colours::white);
    g.setFont (ToxicFonts::ui (24.0f, true));
    g.drawText ("Beat Uploader", 30, 30, 400, 40, juce::Justification::centredLeft);

    // Cards
    for (auto cardBounds : { previewCard, formCard })
    {
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRoundedRectangle (cardBounds.toFloat(), 16.0f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawRoundedRectangle (cardBounds.toFloat().reduced (0.5f), 16.0f, 1.0f);
    }

    // Preview frame: shows roughly what the rendered video will look like.
    g.setColour (juce::Colours::black);
    g.fillRect (frameBounds);

    if (videoFile.existsAsFile())
    {
        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::FontOptions (13.0f));
        g.drawFittedText ("Background video\n" + videoFile.getFileName(), frameBounds.reduced (16),
                          juce::Justification::centred, 3);
    }
    else if (coverThumb.isValid())
    {
        const auto side = frameBounds.getHeight();
        const auto square = frameBounds.withSizeKeepingCentre (side, side);
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (square);
        g.drawImageWithin (coverThumb, square.getX(), square.getY(), square.getWidth(), square.getHeight(),
                           juce::RectanglePlacement::fillDestination);
    }
    else
    {
        g.setColour (juce::Colours::grey);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText ("Add a cover image or video", frameBounds, juce::Justification::centred);
    }

    g.setColour (juce::Colours::white.withAlpha (0.2f));
    g.drawRect (frameBounds, 1);

    // Visibility badge
    {
        const auto b = visibilityBadge.toFloat();
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (uploadPrivacy.toUpperCase(), visibilityBadge, juce::Justification::centred);
    }

    // Footer divider
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillRect (30, footerTop - 8, getWidth() - 60, 1);

    if (dragging)
    {
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (6.0f), 14.0f, 2.0f);
    }
}

void BeatUploaderVSTAudioProcessorEditor::resized()
{
    const int pad = 16;
    const int chipW = 220;
    const int chipH = 38;

    auto area = getLocalBounds().reduced (30);

    auto header = area.removeFromTop (60);
    channelChip.setBounds (header.removeFromRight (chipW).withSizeKeepingCentre (chipW, chipH));

    // Footer: status strip + upload button
    auto footer = area.removeFromBottom (28 + 8 + 42);
    footerTop = footer.getY();
    stepStrip.setBounds (footer.removeFromTop (28));
    footer.removeFromTop (8);
    uploadButton.setBounds (footer.withSizeKeepingCentre (200, 42));
    area.removeFromBottom (pad);

    // Left column: preview
    const int leftW = juce::jmax (380, (int) (area.getWidth() * 0.42f));
    previewCard = area.removeFromLeft (leftW);
    area.removeFromLeft (24);

    auto inner = previewCard.reduced (14);
    frameBounds = inner.removeFromTop (inner.getWidth() * 9 / 16);
    inner.removeFromTop (12);
    visibilityBadge = inner.removeFromTop (26).withWidth (96);

    // Right column: tiles on top, form below
    auto tiles = area.removeFromTop (84);
    const int tileW = (tiles.getWidth() - pad * 3) / 4;
    beatTile.setBounds  (tiles.removeFromLeft (tileW)); tiles.removeFromLeft (pad);
    coverTile.setBounds (tiles.removeFromLeft (tileW)); tiles.removeFromLeft (pad);
    videoTile.setBounds (tiles.removeFromLeft (tileW)); tiles.removeFromLeft (pad);
    thumbTile.setBounds (tiles);

    area.removeFromTop (pad);
    formCard = area;
    auto form = formCard.reduced (18);

    nameLabel.setBounds (form.removeFromTop (20));
    nameEditor.setBounds (form.removeFromTop (32));
    form.removeFromTop (12);

    auto titleRow = form.removeFromTop (20);
    resetTitleButton.setBounds (titleRow.removeFromRight (56));
    titleCountLabel.setBounds (titleRow.removeFromRight (90));
    titleLabel.setBounds (titleRow);
    titleEditor.setBounds (form.removeFromTop (32));
    form.removeFromTop (12);

    auto descRow = form.removeFromTop (20);
    resetDescButton.setBounds (descRow.removeFromRight (56));
    descLabel.setBounds (descRow);
    descriptionEditor.setBounds (form);
}