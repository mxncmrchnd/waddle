#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>

static juce::Font getGeoformFont (float size)
{
    static auto typeface = juce::Typeface::createSystemTypefaceFor (
        BinaryData::Geoform_otf, BinaryData::Geoform_otfSize);
    return juce::Font (typeface).withHeight (size);
}

//==============================================================================
EnvelopeDisplay::EnvelopeDisplay (juce::AudioProcessorValueTreeState& a) : apvts (a)
{
    apvts.addParameterListener ("depth", this);
    apvts.addParameterListener ("curve", this);
    apvts.addParameterListener ("shape", this);
}

EnvelopeDisplay::~EnvelopeDisplay()
{
    apvts.removeParameterListener ("depth", this);
    apvts.removeParameterListener ("curve", this);
    apvts.removeParameterListener ("shape", this);
}

void EnvelopeDisplay::parameterChanged (const juce::String&, float)
{
    juce::MessageManager::callAsync ([this]() { repaint(); });
}

void EnvelopeDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);

    g.setColour (juce::Colour (0xff0d0d0d));
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (juce::Colour (0xff444444));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    float depth = apvts.getRawParameterValue ("depth")->load();
    float curve = apvts.getRawParameterValue ("curve")->load();
    int   shape = (int) apvts.getRawParameterValue ("shape")->load();

    juce::Path envelopePath;
    int numSteps = (int) bounds.getWidth();

    for (int i = 0; i <= numSteps; ++i)
    {
        float phase = (float) i / (float) numSteps;
        float gain  = 1.0f - depth * (1.0f - WaddleAudioProcessor::getEnvelopeGain (phase, curve, shape));

        float x = bounds.getX() + phase * bounds.getWidth();
        float y = bounds.getBottom() - gain * bounds.getHeight();

        if (i == 0)
            envelopePath.startNewSubPath (x, y);
        else
            envelopePath.lineTo (x, y);
    }

    juce::Path filledPath = envelopePath;
    filledPath.lineTo (bounds.getRight(), bounds.getBottom());
    filledPath.lineTo (bounds.getX(),     bounds.getBottom());
    filledPath.closeSubPath();

    g.setColour (juce::Colour (0x33808080));
    g.fillPath (filledPath);

    g.setColour (juce::Colour (0xff808080));
    g.strokePath (envelopePath, juce::PathStrokeType (1.5f));
}

//==============================================================================
WaddleAudioProcessorEditor::WaddleAudioProcessorEditor (WaddleAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      envelopeDisplay (p.apvts),
      depthAttachment (p.apvts, "depth", depthKnob),
      curveAttachment (p.apvts, "curve", curveKnob)
{
    auto grey = juce::Colour (128, 128, 128);

    addAndMakeVisible (envelopeDisplay);

    // Depth knob
    depthKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    depthKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    depthKnob.setColour (juce::Slider::textBoxTextColourId, grey);
    depthKnob.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    depthKnob.setColour (juce::Slider::rotarySliderFillColourId, grey);
    depthKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff333333));
    addAndMakeVisible (depthKnob);

    depthLabel.setText ("Depth", juce::dontSendNotification);
    depthLabel.setJustificationType (juce::Justification::centred);
    depthLabel.setFont (getGeoformFont (14.0f));
    depthLabel.setColour (juce::Label::textColourId, grey);
    addAndMakeVisible (depthLabel);

    // Curve knob
    curveKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    curveKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    curveKnob.setColour (juce::Slider::textBoxTextColourId, grey);
    curveKnob.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    curveKnob.setColour (juce::Slider::rotarySliderFillColourId, grey);
    curveKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff333333));
    addAndMakeVisible (curveKnob);

    curveLabel.setText ("Curve", juce::dontSendNotification);
    curveLabel.setJustificationType (juce::Justification::centred);
    curveLabel.setFont (getGeoformFont (14.0f));
    curveLabel.setColour (juce::Label::textColourId, grey);
    addAndMakeVisible (curveLabel);

    // Rate buttons
    for (int i = 0; i < numRateButtons; ++i)
    {
        rateButtons[i].setButtonText (rateLabels[i]);
        rateButtons[i].setClickingTogglesState (false);
        rateButtons[i].setColour (juce::TextButton::textColourOnId, grey);
        rateButtons[i].onClick = [this, i]()
        {
            auto* param = processorRef.apvts.getParameter ("rate");
            float normalised = param->convertTo0to1 ((float) i);
            param->setValueNotifyingHost (normalised);
            updateRateButtons();
        };
        addAndMakeVisible (rateButtons[i]);
    }

    // Shape buttons
    for (int i = 0; i < numShapeButtons; ++i)
    {
        shapeButtons[i].onClick = [this, i]()
        {
            auto* param = processorRef.apvts.getParameter ("shape");
            float normalised = param->convertTo0to1 ((float) i);
            param->setValueNotifyingHost (normalised);
            updateShapeButtons();
        };
        addAndMakeVisible (shapeButtons[i]);
    }

    processorRef.apvts.addParameterListener ("rate",  this);
    processorRef.apvts.addParameterListener ("shape", this);

    updateRateButtons();
    updateShapeButtons();

    setSize (500, 440);
}

WaddleAudioProcessorEditor::~WaddleAudioProcessorEditor()
{
    processorRef.apvts.removeParameterListener ("rate",  this);
    processorRef.apvts.removeParameterListener ("shape", this);
}

void WaddleAudioProcessorEditor::parameterChanged (const juce::String& paramID, float)
{
    if (paramID == "rate")
        juce::MessageManager::callAsync ([this]() { updateRateButtons(); });
    if (paramID == "shape")
        juce::MessageManager::callAsync ([this]() { updateShapeButtons(); });
}

void WaddleAudioProcessorEditor::updateRateButtons()
{
    auto grey       = juce::Colour (128, 128, 128);
    auto greyDark   = juce::Colour (0xff2a2a2a);
    auto greyDimmed = juce::Colour (0xff555555);

    int currentRate = (int) processorRef.apvts.getRawParameterValue ("rate")->load();

    for (int i = 0; i < numRateButtons; ++i)
    {
        bool isSelected = (i == currentRate);
        rateButtons[i].setColour (juce::TextButton::buttonColourId,
            isSelected ? juce::Colour (0xff3a3a3a) : greyDark);
        rateButtons[i].setColour (juce::TextButton::textColourOffId,
            isSelected ? grey : greyDimmed);
    }
}

void WaddleAudioProcessorEditor::updateShapeButtons()
{
    int currentShape = (int) processorRef.apvts.getRawParameterValue ("shape")->load();
    for (int i = 0; i < numShapeButtons; ++i)
        shapeButtons[i].setSelected (i == currentShape);
}

void WaddleAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));

    auto logo = juce::ImageCache::getFromMemory (
        BinaryData::logo_png, BinaryData::logo_pngSize);

    g.drawImageWithin (logo,
        0, 0, getWidth(), 50,
        juce::RectanglePlacement::centred);
}

void WaddleAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (50); // logo space

    // Envelope display
    envelopeDisplay.setBounds (area.removeFromTop (120));
    area.removeFromTop (8);

    // Shape buttons row below envelope display
    auto shapeArea = area.removeFromTop (50);
    int shapeButtonWidth = shapeArea.getWidth() / numShapeButtons;
    for (int i = 0; i < numShapeButtons; ++i)
        shapeButtons[i].setBounds (shapeArea.removeFromLeft (shapeButtonWidth).reduced (3));

    area.removeFromTop (8);

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