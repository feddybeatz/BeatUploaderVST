#pragma once

#include <JuceHeader.h>

class BeatUploaderVSTAudioProcessor  : public juce::AudioProcessor
{
public:
    BeatUploaderVSTAudioProcessor();
    ~BeatUploaderVSTAudioProcessor() override;

    // tempo reported by the DAW (0 = unknown)
    std::atomic<double> currentBPM { 0.0 };

    // lets the standalone app drop its unused audio input bus (see the constructor)
    bool canRemoveBus (bool isInput) const override { return isInput && wrapperType == wrapperType_Standalone; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatUploaderVSTAudioProcessor)
};
