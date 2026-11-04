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
    return { (float) getWidth() - 36.0f, 10.0f, 24.0f, 24.0f };
}

void FileTile::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (2.0f);
    const bool has = file.existsAsFile();

    g.setColour (has ? juce::Colours::lightgrey : juce::Colours::grey);
    g.drawRoundedRectangle (r, 12.0f, 1.5f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (has ? file.getFileName() : juce::String ("Drop or browse"),
                14, getHeight() - 32, getWidth() - 28, 18, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (title, 14, getHeight() - 48, getWidth() - 28, 14, juce::Justification::centredLeft);
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
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colours::darkgrey);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (name.isNotEmpty() ? name : juce::String ("Connect channel"), r.toNearestInt(), juce::Justification::centred);
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
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (message, getLocalBounds(), juce::Justification::centredLeft, true);
}

//==============================================================================
BeatUploaderVSTAudioProcessorEditor::BeatUploaderVSTAudioProcessorEditor (BeatUploaderVSTAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&toxicTheme);
    setSize (920, 600);

    uploadButton.setButtonText ("Upload Beat");
    resetTitleButton.setButtonText ("Reset");
    resetDescButton.setButtonText ("Reset");

    for (auto* b : { &uploadButton, &resetTitleButton, &resetDescButton })
        addAndMakeVisible (*b);

    for (auto* t : { &beatTile, &coverTile, &videoTile, &thumbTile })
        addAndMakeVisible (*t);

    addAndMakeVisible (channelChip);
    addAndMakeVisible (stepStrip);

    nameEditor.onTextChange = [this] { applyTemplates(); };
    titleEditor.onTextChange = [this] { titleEdited = true; };
    descriptionEditor.setMultiLine (true, true);
    descriptionEditor.setReturnKeyStartsNewLine (true);

    for (auto* c : { &nameEditor, &titleEditor, &descriptionEditor })
        addAndMakeVisible (*c);

    beatTile.onClick  = [this] { if (! busy) chooseBeat(); };
    coverTile.onClick = [this] { if (! busy) chooseCover(); };
    videoTile.onClick = [this] { if (! busy) chooseVideo(); };
    thumbTile.onClick = [this] { if (! busy) chooseThumbnail(); };

    channelChip.onClick  = [this] { connectChannel(); };
    uploadButton.onClick = [this] { startUpload(); };

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
    if (auto* peer = getPeer())
        peer->setCurrentRenderingEngine (0);
}

void BeatUploaderVSTAudioProcessorEditor::applyTemplates()
{
    auto name = nameEditor.getText();
    if (! titleEdited) titleEditor.setText (makeTitle (name), false);
    if (! descEdited) descriptionEditor.setText (makeDescription (name), false);
}

void BeatUploaderVSTAudioProcessorEditor::refreshChannelLabel()
{
    channelChip.setChannel (uploader->isConnected() ? uploader->getChannelName() : juce::String());
}

void BeatUploaderVSTAudioProcessorEditor::updateReadyStatus()
{
    if (busy) return;
    if (! beatFile.existsAsFile())
        stepStrip.setMessage ("Select your exported beat.");
    else if (! videoFile.existsAsFile() && ! coverImage.existsAsFile())
        stepStrip.setMessage ("Select a cover image or a background video.");
    else
        stepStrip.setMessage ("Ready. Press Upload Beat.");
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

    setBusy (true);
    stepStrip.setStage (0, seconds > 0.0 ? 0.0 : -1.0);
    stepStrip.setMessage ("Rendering video...");

    juce::Component::SafePointer<BeatUploaderVSTAudioProcessorEditor> safeThis (this);

    juce::Thread::launch ([safeThis, beat, image, video, thumb, mp4, useVideo, useApi, api, title, description, tags, seconds]
    {
        juce::StringArray args;
        args.add ("ffmpeg"); args.add ("-y"); args.add ("-loglevel"); args.add ("error");
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
        juce::String output;
        int exitCode = -1;
        const bool started = proc.start (args);
        if (started)
        {
            output = proc.readAllProcessOutput();
            proc.waitForProcessToFinish (60000);
            exitCode = (int) proc.getExitCode();
        }
        else
        {
            output = "Could not start ffmpeg. Add it to your PATH.";
        }

        const bool renderOk = started && exitCode == 0;
        if (! renderOk || ! useApi)
        {
            juce::MessageManager::callAsync ([safeThis, mp4, renderOk, output]
            {
                if (safeThis != nullptr) safeThis->onRenderFinished (renderOk, mp4, output);
            });
            return;
        }

        juce::MessageManager::callAsync ([safeThis]
        {
            if (safeThis != nullptr) safeThis->stepStrip.setMessage ("Uploading to YouTube...");
        });

        const auto result = api->upload (mp4, thumb, title, description, tags, uploadPrivacy);
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
    g.fillAll (juce::Colours::black);
}

void BeatUploaderVSTAudioProcessorEditor::resized()
{
    // minimal layout
}
