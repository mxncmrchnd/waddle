#pragma once
#include "PluginProcessor.h"

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

    WaddleAudioProcessor& processorRef;

    EnvelopeDisplay envelopeDisplay;

    juce::Slider depthKnob;
    juce::Label  depthLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment depthAttachment;

    juce::Slider curveKnob;
    juce::Label  curveLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment curveAttachment;

    static constexpr int numRateButtons = 4;
    juce::TextButton rateButtons[numRateButtons];
    const juce::String rateLabels[numRateButtons] = { "1/1", "1/2", "1/4", "1/8" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaddleAudioProcessorEditor)
};