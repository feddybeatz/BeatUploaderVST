#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::StringArray& getKeyNames()
    {
        static const juce::StringArray keys
        {
            "C Minor", "C# Minor", "D Minor", "Eb Minor", "E Minor", "F Minor",
            "F# Minor", "G Minor", "G# Minor", "A Minor", "Bb Minor", "B Minor",
            "C Major", "Db Major", "D Major", "Eb Major", "E Major", "F Major",
            "F# Major", "G Major", "Ab Major", "A Major", "Bb Major", "B Major"
        };
        return keys;
    }

    juce::String formatBpm (double bpm)
    {
        if (bpm <= 0.0)
            return {};

        if (std::abs (bpm - std::round (bpm)) < 0.01)
            return juce::String (juce::roundToInt (bpm));

        return juce::String (bpm, 1);
    }

    juce::String cleanName (const juce::String& raw)
    {
        auto name = raw.trim();
        return name.isEmpty() ? juce::String ("Beat Name") : name;
    }

    juce::String makeTitle (const juce::String& rawName)
    {
        auto title = "(FREE) Kehlani x Aaliyah R&B Type Beat - \"" + cleanName (rawName) + "\"";
        return title.substring (0, 100);   // YouTube title limit
    }

    juce::StringArray makeTags()
    {
        return
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

    void drawCard (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title)
    {
        using namespace ToxicColours;

        auto r = area.toFloat();
        g.setColour (card());
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (violet().withAlpha (0.35f));
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);

        g.setColour (violet());
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (title, area.getX() + 20, area.getY() + 10, area.getWidth() - 40, 18,
                    juce::Justification::centredLeft);
    }
}

BeatUploaderVSTAudioProcessorEditor::BeatUploaderVSTAudioProcessorEditor (BeatUploaderVSTAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    using namespace ToxicColours;

    setLookAndFeel (&toxicTheme);
    setSize (850, 530);

    auto setupLabel = [this] (juce::Label& l, const juce::String& text, float size,
                              juce::Colour colour, bool bold)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        l.setColour (juce::Label::textColourId, colour);
        l.setMinimumHorizontalScale (1.0f);
        addAndMakeVisible (l);
    };

    selectBeatButton.setButtonText ("Select Beat (WAV)");
    selectCoverButton.setButtonText ("Select Cover Image");
    youtubeButton.setButtonText ("Connect YouTube Channel");
    uploadButton.setButtonText ("Upload Beat");
    uploadButton.getProperties().set ("primary", true);
    addAndMakeVisible (selectBeatButton);
    addAndMakeVisible (selectCoverButton);
    addAndMakeVisible (youtubeButton);
    addAndMakeVisible (uploadButton);

    setupLabel (beatFileLabel, "No beat selected", 13.0f, muted(), false);
    setupLabel (coverFileLabel, "No cover selected", 13.0f, muted(), false);
    setupLabel (nameLabel, "BEAT NAME", 11.0f, muted(), true);
    setupLabel (bpmLabel, "BPM", 11.0f, muted(), true);
    setupLabel (keyLabel, "KEY", 11.0f, muted(), true);
    setupLabel (titleHeadingLabel, "YOUTUBE TITLE PREVIEW", 11.0f, muted(), true);
    setupLabel (titlePreviewLabel, "", 15.0f, lime(), false);
    setupLabel (youtubeLabel, "YouTube: not connected", 13.0f, muted(), false);
    setupLabel (statusLabel, "Connect a YouTube channel, then select beat and cover.", 14.0f, lime(), false);
    titlePreviewLabel.setJustificationType (juce::Justification::topLeft);

    nameEditor.setFont (juce::FontOptions (16.0f));
    nameEditor.setTextToShowWhenEmpty ("e.g. Midnight Drive", muted());
    nameEditor.onTextChange = [this] { updateTitlePreview(); };
    addAndMakeVisible (nameEditor);

    bpmEditor.setFont (juce::FontOptions (16.0f));
    bpmEditor.setInputRestrictions (6, "0123456789.");
    bpmEditor.setTextToShowWhenEmpty ("--", muted());
    bpmEditor.onTextChange = [this]
    {
        bpmEditedByUser = bpmEditor.getText().isNotEmpty();
        updateTitlePreview();
    };
    addAndMakeVisible (bpmEditor);

    keySelector.addItemList (getKeyNames(), 1);
    keySelector.setSelectedId (1, juce::dontSendNotification);
    keySelector.onChange = [this] { updateTitlePreview(); };
    addAndMakeVisible (keySelector);

    progressBar.setPercentageDisplay (false);
    addChildComponent (progressBar);

    selectBeatButton.onClick  = [this] { chooseBeat(); };
    selectCoverButton.onClick = [this] { chooseCover(); };
    youtubeButton.onClick     = [this] { connectYouTube(); };
    uploadButton.onClick      = [this] { startUpload(); };

    updateTitlePreview();
    refreshYouTubeUi();

    startTimerHz (10);
}

BeatUploaderVSTAudioProcessorEditor::~BeatUploaderVSTAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BeatUploaderVSTAudioProcessorEditor::chooseBeat()
{
    auto startDir = beatFile.existsAsFile() ? beatFile.getParentDirectory()
                                            : juce::File::getSpecialLocation (juce::File::userMusicDirectory);

    chooser = std::make_unique<juce::FileChooser> ("Select your exported beat", startDir, "*.wav;*.mp3;*.flac");

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();

        if (! f.existsAsFile())
            return;

        beatFile = f;
        beatFileLabel.setText (f.getFileName(), juce::dontSendNotification);
        beatFileLabel.setColour (juce::Label::textColourId, ToxicColours::lime());
        nameEditor.setText (f.getFileNameWithoutExtension(), true);
        setStatus (coverImage.existsAsFile() ? "Ready. Press Upload Beat to send it unlisted."
                                             : "Beat selected. Now choose a cover image.");
    });
}

void BeatUploaderVSTAudioProcessorEditor::chooseCover()
{
    auto startDir = coverImage.existsAsFile() ? coverImage.getParentDirectory()
                                              : juce::File::getSpecialLocation (juce::File::userPicturesDirectory);

    chooser = std::make_unique<juce::FileChooser> ("Select your cover image", startDir, "*.png;*.jpg;*.jpeg");

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();

        if (! f.existsAsFile())
            return;

        coverImage = f;
        coverThumb = juce::ImageFileFormat::loadFrom (f);
        coverFileLabel.setText (f.getFileName(), juce::dontSendNotification);
        coverFileLabel.setColour (juce::Label::textColourId, ToxicColours::lime());
        repaint();
        setStatus (beatFile.existsAsFile() ? "Ready. Press Upload Beat to send it unlisted."
                                           : "Cover selected. Now choose your beat.");
    });
}

juce::String BeatUploaderVSTAudioProcessorEditor::buildTitle() const
{
    return makeTitle (nameEditor.getText());
}

void BeatUploaderVSTAudioProcessorEditor::updateTitlePreview()
{
    titlePreviewLabel.setText (buildTitle(), juce::dontSendNotification);
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
    uploadButton.setEnabled (! shouldBeBusy);
    youtubeButton.setEnabled (! shouldBeBusy);
    progressBar.setVisible (shouldBeBusy);

    if (shouldBeBusy)
        progressValue = -1.0;
}

void BeatUploaderVSTAudioProcessorEditor::refreshYouTubeUi()
{
    YouTubeUploader uploader;

    if (uploader.isConnected())
    {
        auto name = uploader.getChannelName();
        youtubeButton.setButtonText ("Disconnect YouTube");
        youtubeLabel.setText ("YouTube: " + (name.isNotEmpty() ? name : juce::String ("connected")),
                              juce::dontSendNotification);
        youtubeLabel.setColour (juce::Label::textColourId, ToxicColours::lime());
    }
    else
    {
        youtubeButton.setButtonText ("Connect YouTube Channel");
        youtubeLabel.setText ("YouTube: not connected", juce::dontSendNotification);
        youtubeLabel.setColour (juce::Label::textColourId, ToxicColours::muted());
    }
}

void BeatUploaderVSTAudioProcessorEditor::connectYouTube()
{
    if (busy)
        return;

    YouTubeUploader uploader;

    if (uploader.isConnected())
    {
        uploader.disconnect();
        refreshYouTubeUi();
        setStatus ("YouTube disconnected.");
        return;
    }

    if (! uploader.hasClientConfig())
    {
        setStatus ("Missing youtube_client.json in " + uploader.getConfigFolder().getFullPathName(), true);
        return;
    }

    setBusy (true);
    setStatus ("Opening Google sign-in. Pick the channel, then return here...");

    juce::Component::SafePointer<BeatUploaderVSTAudioProcessorEditor> safeThis (this);

    juce::Thread::launch ([safeThis]
    {
        YouTubeUploader worker;
        auto result = worker.connect();

        juce::MessageManager::callAsync ([safeThis, result]
        {
            if (safeThis != nullptr)
                safeThis->onYouTubeConnectFinished (result);
        });
    });
}

void BeatUploaderVSTAudioProcessorEditor::onYouTubeConnectFinished (const YouTubeUploader::Result& result)
{
    setBusy (false);
    refreshYouTubeUi();

    if (! result.ok)
    {
        setStatus (result.message, true);
        return;
    }

    setStatus ("Connected to " + result.message + ". Videos will upload unlisted to this channel.");
}

void BeatUploaderVSTAudioProcessorEditor::startUpload()
{
    if (busy)
        return;

    YouTubeUploader uploader;

    if (! uploader.isConnected())
    {
        setStatus ("Connect a YouTube channel first.", true);
        return;
    }

    if (! beatFile.existsAsFile())
    {
        setStatus ("Select your beat first.", true);
        return;
    }

    if (! coverImage.existsAsFile())
    {
        setStatus ("Select a cover image first.", true);
        return;
    }

    setBusy (true);
    setStatus ("Rendering video...");

    auto wav = beatFile;
    auto image = coverImage;
    auto mp4 = beatFile.withFileExtension ("mp4");
    juce::Component::SafePointer<BeatUploaderVSTAudioProcessorEditor> safeThis (this);

    juce::Thread::launch ([safeThis, wav, image, mp4]
    {
        const juce::String filter = "scale=1080:1080:force_original_aspect_ratio=increase,"
                                    "crop=1080:1080,"
                                    "pad=1920:1080:420:0:black";

        juce::StringArray args { "ffmpeg", "-y", "-loglevel", "error",
                                 "-loop", "1", "-framerate", "2", "-i", image.getFullPathName(),
                                 "-i", wav.getFullPathName(), "-vf", filter, "-r", "30",
                                 "-c:v", "libx264", "-tune", "stillimage", "-pix_fmt", "yuv420p",
                                 "-c:a", "aac", "-b:a", "320k", "-shortest", "-movflags", "+faststart",
                                 mp4.getFullPathName() };

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
            output = "Could not start ffmpeg. Add it to your PATH and restart FL Studio.";
        }

        juce::MessageManager::callAsync ([safeThis, mp4, started, exitCode, output]
        {
            if (safeThis != nullptr)
                safeThis->onRenderFinished (started && exitCode == 0, mp4, output);
        });
    });
}

void BeatUploaderVSTAudioProcessorEditor::onRenderFinished (bool ok, const juce::File& video, const juce::String& log)
{
    if (! ok)
    {
        setBusy (false);
        setStatus ("ffmpeg failed: " + log.trim().substring (0, 160), true);
        return;
    }

    lastVideo = video;
    setStatus ("Uploading to YouTube as unlisted...");
    uploadToYouTube();
}

void BeatUploaderVSTAudioProcessorEditor::uploadToYouTube()
{
    auto video = lastVideo;
    auto title = buildTitle();
    auto description = makeDescription (nameEditor.getText());
    auto tags = makeTags();
    juce::Component::SafePointer<BeatUploaderVSTAudioProcessorEditor> safeThis (this);

    juce::Thread::launch ([safeThis, video, title, description, tags]
    {
        YouTubeUploader worker;
        auto result = worker.upload (video, title, description, tags, "unlisted");

        juce::MessageManager::callAsync ([safeThis, result]
        {
            if (safeThis != nullptr)
                safeThis->onYouTubeUploadFinished (result);
        });
    });
}

void BeatUploaderVSTAudioProcessorEditor::onYouTubeUploadFinished (const YouTubeUploader::Result& result)
{
    setBusy (false);

    if (! result.ok)
    {
        setStatus (result.message, true);
        return;
    }

    const auto url = "https://youtu.be/" + result.message;
    juce::SystemClipboard::copyTextToClipboard (url);
    setStatus ("Uploaded unlisted: " + url);
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

    drawCard (g, { 30, 84, 390, 310 }, "1   BEAT AND COVER");
    drawCard (g, { 430, 84, 390, 310 }, "2   DETAILS");

    juce::Rectangle<float> frame (50.0f, 278.0f, 160.0f, 90.0f);
    juce::Rectangle<float> square (frame.getCentreX() - 45.0f, frame.getY(), 90.0f, 90.0f);

    g.setColour (juce::Colours::black);
    g.fillRect (frame);

    if (coverThumb.isValid())
    {
        juce::Graphics::ScopedSaveState state (g);
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
    g.drawFittedText ("Video: 1920 x 1080\nCover: 1080 x 1080, centered",
                      { 226, 278, 174, 90 }, juce::Justification::centredLeft, 3);
}

void BeatUploaderVSTAudioProcessorEditor::resized()
{
    selectBeatButton.setBounds  (50, 120, 350, 40);
    beatFileLabel.setBounds     (50, 164, 350, 20);
    selectCoverButton.setBounds (50, 196, 350, 40);
    coverFileLabel.setBounds    (50, 240, 350, 20);

    nameLabel.setBounds         (450, 122, 350, 16);
    nameEditor.setBounds        (450, 140, 350, 34);
    bpmLabel.setBounds          (450, 188, 160, 16);
    bpmEditor.setBounds         (450, 206, 160, 34);
    keyLabel.setBounds          (630, 188, 170, 16);
    keySelector.setBounds       (630, 206, 170, 34);
    titleHeadingLabel.setBounds (450, 256, 350, 16);
    titlePreviewLabel.setBounds (450, 276, 350, 100);

    youtubeButton.setBounds     (30, 414, 250, 36);
    youtubeLabel.setBounds      (300, 414, 520, 36);
    uploadButton.setBounds      (30, 458, 250, 50);
    statusLabel.setBounds       (300, 458, 520, 24);
    progressBar.setBounds       (300, 488, 520, 12);
}

void BeatUploaderVSTAudioProcessorEditor::timerCallback()
{
    if (bpmEditedByUser || bpmEditor.hasKeyboardFocus (true))
        return;

    auto text = formatBpm (audioProcessor.currentBPM.load());

    if (text.isNotEmpty() && text != bpmEditor.getText())
    {
        bpmEditor.setText (text, false);
        updateTitlePreview();
    }
}
