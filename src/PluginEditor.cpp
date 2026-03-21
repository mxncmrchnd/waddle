#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>

EnvelopeDisplay::EnvelopeDisplay (juce::AudioProcessorValueTreeState& a) : apvts (a)
{
    apvts.addParameterListener ("depth", this);
    apvts.addParameterListener ("curve", this);
}

EnvelopeDisplay::~EnvelopeDisplay()
{
    apvts.removeParameterListener ("depth", this);
    apvts.removeParameterListener ("curve", this);
}

void EnvelopeDisplay::parameterChanged (const juce::String&, float)
{
    juce::MessageManager::callAsync ([this]() { repaint(); });
}

void EnvelopeDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);

    // Background
    g.setColour (juce::Colour (0xff0d0d0d));
    g.fillRoundedRectangle (bounds, 6.0f);

    // Border
    g.setColour (juce::Colour (0xff444444));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    float depth = apvts.getRawParameterValue ("depth")->load();
    float curve = apvts.getRawParameterValue ("curve")->load();

    // Build the envelope path by sampling getEnvelopeGain across the width
    juce::Path envelopePath;
    int numSteps = (int) bounds.getWidth();

    for (int i = 0; i <= numSteps; ++i)
    {
        float phase = (float) i / (float) numSteps;
        float gain  = 1.0f - depth * (1.0f - WaddleAudioProcessor::getEnvelopeGain (phase, curve));

        // Flip y: gain=1.0 is top of display, gain=0.0 is bottom
        float x = bounds.getX() + phase * bounds.getWidth();
        float y = bounds.getBottom() - gain * bounds.getHeight();

        if (i == 0)
            envelopePath.startNewSubPath (x, y);
        else
            envelopePath.lineTo (x, y);
    }

    // Filled area under the curve
    juce::Path filledPath = envelopePath;
    filledPath.lineTo (bounds.getRight(), bounds.getBottom());
    filledPath.lineTo (bounds.getX(),     bounds.getBottom());
    filledPath.closeSubPath();

    g.setColour (juce::Colour (0x3300aaff));
    g.fillPath (filledPath);

    // Curve line on top
    g.setColour (juce::Colour (0xff00aaff));
    g.strokePath (envelopePath, juce::PathStrokeType (1.5f));
}

//==============================================================================
WaddleAudioProcessorEditor::WaddleAudioProcessorEditor (WaddleAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      envelopeDisplay (p.apvts),
      depthAttachment (p.apvts, "depth", depthKnob),
      curveAttachment (p.apvts, "curve", curveKnob)
{
    addAndMakeVisible (envelopeDisplay);

    // Depth knob
    depthKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    depthKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (depthKnob);

    depthLabel.setText ("Depth", juce::dontSendNotification);
    depthLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (depthLabel);

    // Curve knob
    curveKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    curveKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (curveKnob);

    curveLabel.setText ("Curve", juce::dontSendNotification);
    curveLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (curveLabel);

    // Rate buttons
    for (int i = 0; i < numRateButtons; ++i)
    {
        rateButtons[i].setButtonText (rateLabels[i]);
        rateButtons[i].setClickingTogglesState (false);
        rateButtons[i].onClick = [this, i]()
        {
            auto* param = processorRef.apvts.getParameter ("rate");
            float normalised = param->convertTo0to1 ((float) i);
            param->setValueNotifyingHost (normalised);
            updateRateButtons();
        };
        addAndMakeVisible (rateButtons[i]);
    }

    processorRef.apvts.addParameterListener ("rate", this);
    updateRateButtons();

    setSize (500, 380);
}

WaddleAudioProcessorEditor::~WaddleAudioProcessorEditor()
{
    processorRef.apvts.removeParameterListener ("rate", this);
}

void WaddleAudioProcessorEditor::parameterChanged (const juce::String& paramID, float)
{
    if (paramID == "rate")
        juce::MessageManager::callAsync ([this]() { updateRateButtons(); });
}

void WaddleAudioProcessorEditor::updateRateButtons()
{
    int currentRate = (int) processorRef.apvts.getRawParameterValue ("rate")->load();

    for (int i = 0; i < numRateButtons; ++i)
    {
        bool isSelected = (i == currentRate);
        rateButtons[i].setColour (juce::TextButton::buttonColourId,
            isSelected ? juce::Colour (0xff00aaff) : juce::Colour (0xff2a2a2a));
        rateButtons[i].setColour (juce::TextButton::textColourOffId,
            isSelected ? juce::Colours::white : juce::Colour (0xff888888));
    }
}

void WaddleAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour (0xff1a1a1a));
    auto logo = juce::ImageCache::getFromMemory(
        BinaryData::logo_png, BinaryData::logo_pngSize);
    
    g.drawImageWithin (logo,
        0, 0, getWidth(), 50,
        juce::RectanglePlacement::centred);
}

void WaddleAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (50); // title space

    // Envelope display at the top
    envelopeDisplay.setBounds (area.removeFromTop (120));
    area.removeFromTop (10); // gap

    // Rate buttons on the right
    auto rateArea = area.removeFromRight (100);
    int buttonHeight = rateArea.getHeight() / numRateButtons;
    for (int i = 0; i < numRateButtons; ++i)
        rateButtons[i].setBounds (rateArea.removeFromTop (buttonHeight).reduced (2));

    // Depth knob on the left
    auto depthArea = area.removeFromLeft (area.getWidth() / 2);
    depthLabel.setBounds (depthArea.removeFromTop (20));
    depthKnob.setBounds (depthArea.reduced (10));

    // Curve knob in the middle
    curveLabel.setBounds (area.removeFromTop (20));
    curveKnob.setBounds (area.reduced (10));
}