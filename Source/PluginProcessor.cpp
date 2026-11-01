#include "PluginProcessor.h"
#include "PluginEditor.h"

BeatUploaderVSTAudioProcessor::BeatUploaderVSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{}

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

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

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
    return new BeatUploaderVSTAudioProcessorEditor (*this);
}

void BeatUploaderVSTAudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void BeatUploaderVSTAudioProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BeatUploaderVSTAudioProcessor();
}
