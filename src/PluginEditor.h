#pragma once
#include "PluginProcessor.h"

//==============================================================================
// A button that draws a mini envelope curve instead of text
class ShapeButton : public juce::Component
{
public:
    std::function<void()> onClick;

    ShapeButton (int shapeIndex) : shape (shapeIndex) {}

    void setSelected (bool s)
    {
        selected = s;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);

        // Background
        g.setColour (selected ? juce::Colour (0xff3a3a3a) : juce::Colour (0xff2a2a2a));
        g.fillRoundedRectangle (bounds, 4.0f);

        // Border
        g.setColour (selected ? juce::Colour (128, 128, 128) : juce::Colour (0xff444444));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        // Mini curve
        auto curveBounds = bounds.reduced (6.0f, 6.0f);
        juce::Path curvePath;
        int numSteps = (int) curveBounds.getWidth();

        for (int i = 0; i <= numSteps; ++i)
        {
            float p = (float) i / (float) numSteps;
            // Use a fixed mid-curve value (0.5) for the preview
            float gain = WaddleAudioProcessor::getEnvelopeGain (p, 0.5f, shape);

            float x = curveBounds.getX() + p * curveBounds.getWidth();
            float y = curveBounds.getBottom() - gain * curveBounds.getHeight();

            if (i == 0)
                curvePath.startNewSubPath (x, y);
            else
                curvePath.lineTo (x, y);
        }

        g.setColour (selected ? juce::Colour (128, 128, 128) : juce::Colour (0xff555555));
        g.strokePath (curvePath, juce::PathStrokeType (1.5f));
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (onClick) onClick();
    }

private:
    int shape;
    bool selected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShapeButton)
};

//==============================================================================
// LookAndFeel that draws rotary knobs as a track + filled value arc,
// with no pointer/handle line.
class ArcLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto lineWidth = juce::jmax (2.0f, radius * 0.2f);
        auto arcRadius = radius - lineWidth * 0.5f;

        // Track (background arc)
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius,
                             0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
        g.strokePath (track, juce::PathStrokeType (lineWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Value fill arc
        auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        if (toAngle > rotaryStartAngle)
        {
            juce::Path valueArc;
            valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, toAngle, true);
            g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
            g.strokePath (valueArc, juce::PathStrokeType (lineWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
};

//==============================================================================
class EnvelopeDisplay : public juce::Component,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    EnvelopeDisplay (juce::AudioProcessorValueTreeState& apvts);
    ~EnvelopeDisplay() override;

    void paint (juce::Graphics& g) override;

private:
    void parameterChanged (const juce::String& paramID, float newValue) override;

    juce::AudioProcessorValueTreeState& apvts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeDisplay)
};

//==============================================================================
class WaddleAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit WaddleAudioProcessorEditor (WaddleAudioProcessor&);
    ~WaddleAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void parameterChanged (const juce::String& paramID, float newValue) override;
    void updateRateButtons();
    void updateShapeButtons();

    WaddleAudioProcessor& processorRef;

    ArcLookAndFeel arcLookAndFeel;

    EnvelopeDisplay envelopeDisplay;

    juce::Slider depthKnob;
    juce::Label  depthLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment depthAttachment;

    juce::Slider curveKnob;
    juce::Label  curveLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment curveAttachment;

    juce::Slider attackShapeKnob, attackFracKnob;
    juce::Label  attackShapeLabel, attackFracLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment attackShapeAttachment, attackFracAttachment;

    juce::Slider recoveryShapeKnob, recoverFracKnob;
    juce::Label  recoveryShapeLabel, recoverFracLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment recoveryShapeAttachment, recoverFracAttachment;

    static constexpr int numRateButtons = 4;
    juce::TextButton rateButtons[numRateButtons];
    const juce::String rateLabels[numRateButtons] = { "1/1", "1/2", "1/4", "1/8" };

    static constexpr int numShapeButtons = 4;
    ShapeButton shapeButtons[4] = { ShapeButton(0), ShapeButton(1), ShapeButton(2), ShapeButton(3) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaddleAudioProcessorEditor)
};