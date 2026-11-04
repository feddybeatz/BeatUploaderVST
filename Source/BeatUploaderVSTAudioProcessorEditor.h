#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ToxicPulseLookAndFeel.h"
#include "YouTubeUploader.h"

//==============================================================================
// A clickable, drop-friendly tile for one input file (beat, cover, video, thumbnail)
class FileTile : public juce::Component
{
public:
    enum class Kind { beat, cover, video, thumbnail };

    FileTile (Kind kindToUse, const juce::String& titleText, bool isOptional);

    void setFile (const juce::File& f)      { file = f; repaint(); }
    void setDragOver (bool shouldHighlight) { dragOver = shouldHighlight; repaint(); }

    std::function<void()> onClick;
    std::function<void()> onClear;

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }

private:
    juce::Rectangle<float> getClearBounds() const;

    Kind kind;
    juce::String title;
    bool optional;
    juce::File file;
    bool hover = false, dragOver = false;
};

//==============================================================================
// Pill in the header showing which YouTube channel uploads go to
class ChannelChip : public juce::Component
{
public:
    std::function<void()> onClick;

    void setChannel (const juce::String& channelName) { name = channelName; repaint(); }

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }

private:
    juce::String name;
    bool hover = false;
};

//==============================================================================
// Three steps (render, upload, thumbnail) with a progress bar and a status line
class StepStrip : public juce::Component, private juce::Timer
{
public:
    // stage: -1 idle, 0 rendering, 1 uploading, 2 thumbnail, 3 all done
    // progress: 0..1, or -1 for "working, unknown length" (animated bar)
    void setStage (int newStage, double newProgress = -1.0);
    void setProgress (double newProgress);
    void setMessage (const juce::String& text, bool isError = false);
    void setThumbnailStepUsed (bool used) { thumbUsed = used; repaint(); }

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void updateTimer();

    int stage = -1;
    double progress = -1.0;
    double phase = 0.0;
    juce::String message;
    bool error = false;
    bool thumbUsed = true;
};

//==============================================================================
class BeatUploaderVSTAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                             public juce::FileDragAndDropTarget
{
public:
    BeatUploaderVSTAudioProcessorEditor (BeatUploaderVSTAudioProcessor&);
    ~BeatUploaderVSTAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragging = true;  repaint(); }
    void fileDragExit (const juce::StringArray&) override            { dragging = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    // called (on the UI thread) when background work finishes
    void onRenderFinished (bool ok, const juce::File& video, const juce::String& log);
    void onUploadFinished (bool ok, const juce::String& message, const juce::String& warning);

private:
    void pickFile (const juce::String& prompt, const juce::File& startDir, const juce::String& wildcards,
                   std::function<void (const juce::File&)> onPicked);

    void chooseBeat();
    void chooseCover();
    void chooseVideo();
    void chooseThumbnail();

    void setBeat (const juce::File& f);
    void setCover (const juce::File& f);
    void setVideo (const juce::File& f);
    void setThumbnail (const juce::File& f);

    void connectChannel();
    void disconnectChannel();
    void startUpload();

    void applyTemplates();
    void updateTitleCount();
    void refreshChannelLabel();
    void updateReadyStatus();
    void setBusy (bool shouldBeBusy);

    BeatUploaderVSTAudioProcessor& audioProcessor;
    ToxicPulseLookAndFeel toxicTheme;

    FileTile beatTile  { FileTile::Kind::beat,      "BEAT",      false };
    FileTile coverTile { FileTile::Kind::cover,     "COVER",     false };
    FileTile videoTile { FileTile::Kind::video,     "VIDEO",     true  };
    FileTile thumbTile { FileTile::Kind::thumbnail, "THUMBNAIL", true  };

    ChannelChip channelChip;
    StepStrip stepStrip;

    juce::TextButton uploadButton, resetTitleButton, resetDescButton;
    juce::Label nameLabel, titleLabel, descLabel, titleCountLabel;
    juce::TextEditor nameEditor, titleEditor, descriptionEditor;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::File beatFile, coverImage, videoFile, thumbFile;
    juce::Image coverThumb;

    // layout rectangles, filled in by resized()
    juce::Rectangle<int> previewCard, frameBounds, formCard, visibilityBadge;
    int footerTop = 0;

    bool titleEdited = false;   // true once you type in the title box (stops auto-filling it)
    bool descEdited  = false;
    bool busy        = false;
    bool dragging    = false;

    std::shared_ptr<YouTubeUploader> uploader = std::make_shared<YouTubeUploader>();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatUploaderVSTAudioProcessorEditor)
};