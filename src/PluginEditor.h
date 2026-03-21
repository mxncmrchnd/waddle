#pragma once

#include "PluginProcessor.h"

class WaddleAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit WaddleAudioProcessorEditor (WaddleAudioProcessor&);
    ~WaddleAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    WaddleAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaddleAudioProcessorEditor)
};