#pragma once

#include "PluginProcessor.h"
#include "YouTubeUploader.h"

// >>> CHECK THESE TWO LINES against your project <<<
// Your .cpp uses ToxicColours::, ToxicFonts:: and a LookAndFeel called toxicTheme.
// Include whichever header defines them, and use that LookAndFeel's real class name below.
#include "ToxicPulseLookAndFeel.h"
using ToxicLookAndFeelType = ToxicPulseLookAndFeel;

//==============================================================================
// A clickable / droppable tile that shows one chosen file.
class FileTile : public juce::Component
{
public:
    enum class Kind { beat, cover, video, thumbnail };

    FileTile (Kind kindToUse, const juce::String& titleText, bool isOptional);

    void setFile (const juce::File& f) { file = f; repaint(); }
    const juce::File& getFile() const  { return file; }

    void paint (juce::Graphics& g) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }

    std::function<void()> onClick;
    std::function<void()> onClear;   // only used by optional tiles (the little X)

private:
    juce::Rectangle<float> getClearBounds() const;

    Kind kind;
    juce::String title;
    bool optional;
    juce::File file;
    bool hover = false;
    bool dragOver = false;
};

//==============================================================================
// Pill in the top right: "Connect channel" or the connected channel's name.
class ChannelChip : public juce::Component
{
public:
    ChannelChip() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void setChannel (const juce::String& channelName) { name = channelName; repaint(); }

    void paint (juce::Graphics& g) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }
    void enablementChanged() override { repaint(); }

    std::function<void()> onClick;

private:
    juce::String name;
    bool hover = false;
};

//==============================================================================
// Status line + step labels (Render / Upload / Thumbnail / Done) + progress bar.
class StepStrip : public juce::Component, private juce::Timer
{
public:
    ~StepStrip() override { stopTimer(); }

    // stage: -1 idle, 0 render, 1 upload, 2 thumbnail, 3 done.  progress < 0 = indeterminate.
    void setStage (int newStage, double newProgress = -1.0);
    void setProgress (double newProgress);
    void setMessage (const juce::String& text, bool isError = false);
    void setThumbnailStepUsed (bool used) { thumbUsed = used; repaint(); }

    void paint (juce::Graphics& g) override;

private:
    void updateTimer();
    void timerCallback() override;

    int stage = -1;
    double progress = -1.0;
    double phase = 0.0;
    juce::String message;
    bool error = false;
    bool thumbUsed = false;
};

//==============================================================================
class BeatUploaderVSTAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            public juce::FileDragAndDropTarget
{
public:
    explicit BeatUploaderVSTAudioProcessorEditor (BeatUploaderVSTAudioProcessor&);
    ~BeatUploaderVSTAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

    // File drag & drop
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragging = true;  repaint(); }
    void fileDragExit  (const juce::StringArray&) override           { dragging = false; repaint(); }

private:
    // UI helpers
    void applyTemplates();
    void updateTitleCount();
    void refreshChannelLabel();
    void updateReadyStatus();
    void setBusy (bool shouldBeBusy);

    // File picking
    void pickFile (const juce::String& prompt, const juce::File& startDir,
                   const juce::String& wildcards,
                   std::function<void (const juce::File&)> onPicked);
    void chooseBeat();
    void chooseCover();
    void chooseVideo();
    void chooseThumbnail();
    void setBeat (const juce::File& f);
    void setCover (const juce::File& f);
    void setVideo (const juce::File& f);
    void setThumbnail (const juce::File& f);

    // Actions
    void connectChannel();
    void startUpload();
    void onRenderFinished (bool ok, const juce::File& video, const juce::String& log);
    void onUploadFinished (bool ok, const juce::String& message, const juce::String& warning);

    //==========================================================================
    BeatUploaderVSTAudioProcessor& audioProcessor;

    // Declared first so it is destroyed last (the destructor also resets it).
    ToxicLookAndFeelType toxicTheme;

    std::shared_ptr<YouTubeUploader> uploader = std::make_shared<YouTubeUploader>();
    std::unique_ptr<juce::FileChooser> chooser;

    juce::File beatFile, coverImage, videoFile, thumbFile;
    juce::Image coverThumb;

    FileTile beatTile  { FileTile::Kind::beat,      "BEAT",             false };
    FileTile coverTile { FileTile::Kind::cover,     "COVER IMAGE",      false };
    FileTile videoTile { FileTile::Kind::video,     "BACKGROUND VIDEO", true  };
    FileTile thumbTile { FileTile::Kind::thumbnail, "THUMBNAIL",        true  };

    ChannelChip channelChip;
    StepStrip stepStrip;

    juce::TextButton uploadButton, resetTitleButton, resetDescButton;
    juce::TextEditor nameEditor, titleEditor, descriptionEditor;
    juce::Label nameLabel, titleLabel, descLabel, titleCountLabel;

    // Layout rectangles filled in resized() and used in paint()
    juce::Rectangle<int> previewCard, formCard, frameBounds, visibilityBadge;
    int footerTop = 0;

    bool busy = false;
    bool titleEdited = false;
    bool descEdited = false;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatUploaderVSTAudioProcessorEditor)
};