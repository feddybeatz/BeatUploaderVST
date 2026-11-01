#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ToxicPulseLookAndFeel.h"
#include "YouTubeUploader.h"

class BeatUploaderVSTAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    BeatUploaderVSTAudioProcessorEditor (BeatUploaderVSTAudioProcessor&);
    ~BeatUploaderVSTAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void onRenderFinished (bool ok, const juce::File& video, const juce::String& log);
    void onYouTubeConnectFinished (const YouTubeUploader::Result& result);
    void onYouTubeUploadFinished (const YouTubeUploader::Result& result);

private:
    void chooseBeat();
    void chooseCover();
    void startUpload();
    void connectYouTube();
    void uploadToYouTube();
    void refreshYouTubeUi();
    void setBusy (bool shouldBeBusy);
    void updateTitlePreview();
    void setStatus (const juce::String& text, bool isError = false);
    juce::String buildTitle() const;

    BeatUploaderVSTAudioProcessor& audioProcessor;
    ToxicPulseLookAndFeel toxicTheme;

    juce::TextButton selectBeatButton, selectCoverButton, uploadButton, youtubeButton;
    juce::Label beatFileLabel, coverFileLabel, nameLabel, bpmLabel, keyLabel,
                titleHeadingLabel, titlePreviewLabel, statusLabel, youtubeLabel;
    juce::TextEditor nameEditor, bpmEditor;
    juce::ComboBox keySelector;

    // must be declared before progressBar (it holds a reference to it)
    double progressValue = -1.0;   // -1 = indeterminate
    juce::ProgressBar progressBar { progressValue };

    std::unique_ptr<juce::FileChooser> chooser;
    juce::File beatFile, coverImage, lastVideo;
    juce::Image coverThumb;

    bool bpmEditedByUser = false;
    bool busy = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatUploaderVSTAudioProcessorEditor)
};
