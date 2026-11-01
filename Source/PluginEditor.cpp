#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // YouTube visibility for API uploads. Switch to "unlisted" after Google has audited the project.
    const juce::String uploadPrivacy { "private" };

    // true: background video fills the frame. false: centered 1080 x 1080 square.
    const bool videoFillsFrame = true;

    const char* const emptyBeatText  = "No beat selected";
    const char* const emptyCoverText = "No cover selected";
    const char* const emptyVideoText = "Optional: loops under your beat. Its own audio is muted.";
    const char* const emptyThumbText = "Optional: YouTube thumbnail (JPG/PNG, max 2 MB)";

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

    juce::StringArray makeTags()
    {
        return { "kehlani type beat", "free kehlani type beat",
                 "kehlani type beat 2026", "free kehlani type beat 2026",
                 "kehlani type beat free 2026", "kehlani type beat free",
                 "r&b type beat", "free r&b type beat", "r&b type beat 2026",
                 "free r&b type beat 2026", "r&b type beat free 2026",
                 "aaliyah type beat", "free aaliyah type beat", "aaliyah type beat 2026",
                 "type beat", "free type beat", "type beat 2026", "free type beat 2026",
                 "type beat free", "beat", "beats", "type beats", "free type beats" };
    }

    juce::String makeDescription (const juce::String& rawName)
    {
        const auto money  = juce::String::fromUTF8 ("\xF0\x9F\x92\xB0");
        const auto mail   = juce::String::fromUTF8 ("\xF0\x9F\x93\xA7");
        const auto camera = juce::String::fromUTF8 ("\xF0\x9F\x93\xB7");
        const auto cash   = juce::String::fromUTF8 ("\xF0\x9F\x92\xB8");

        juce::String description;
        description << "Watch this next:  link\n\n"
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
        return description;
    }

    void showPicked (juce::Label& label, const juce::File& file)
    {
        label.setText (file.getFileName(), juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, ToxicColours::lime());
    }

    void showEmpty (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, ToxicColours::muted());
    }

    void drawCard (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title)
    {
        using namespace ToxicColours;
        auto bounds = area.toFloat();
        g.setColour (card());
        g.fillRoundedRectangle (bounds, 10.0f);
        g.setColour (violet().withAlpha (0.35f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);
        g.setColour (violet());
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (title, area.getX() + 20, area.getY() + 10, area.getWidth() - 40, 18,
                    juce::Justification::centredLeft);
    }
}

BeatUploaderVSTAudioProcessorEditor::BeatUploaderVSTAudioProcessorEditor (BeatUploaderVSTAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), uploader (std::make_shared<YouTubeUploader>())
{
    using namespace ToxicColours;

    setLookAndFeel (&toxicTheme);
    setSize (920, 600);

    auto setupLabel = [this] (juce::Label& label, const juce::String& text, float size,
                              juce::Colour colour, bool bold)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        label.setColour (juce::Label::textColourId, colour);
        label.setMinimumHorizontalScale (1.0f);
        addAndMakeVisible (label);
    };

    selectBeatButton.setButtonText ("Select Beat (WAV)");
    selectCoverButton.setButtonText ("Cover Image");
    selectVideoButton.setButtonText ("Background Video");
    selectThumbButton.setButtonText ("Thumbnail");
    clearVideoButton.setButtonText ("Clear");
    clearThumbButton.setButtonText ("Clear");
    resetTitleButton.setButtonText ("Reset");
    resetDescButton.setButtonText ("Reset");
    uploadButton.setButtonText ("Upload Beat");
    connectButton.setButtonText ("Connect Channel");
    uploadButton.getProperties().set ("primary", true);

    for (auto* button : { &selectBeatButton, &selectCoverButton, &selectVideoButton, &selectThumbButton,
                          &clearVideoButton, &clearThumbButton, &resetTitleButton, &resetDescButton,
                          &uploadButton, &connectButton })
        addAndMakeVisible (*button);

    setupLabel (beatFileLabel, emptyBeatText, 13.0f, muted(), false);
    setupLabel (coverFileLabel, emptyCoverText, 13.0f, muted(), false);
    setupLabel (videoFileLabel, emptyVideoText, 13.0f, muted(), false);
    setupLabel (thumbFileLabel, emptyThumbText, 13.0f, muted(), false);
    setupLabel (nameLabel, "BEAT NAME", 11.0f, muted(), true);
    setupLabel (titleLabel, "YOUTUBE TITLE", 11.0f, muted(), true);
    setupLabel (descLabel, "DESCRIPTION", 11.0f, muted(), true);
    setupLabel (statusLabel, "Select your exported beat.", 14.0f, lime(), false);
    setupLabel (channelLabel, {}, 13.0f, muted(), false);
    channelLabel.setJustificationType (juce::Justification::centredRight);

    nameEditor.setFont (juce::FontOptions (16.0f));
    nameEditor.setTextToShowWhenEmpty ("e.g. Midnight Drive", muted());
    nameEditor.onTextChange = [this] { applyTemplates(); };
    addAndMakeVisible (nameEditor);

    titleEditor.setFont (juce::FontOptions (16.0f));
    titleEditor.setInputRestrictions (100);
    titleEditor.onTextChange = [this] { titleEdited = true; };
    addAndMakeVisible (titleEditor);

    descriptionEditor.setMultiLine (true, true);
    descriptionEditor.setReturnKeyStartsNewLine (true);
    descriptionEditor.setFont (juce::FontOptions (14.0f));
    descriptionEditor.setInputRestrictions (5000);
    descriptionEditor.onTextChange = [this] { descEdited = true; };
    addAndMakeVisible (descriptionEditor);

    progressBar.setPercentageDisplay (false);
    addChildComponent (progressBar);

    selectBeatButton.onClick  = [this] { chooseBeat(); };
    selectCoverButton.onClick = [this] { chooseCover(); };
    selectVideoButton.onClick = [this] { chooseVideo(); };
    selectThumbButton.onClick = [this] { chooseThumbnail(); };
    uploadButton.onClick      = [this] { startUpload(); };
    connectButton.onClick     = [this] { connectChannel(); };

    clearVideoButton.onClick = [this]
    {
        videoFile = {};
        showEmpty (videoFileLabel, emptyVideoText);
        repaint();
        updateReadyStatus();
    };

    clearThumbButton.onClick = [this]
    {
        thumbFile = {};
        showEmpty (thumbFileLabel, emptyThumbText);
    };

    resetTitleButton.onClick = [this]
    {
        titleEdited = false;
        titleEditor.setText (makeTitle (nameEditor.getText()), false);
    };

    resetDescButton.onClick = [this]
    {
        descEdited = false;
        descriptionEditor.setText (makeDescription (nameEditor.getText()), false);
    };

    applyTemplates();
    refreshChannelLabel();
}

BeatUploaderVSTAudioProcessorEditor::~BeatUploaderVSTAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void BeatUploaderVSTAudioProcessorEditor::applyTemplates()
{
    const auto name = nameEditor.getText();
    if (! titleEdited)
        titleEditor.setText (makeTitle (name), false);
    if (! descEdited)
        descriptionEditor.setText (makeDescription (name), false);
}

void BeatUploaderVSTAudioProcessorEditor::refreshChannelLabel()
{
    const bool connected = uploader->isConnected();
    channelLabel.setText (connected ? "Channel: " + uploader->getChannelName()
                                    : "No channel connected (manual upload is available)",
                          juce::dontSendNotification);
    connectButton.setButtonText (connected ? "Switch Channel" : "Connect Channel");
}

void BeatUploaderVSTAudioProcessorEditor::updateReadyStatus()
{
    if (! beatFile.existsAsFile())
        setStatus ("Select your exported beat.");
    else if (! videoFile.existsAsFile() && ! coverImage.existsAsFile())
        setStatus ("Select a cover image or a background video.");
    else
        setStatus ("Ready. Press Upload Beat.");
}

void BeatUploaderVSTAudioProcessorEditor::setStatus (const juce::String& text, bool isError)
{
    statusLabel.setColour (juce::Label::textColourId,
                           isError ? ToxicColours::danger() : ToxicColours::lime());
    statusLabel.setText (text, juce::dontSendNotification);
}

void BeatUploaderVSTAudioProcessorEditor::setBusy (bool shouldBeBusy)
{
    busy = shouldBeBusy;
    for (auto* button : { &uploadButton, &connectButton, &selectBeatButton, &selectCoverButton,
                          &selectVideoButton, &selectThumbButton, &clearVideoButton, &clearThumbButton })
        button->setEnabled (! busy);
    progressBar.setVisible (busy);
    if (busy)
        progressValue = -1.0;
}

void BeatUploaderVSTAudioProcessorEditor::pickFile (const juce::String& prompt, const juce::File& startDir,
                                                     const juce::String& wildcards,
                                                     std::function<void (const juce::File&)> onPicked)
{
    chooser = std::make_unique<juce::FileChooser> (prompt, startDir, wildcards);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [onPicked = std::move (onPicked)] (const juce::FileChooser& fileChooser)
    {
        const auto file = fileChooser.getResult();
        if (file.existsAsFile())
            onPicked (file);
    });
}

void BeatUploaderVSTAudioProcessorEditor::chooseBeat()
{
    const auto startDir = beatFile.existsAsFile() ? beatFile.getParentDirectory()
                                                   : juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    pickFile ("Select your exported beat", startDir, "*.wav;*.mp3;*.flac", [this] (const juce::File& file)
    {
        beatFile = file;
        showPicked (beatFileLabel, file);
        nameEditor.setText (file.getFileNameWithoutExtension(), true);
        updateReadyStatus();
    });
}

void BeatUploaderVSTAudioProcessorEditor::chooseCover()
{
    const auto startDir = coverImage.existsAsFile() ? coverImage.getParentDirectory()
                                                     : juce::File::getSpecialLocation (juce::File::userPicturesDirectory);
    pickFile ("Select your cover image", startDir, "*.png;*.jpg;*.jpeg", [this] (const juce::File& file)
    {
        coverImage = file;
        coverThumb = juce::ImageFileFormat::loadFrom (file);
        showPicked (coverFileLabel, file);
        repaint();
        updateReadyStatus();
    });
}

void BeatUploaderVSTAudioProcessorEditor::chooseVideo()
{
    const auto startDir = videoFile.existsAsFile() ? videoFile.getParentDirectory()
                                                    : juce::File::getSpecialLocation (juce::File::userVideosDirectory);
    pickFile ("Select a background video", startDir, "*.mp4;*.mov;*.mkv;*.avi;*.webm", [this] (const juce::File& file)
    {
        videoFile = file;
        showPicked (videoFileLabel, file);
        repaint();
        updateReadyStatus();
    });
}

void BeatUploaderVSTAudioProcessorEditor::chooseThumbnail()
{
    const auto startDir = thumbFile.existsAsFile() ? thumbFile.getParentDirectory()
                                                    : juce::File::getSpecialLocation (juce::File::userPicturesDirectory);
    pickFile ("Select a YouTube thumbnail", startDir, "*.png;*.jpg;*.jpeg", [this] (const juce::File& file)
    {
        if (file.getSize() > 2 * 1024 * 1024)
        {
            setStatus ("That thumbnail is over 2 MB, YouTube won't accept it.", true);
            return;
        }
        thumbFile = file;
        showPicked (thumbFileLabel, file);
    });
}

void BeatUploaderVSTAudioProcessorEditor::connectChannel()
{
    if (busy)
        return;

    if (! uploader->hasClientConfig())
    {
        setStatus ("Missing youtube_client.json in " + uploader->getConfigFolder().getFullPathName(), true);
        return;
    }

    setBusy (true);
    setStatus ("Sign in with Google in your browser...");
    auto api = uploader;
    juce::Component::SafePointer<BeatUploaderVSTAudioProcessorEditor> safeThis (this);

    juce::Thread::launch ([safeThis, api]
    {
        auto result = api->connect();
        juce::MessageManager::callAsync ([safeThis, result]
        {
            if (safeThis == nullptr)
                return;
            safeThis->setBusy (false);
            safeThis->refreshChannelLabel();
            if (result.ok)
                safeThis->setStatus ("Connected to " + result.message + ".");
            else
                safeThis->setStatus ("Sign-in failed: " + result.message, true);
        });
    });
}

void BeatUploaderVSTAudioProcessorEditor::startUpload()
{
    if (busy)
        return;

    const bool useVideo = videoFile.existsAsFile();
    if (! beatFile.existsAsFile())
    {
        setStatus ("Select your beat first.", true);
        return;
    }
    if (! useVideo && ! coverImage.existsAsFile())
    {
        setStatus ("Select a cover image or a background video first.", true);
        return;
    }

    const auto title = titleEditor.getText().trim();
    if (title.isEmpty())
    {
        setStatus ("The title is empty.", true);
        return;
    }

    const auto description = descriptionEditor.getText();
    const bool useApi = uploader->isConnected();
    auto api = uploader;
    auto tags = makeTags();
    auto beat = beatFile;
    auto image = coverImage;
    auto video = videoFile;
    auto thumb = thumbFile;
    auto mp4 = beatFile.withFileExtension ("mp4");

    if (mp4 == video)
        mp4 = beatFile.getSiblingFile (beatFile.getFileNameWithoutExtension() + " - youtube.mp4");

    setBusy (true);
    setStatus ("Rendering video...");
    juce::Component::SafePointer<BeatUploaderVSTAudioProcessorEditor> safeThis (this);

    juce::Thread::launch ([safeThis, beat, image, video, thumb, mp4, useVideo, useApi, api, title, description, tags]
    {
        juce::StringArray args { "ffmpeg", "-y", "-loglevel", "error" };
        if (useVideo)
            args.addArray ({ "-stream_loop", "-1", "-i", video.getFullPathName() });
        else
            args.addArray ({ "-loop", "1", "-framerate", "2", "-i", image.getFullPathName() });

        args.addArray ({ "-i", beat.getFullPathName(), "-map", "0:v:0", "-map", "1:a:0" });
        const juce::String filter = useVideo
            ? (videoFillsFrame
                ? "scale=1920:1080:force_original_aspect_ratio=increase,crop=1920:1080,setsar=1"
                : "scale=1080:1080:force_original_aspect_ratio=increase,crop=1080:1080,pad=1920:1080:420:0:black,setsar=1")
            : "scale=1080:1080:force_original_aspect_ratio=increase,crop=1080:1080,pad=1920:1080:420:0:black";
        args.addArray ({ "-vf", filter, "-r", "30", "-c:v", "libx264", "-pix_fmt", "yuv420p" });
        if (useVideo)
            args.addArray ({ "-preset", "veryfast", "-crf", "20" });
        else
            args.addArray ({ "-tune", "stillimage" });
        args.addArray ({ "-c:a", "aac", "-b:a", "320k", "-shortest", "-movflags", "+faststart", mp4.getFullPathName() });

        juce::ChildProcess process;
        juce::String output;
        int exitCode = -1;
        const bool started = process.start (args);
        if (started)
        {
            output = process.readAllProcessOutput();
            process.waitForProcessToFinish (60000);
            exitCode = (int) process.getExitCode();
        }
        else
        {
            output = "Could not start ffmpeg. Add it to your PATH and restart FL Studio.";
        }

        const bool renderOk = started && exitCode == 0;
        if (! renderOk || ! useApi)
        {
            juce::MessageManager::callAsync ([safeThis, mp4, renderOk, output]
            {
                if (safeThis != nullptr)
                    safeThis->onRenderFinished (renderOk, mp4, output);
            });
            return;
        }

        juce::MessageManager::callAsync ([safeThis]
        {
            if (safeThis != nullptr)
                safeThis->setStatus ("Uploading to YouTube as " + uploadPrivacy + "...");
        });

        const auto result = api->upload (mp4, thumb, title, description, tags, uploadPrivacy);
        juce::MessageManager::callAsync ([safeThis, result]
        {
            if (safeThis != nullptr)
                safeThis->onUploadFinished (result.ok, result.message, result.warning);
        });
    });
}

void BeatUploaderVSTAudioProcessorEditor::onRenderFinished (bool ok, const juce::File& video,
                                                             const juce::String& log)
{
    setBusy (false);
    if (! ok)
    {
        setStatus ("ffmpeg failed: " + log.trim().substring (0, 160), true);
        return;
    }

    juce::SystemClipboard::copyTextToClipboard (titleEditor.getText());
    const auto infoFile = video.getSiblingFile (video.getFileNameWithoutExtension() + " - youtube.txt");
    infoFile.replaceWithText ("TITLE\n" + titleEditor.getText()
                              + "\n\nDESCRIPTION\n" + descriptionEditor.getText()
                              + "\n\nTAGS\n" + makeTags().joinIntoString (",") + "\n");
    video.revealToUser();
    juce::URL ("https://www.youtube.com/upload").launchInDefaultBrowser();
    setStatus ("Done! Title copied, description saved next to the video. Drag "
               + video.getFileName() + " into YouTube.");
}

void BeatUploaderVSTAudioProcessorEditor::onUploadFinished (bool ok, const juce::String& message,
                                                             const juce::String& warning)
{
    setBusy (false);
    if (! ok)
    {
        setStatus ("Upload failed: " + message.substring (0, 200) + " (the video is saved next to your beat)", true);
        return;
    }

    juce::SystemClipboard::copyTextToClipboard ("https://youtu.be/" + message);
    juce::URL ("https://studio.youtube.com/video/" + message + "/edit").launchInDefaultBrowser();
    auto status = "Uploaded as " + uploadPrivacy + ". Link copied.";
    if (warning.isNotEmpty())
        status += " Warning: " + warning;
    setStatus (status, warning.isNotEmpty());
}

void BeatUploaderVSTAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace ToxicColours;
    g.fillAll (background());
    g.setColour (violet());
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("FEDDY BEATZ", 30, 16, 300, 14, juce::Justification::centredLeft);
    g.setColour (lime());
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    g.drawText ("Beat Uploader", 30, 30, 400, 40, juce::Justification::centredLeft);

    drawCard (g, { 30, 84, 420, 400 }, "1   FILES");
    drawCard (g, { 470, 84, 420, 400 }, "2   DETAILS");

    const juce::Rectangle<float> frame (50.0f, 372.0f, 160.0f, 90.0f);
    const juce::Rectangle<float> square (frame.getCentreX() - 45.0f, frame.getY(), 90.0f, 90.0f);
    g.setColour (juce::Colours::black);
    g.fillRect (frame);

    if (videoFile.existsAsFile())
    {
        const auto area = videoFillsFrame ? frame : square;
        g.setColour (violet().withAlpha (0.3f));
        g.fillRect (area);
        g.setColour (lime());
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("VIDEO (muted)", area.toNearestInt(), juce::Justification::centred);
    }
    else if (coverThumb.isValid())
    {
        juce::Graphics::ScopedSaveState saveState (g);
        g.reduceClipRegion (square.toNearestInt());
        g.drawImage (coverThumb, square,
                     juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
    }
    else
    {
        g.setColour (violet().withAlpha (0.25f));
        g.fillRect (square);
        g.setColour (muted());
        g.setFont (juce::FontOptions (11.0f));
        g.drawText ("cover", square.toNearestInt(), juce::Justification::centred);
    }

    g.setColour (violet().withAlpha (0.6f));
    g.drawRect (frame, 1.0f);
    g.setColour (muted());
    g.setFont (juce::FontOptions (12.0f));

    const juce::String caption = videoFile.existsAsFile()
        ? (videoFillsFrame ? "Video: 1920 x 1080\nBackground video fills the frame.\nYour beat is the only audio."
                           : "Video: 1920 x 1080\nBackground video: 1080 x 1080, centered.\nYour beat is the only audio.")
        : "Video: 1920 x 1080\nCover: 1080 x 1080, centered";
    g.drawFittedText (caption, { 226, 372, 210, 90 }, juce::Justification::centredLeft, 4);
}

void BeatUploaderVSTAudioProcessorEditor::resized()
{
    channelLabel.setBounds (400, 30, 300, 22);
    connectButton.setBounds (710, 22, 180, 38);

    selectBeatButton.setBounds  (50, 118, 190, 32);
    beatFileLabel.setBounds     (50, 152, 380, 18);
    selectCoverButton.setBounds (50, 178, 190, 32);
    coverFileLabel.setBounds    (50, 212, 380, 18);
    selectVideoButton.setBounds (50, 238, 190, 32);
    clearVideoButton.setBounds  (250, 242, 60, 24);
    videoFileLabel.setBounds    (50, 272, 380, 18);
    selectThumbButton.setBounds (50, 298, 190, 32);
    clearThumbButton.setBounds  (250, 302, 60, 24);
    thumbFileLabel.setBounds    (50, 332, 380, 32);

    nameLabel.setBounds         (490, 120, 380, 16);
    nameEditor.setBounds        (490, 138, 380, 32);
    titleLabel.setBounds        (490, 184, 300, 16);
    resetTitleButton.setBounds  (810, 182, 60, 22);
    titleEditor.setBounds       (490, 206, 380, 32);
    descLabel.setBounds         (490, 252, 300, 16);
    resetDescButton.setBounds   (810, 250, 60, 22);
    descriptionEditor.setBounds (490, 274, 380, 192);

    uploadButton.setBounds (30, 504, 250, 60);
    statusLabel.setBounds (300, 504, 590, 28);
    progressBar.setBounds (300, 544, 590, 12);
}
