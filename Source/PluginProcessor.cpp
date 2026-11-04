#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BeatUploaderVSTAudioProcessor::BeatUploaderVSTAudioProcessor()
     : AudioProcessor (BusesProperties()
                      #if ! JucePlugin_IsMidiEffect
                       #if ! JucePlugin_IsSynth
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       #endif
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                      #endif
                       )
{
   
}

BeatUploaderVSTAudioProcessor::~BeatUploaderVSTAudioProcessor()
{
}

//==============================================================================
const juce::String BeatUploaderVSTAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

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

double BeatUploaderVSTAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BeatUploaderVSTAudioProcessor::getNumPrograms()
{
    return 1;
}

int BeatUploaderVSTAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BeatUploaderVSTAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String BeatUploaderVSTAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void BeatUploaderVSTAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void BeatUploaderVSTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void BeatUploaderVSTAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BeatUploaderVSTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    // The input is optional (it's removed in the standalone app); if present it must match the output.
    const auto& in = layouts.getMainInputChannelSet();
    if (! in.isDisabled() && in != out)
        return false;
   #endif

    return true;
  #endif
}
#endif

void BeatUploaderVSTAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    // Pass audio through untouched; silence any output channels without a matching input.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // Remember the DAW's tempo (0 = unknown).
    double bpm = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;
    currentBPM.store (bpm);
}

//==============================================================================
bool BeatUploaderVSTAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* BeatUploaderVSTAudioProcessor::createEditor()
{
    return new BeatUploaderVSTAudioProcessorEditor (*this);
}

//==============================================================================
void BeatUploaderVSTAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void BeatUploaderVSTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BeatUploaderVSTAudioProcessor();
}