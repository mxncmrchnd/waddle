#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>

static juce::Font getGeoformFont(float size)
{
    static auto typeface = juce::Typeface::createSystemTypefaceFor(
        BinaryData::Geoform_otf, BinaryData::Geoform_otfSize);
    return juce::Font(typeface).withHeight(size);
}

//==============================================================================
EnvelopeDisplay::EnvelopeDisplay(juce::AudioProcessorValueTreeState &a) : apvts(a)
{
    apvts.addParameterListener("depth", this);
    apvts.addParameterListener("curve", this);
    apvts.addParameterListener("shape", this);
    apvts.addParameterListener("attackShape", this);
    apvts.addParameterListener("recoveryShape", this);
    apvts.addParameterListener("attackFrac", this);
    apvts.addParameterListener("recoverFrac", this);
}

EnvelopeDisplay::~EnvelopeDisplay()
{
    apvts.removeParameterListener("depth", this);
    apvts.removeParameterListener("curve", this);
    apvts.removeParameterListener("shape", this);
    apvts.removeParameterListener("attackShape", this);
    apvts.removeParameterListener("recoveryShape", this);
    apvts.removeParameterListener("attackFrac", this);
    apvts.removeParameterListener("recoverFrac", this);
}

void EnvelopeDisplay::parameterChanged(const juce::String &, float)
{
    juce::MessageManager::callAsync([this]()
                                    { repaint(); });
}

void EnvelopeDisplay::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    g.setColour(juce::Colour(0xff0d0d0d));
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(juce::Colour(0xff444444));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    float depth = apvts.getRawParameterValue("depth")->load();
    float curve = apvts.getRawParameterValue("curve")->load();
    int shape = (int)apvts.getRawParameterValue("shape")->load();
    float attackAmt = apvts.getRawParameterValue("attackShape")->load();
    float recoveryAmt = apvts.getRawParameterValue("recoveryShape")->load();
    float attackFrac = apvts.getRawParameterValue("attackFrac")->load();
    float recoverFrac = apvts.getRawParameterValue("recoverFrac")->load();

    juce::Path envelopePath;
    int numSteps = (int)bounds.getWidth();

    for (int i = 0; i <= numSteps; ++i)
    {
        float phase = (float)i / (float)numSteps;
        float openness = WaddleAudioProcessor::computeOpenness(phase, curve, shape,
                                                               attackAmt, recoveryAmt,
                                                               attackFrac, recoverFrac);
        float gain = 1.0f - depth * (1.0f - openness);

        float x = bounds.getX() + phase * bounds.getWidth();
        float y = bounds.getBottom() - gain * bounds.getHeight();

        if (i == 0)
            envelopePath.startNewSubPath(x, y);
        else
            envelopePath.lineTo(x, y);
    }

    juce::Path filledPath = envelopePath;
    filledPath.lineTo(bounds.getRight(), bounds.getBottom());
    filledPath.lineTo(bounds.getX(), bounds.getBottom());
    filledPath.closeSubPath();

    g.setColour(juce::Colour(0x33808080));
    g.fillPath(filledPath);

    g.setColour(juce::Colour(0xff808080));
    g.strokePath(envelopePath, juce::PathStrokeType(1.5f));
}

//==============================================================================
WaddleAudioProcessorEditor::WaddleAudioProcessorEditor(WaddleAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p),
      envelopeDisplay(p.apvts),
      depthAttachment(p.apvts, "depth", depthKnob),
      curveAttachment(p.apvts, "curve", curveKnob),
      attackShapeAttachment(p.apvts, "attackShape", attackShapeKnob),
      attackFracAttachment(p.apvts, "attackFrac", attackFracKnob),
      recoveryShapeAttachment(p.apvts, "recoveryShape", recoveryShapeKnob),
      recoverFracAttachment(p.apvts, "recoverFrac", recoverFracKnob)
{
    auto grey = juce::Colour(128, 128, 128);

    addAndMakeVisible(envelopeDisplay);

    // Knobs
    auto setupKnob = [&](juce::Slider &knob, juce::Label &label, const juce::String &text)
    {
        knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
        knob.setNumDecimalPlacesToDisplay(2);
        knob.setColour(juce::Slider::textBoxTextColourId, grey);
        knob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        knob.setColour(juce::Slider::rotarySliderFillColourId, grey);
        knob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333333));
        knob.setLookAndFeel(&arcLookAndFeel);
        addAndMakeVisible(knob);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(getGeoformFont(11.0f));
        label.setColour(juce::Label::textColourId, grey);
        addAndMakeVisible(label);
    };
    setupKnob(depthKnob, depthLabel, "Depth");
    setupKnob(curveKnob, curveLabel, "Curve");
    setupKnob(attackShapeKnob, attackShapeLabel, "A.Shape");
    setupKnob(attackFracKnob, attackFracLabel, "A.Time");
    setupKnob(recoveryShapeKnob, recoveryShapeLabel, "R.Shape");
    setupKnob(recoverFracKnob, recoverFracLabel, "R.Time");

    attackFracKnob.textFromValueFunction = [](double value)
    {
        return juce::String(value, 2);
    };

    recoverFracKnob.textFromValueFunction = [](double value)
    {
        return juce::String(value, 2);
    };

    // Rate buttons
    for (int i = 0; i < numRateButtons; ++i)
    {
        rateButtons[i].setButtonText(rateLabels[i]);
        rateButtons[i].setClickingTogglesState(false);
        rateButtons[i].setColour(juce::TextButton::textColourOnId, grey);
        rateButtons[i].onClick = [this, i]()
        {
            auto *param = processorRef.apvts.getParameter("rate");
            float normalised = param->convertTo0to1((float)i);
            param->setValueNotifyingHost(normalised);
            updateRateButtons();
        };
        addAndMakeVisible(rateButtons[i]);
    }

    // Shape buttons
    for (int i = 0; i < numShapeButtons; ++i)
    {
        shapeButtons[i].onClick = [this, i]()
        {
            auto *param = processorRef.apvts.getParameter("shape");
            float normalised = param->convertTo0to1((float)i);
            param->setValueNotifyingHost(normalised);
            updateShapeButtons();
        };
        addAndMakeVisible(shapeButtons[i]);
    }

    processorRef.apvts.addParameterListener("rate", this);
    processorRef.apvts.addParameterListener("shape", this);

    updateRateButtons();
    updateShapeButtons();

    setSize(560, 440);
}

WaddleAudioProcessorEditor::~WaddleAudioProcessorEditor()
{
    processorRef.apvts.removeParameterListener("rate", this);
    processorRef.apvts.removeParameterListener("shape", this);

    depthKnob.setLookAndFeel(nullptr);
    curveKnob.setLookAndFeel(nullptr);
    attackShapeKnob.setLookAndFeel(nullptr);
    attackFracKnob.setLookAndFeel(nullptr);
    recoveryShapeKnob.setLookAndFeel(nullptr);
    recoverFracKnob.setLookAndFeel(nullptr);
}

void WaddleAudioProcessorEditor::parameterChanged(const juce::String &paramID, float)
{
    if (paramID == "rate")
        juce::MessageManager::callAsync([this]()
                                        { updateRateButtons(); });
    if (paramID == "shape")
        juce::MessageManager::callAsync([this]()
                                        { updateShapeButtons(); });
}

void WaddleAudioProcessorEditor::updateRateButtons()
{
    auto grey = juce::Colour(128, 128, 128);
    auto greyDark = juce::Colour(0xff2a2a2a);
    auto greyDimmed = juce::Colour(0xff555555);

    int currentRate = (int)processorRef.apvts.getRawParameterValue("rate")->load();

    for (int i = 0; i < numRateButtons; ++i)
    {
        bool isSelected = (i == currentRate);
        rateButtons[i].setColour(juce::TextButton::buttonColourId,
                                 isSelected ? juce::Colour(0xff3a3a3a) : greyDark);
        rateButtons[i].setColour(juce::TextButton::textColourOffId,
                                 isSelected ? grey : greyDimmed);
    }
}

void WaddleAudioProcessorEditor::updateShapeButtons()
{
    int currentShape = (int)processorRef.apvts.getRawParameterValue("shape")->load();
    for (int i = 0; i < numShapeButtons; ++i)
        shapeButtons[i].setSelected(i == currentShape);
}

void WaddleAudioProcessorEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));

    auto logo = juce::ImageCache::getFromMemory(
        BinaryData::logo_png, BinaryData::logo_pngSize);

    g.drawImageWithin(logo,
                      0, 0, getWidth(), 50,
                      juce::RectanglePlacement::centred);
}

void WaddleAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(50); // logo space

    // Envelope display
    envelopeDisplay.setBounds(area.removeFromTop(120));
    area.removeFromTop(8);

    // Shape buttons row below envelope display
    auto shapeArea = area.removeFromTop(50);
    int shapeButtonWidth = shapeArea.getWidth() / numShapeButtons;
    for (int i = 0; i < numShapeButtons; ++i)
        shapeButtons[i].setBounds(shapeArea.removeFromLeft(shapeButtonWidth).reduced(3));

    area.removeFromTop(8);

    // Rate buttons on the right
    auto rateArea = area.removeFromRight(100);
    int buttonHeight = rateArea.getHeight() / numRateButtons;
    for (int i = 0; i < numRateButtons; ++i)
        rateButtons[i].setBounds(rateArea.removeFromTop(buttonHeight).reduced(2));

    // Attack column (left) and Recovery column (right)
    auto attackColumn = area.removeFromLeft(90);
    auto recoveryColumn = area.removeFromRight(90);

    // Attack column (top: shape, bottom: time)
    auto attackTop = attackColumn.removeFromTop(attackColumn.getHeight() / 2);
    attackShapeLabel.setBounds(attackTop.removeFromTop(14));
    attackShapeKnob.setBounds(attackTop.reduced(4));
    attackFracLabel.setBounds(attackColumn.removeFromTop(14));
    attackFracKnob.setBounds(attackColumn.reduced(4));

    // Recovery column (top: shape, bottom: time)
    auto recoveryTop = recoveryColumn.removeFromTop(recoveryColumn.getHeight() / 2);
    recoveryShapeLabel.setBounds(recoveryTop.removeFromTop(14));
    recoveryShapeKnob.setBounds(recoveryTop.reduced(4));
    recoverFracLabel.setBounds(recoveryColumn.removeFromTop(14));
    recoverFracKnob.setBounds(recoveryColumn.reduced(4));

    // Depth knob on the left, Curve knob on the right
    auto centreStrip = area.withSizeKeepingCentre(area.getWidth(), 110);
    auto depthArea = centreStrip.removeFromLeft(centreStrip.getWidth() / 2);
    auto curveArea = centreStrip;
    
    auto layoutCentreControl = [](juce::Rectangle<int> area,
                                  juce::Label &label,
                                  juce::Slider &slider)
    {
        constexpr int labelH = 16;
        constexpr int sliderW = 70;
        constexpr int sliderH = 86; // rotary knob + 16 px text box
        const int totalH = labelH + 2 + sliderH;
        // Vertically center the whole group
        auto group = area.withSizeKeepingCentre(area.getWidth(), totalH);
        label.setBounds(group.removeFromTop(labelH));
        group.removeFromTop(2);
        slider.setBounds(group.withSizeKeepingCentre(sliderW, sliderH));
    };
    layoutCentreControl(depthArea, depthLabel, depthKnob);
    layoutCentreControl(curveArea, curveLabel, curveKnob);
}