#include "PluginProcessor.h"
#include "PluginEditor.h"

WaddleAudioProcessorEditor::WaddleAudioProcessorEditor (WaddleAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      depthAttachment (p.apvts, "depth", depthKnob),
      curveAttachment (p.apvts, "curve", curveKnob)
{
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

    setSize (500, 300);
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
    g.fillAll (juce::Colour (0xff1a1a1a));

    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawFittedText ("Waddle", getLocalBounds().removeFromTop (50), juce::Justification::centred, 1);
}

void WaddleAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (50); // title space

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