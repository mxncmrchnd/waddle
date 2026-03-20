#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WaddleAudioProcessorEditor::WaddleAudioProcessorEditor (WaddleAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setSize (400, 300);
}

WaddleAudioProcessorEditor::~WaddleAudioProcessorEditor()
{
}

//==============================================================================
void WaddleAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Waddle", getLocalBounds(), juce::Justification::centred, 1);
}

void WaddleAudioProcessorEditor::resized()
{
}