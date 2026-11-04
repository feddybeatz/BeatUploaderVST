#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <fstream>

BeatUploaderVSTAudioProcessor::BeatUploaderVSTAudioProcessor()
     : AudioProcessor (BusesProperties()
                         .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                         .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // The standalone app has no use for audio input. Dropping it removes JUCE's yellow
    // "Audio input is muted" bar. In FL Studio the plugin keeps its stereo input.
    if (wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        removeBus (true);
}

BeatUploaderVSTAudioProcessor::~BeatUploaderVSTAudioProcessor() {}

const juce::String BeatUploaderVSTAudioProcessor::getName() const { return JucePlugin_Name; }

bool BeatUploaderVSTAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BeatUploaderVSTAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BeatUploaderVSTAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BeatUploaderVSTAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int BeatUploaderVSTAudioProcessor::getNumPrograms() { return 1; }
int BeatUploaderVSTAudioProcessor::getCurrentProgram() { return 0; }
void BeatUploaderVSTAudioProcessor::setCurrentProgram (int) {}
const juce::String BeatUploaderVSTAudioProcessor::getProgramName (int) { return {}; }
void BeatUploaderVSTAudioProcessor::changeProgramName (int, const juce::String&) {}

void BeatUploaderVSTAudioProcessor::prepareToPlay (double, int) {}
void BeatUploaderVSTAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BeatUploaderVSTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // the input bus does not exist in the standalone app; otherwise it must match the output
    if (! layouts.getMainInputChannelSet().isDisabled()
         && layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
  #endif
}
#endif

void BeatUploaderVSTAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // audio passes through untouched; we only read the DAW tempo
    if (auto* playHead = getPlayHead())
        if (auto positionInfo = playHead->getPosition())
            if (positionInfo->getBpm().hasValue())
                currentBPM = *positionInfo->getBpm();
}

bool BeatUploaderVSTAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* BeatUploaderVSTAudioProcessor::createEditor()
{
    std::ofstream log (R"(C:\Users\fedna\AppData\Local\Temp\beatuploader-debug.log)", std::ios::app);
    log << "createEditor\n"; log.flush();
    return new BeatUploaderVSTAudioProcessorEditor (*this);
}

void BeatUploaderVSTAudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void BeatUploaderVSTAudioProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BeatUploaderVSTAudioProcessor();
}
