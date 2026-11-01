#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ToxicPulseLookAndFeel.h"
#include "YouTubeUploader.h"

class BeatUploaderVSTAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    BeatUploaderVSTAudioProcessorEditor (BeatUploaderVSTAudioProcessor&);
    ~BeatUploaderVSTAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // called from the background thread (via the message thread) when work finishes
    void onRenderFinished (bool ok, const juce::File& video, const juce::String& log);
    void onUploadFinished (bool ok, const juce::String& message, const juce::String& warning);

private:
    void pickFile (const juce::String& prompt, const juce::File& startDir, const juce::String& wildcards,
                   std::function<void (const juce::File&)> onPicked);
    void chooseBeat();
    void chooseCover();
    void chooseVideo();
    void chooseThumbnail();
    void connectChannel();
    void startUpload();

    void applyTemplates();
    void refreshChannelLabel();
    void updateReadyStatus();
    void setStatus (const juce::String& text, bool isError = false);
    void setBusy (bool shouldBeBusy);

    BeatUploaderVSTAudioProcessor& audioProcessor;
    ToxicPulseLookAndFeel toxicTheme;

    juce::TextButton selectBeatButton, selectCoverButton, selectVideoButton, selectThumbButton,
                     clearVideoButton, clearThumbButton, resetTitleButton, resetDescButton,
                     uploadButton, connectButton;

    juce::Label beatFileLabel, coverFileLabel, videoFileLabel, thumbFileLabel,
                nameLabel, titleLabel, descLabel, statusLabel, channelLabel;

    juce::TextEditor nameEditor, titleEditor, descriptionEditor;

    // must be declared before progressBar (it holds a reference to it)
    double progressValue = -1.0;   // -1 = indeterminate
    juce::ProgressBar progressBar { progressValue };

    std::unique_ptr<juce::FileChooser> chooser;
    juce::File beatFile, coverImage, videoFile, thumbFile;
    juce::Image coverThumb;

    bool titleEdited = false;      // true once you type in the title box (stops auto-filling it)
    bool descEdited  = false;
    bool busy = false;

    std::shared_ptr<YouTubeUploader> uploader = std::make_shared<YouTubeUploader>();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatUploaderVSTAudioProcessorEditor)
};
