/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BeatUploaderVSTAudioProcessorEditor::BeatUploaderVSTAudioProcessorEditor (BeatUploaderVSTAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 500);
    startTimerHz(30);
    
    //mount the button to the vst window 
    captureButton.setButtonText("Capture Beat");
    addAndMakeVisible(captureButton);

    //mount the label to the vst window
    tempoLabel.setText("BPM: --", juce::dontSendNotification);
    addAndMakeVisible(tempoLabel);


    captureButton.onClick=[this](){
        //check if we are already recording
        if(audioProcessor.isRecording.load()){
            //stoprecording
            audioProcessor.isRecording.store(false); //flip switch off
            audioProcessor.formatWriter.reset(); //destroy the notepad, closing the file
            captureButton.setButtonText("capture Beat"); //reset button text

        }
        else{
            //start recording
            //launch asychronous "save as " window
            chooser=std::make_unique<juce::FileChooser> ("Save Beat As...", juce::File::getSpecialLocation(juce::File::userDesktopDirectory),"*.wav");

            auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

            chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc){
                juce::File selectedFile = fc.getResult();

                //if thr user selected a file and didnt hit cancel
                if(selectedFile != juce::File{}){
                    audioProcessor.outputFile = selectedFile;

                    juce::WavAudioFormat wavFormat;
                    audioProcessor.formatWriter.reset (wavFormat.createWriterFor (new juce::FileOutputStream (audioProcessor.outputFile), audioProcessor.getSampleRate(),
                2,24,{},0));
                //if the file was created successfully, flip the switch to ON
                if (audioProcessor.formatWriter != nullptr){
                    audioProcessor.isRecording.store(true);
                    captureButton.setButtonText("Stop Recording...");
                }
                }
            });
        }
    };
}

BeatUploaderVSTAudioProcessorEditor::~BeatUploaderVSTAudioProcessorEditor()
{
}

//==============================================================================
void BeatUploaderVSTAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void BeatUploaderVSTAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    captureButton.setBounds(10,10,150,40);
    tempoLabel.setBounds(10,60,150,40);
}

void BeatUploaderVSTAudioProcessorEditor::timerCallback(){
    //load the safe atomic variable and convert the number into text
    juce::String bpmText = juce::String (audioProcessor.currentBPM.load());

    //update the label on the screen
    tempoLabel.setText ("BPM: " + bpmText, juce::dontSendNotification);
}