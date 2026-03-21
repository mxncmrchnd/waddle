#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout WaddleAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "depth", "Depth", 0.0f, 1.0f, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "curve", "Curve", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "rate", "Rate",
        juce::StringArray{"1/1", "1/2", "1/4", "1/8"},
        2));

    return {params.begin(), params.end()};
}

WaddleAudioProcessor::WaddleAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         ),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

WaddleAudioProcessor::~WaddleAudioProcessor() {}

const juce::String WaddleAudioProcessor::getName() const { return JucePlugin_Name; }

bool WaddleAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool WaddleAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool WaddleAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double WaddleAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int WaddleAudioProcessor::getNumPrograms() { return 1; }
int WaddleAudioProcessor::getCurrentProgram() { return 0; }
void WaddleAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String WaddleAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}
void WaddleAudioProcessor::changeProgramName(int index, const juce::String &newName) { juce::ignoreUnused(index, newName); }

void WaddleAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;
    phase = 0.0;
    smoothedGain.reset(sampleRate, 0.005);
    smoothedGain.setCurrentAndTargetValue(1.0f);
}

void WaddleAudioProcessor::releaseResources() {}

bool WaddleAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif
    return true;
#endif
}

float WaddleAudioProcessor::getEnvelopeGain(float p, float curve)
{
    float exponent = 1.0f + curve * 9.0f;
    float raw = 1.0f - std::exp(-exponent * p);
    float max = 1.0f - std::exp(-exponent);
    return raw / max;
}

void WaddleAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto *ph = getPlayHead();
    if (ph == nullptr)
        return;

    auto position = ph->getPosition();
    if (!position.hasValue())
        return;
    if (!position->getIsPlaying())
        return;

    auto bpmOpt = position->getBpm();
    if (!bpmOpt.hasValue())
        return;
    double bpm = *bpmOpt;

    auto ppqOpt = position->getPpqPosition();
    if (!ppqOpt.hasValue())
        return;

    float depth = apvts.getRawParameterValue("depth")->load();
    float curve = apvts.getRawParameterValue("curve")->load();
    int rateIndex = (int)apvts.getRawParameterValue("rate")->load();
    const double rateValues[] = {4.0, 2.0, 1.0, 0.5};
    double rate = rateValues[rateIndex];

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        double beatPosition = *ppqOpt + (sample / currentSampleRate) * (bpm / 60.0);
        phase = std::fmod(beatPosition, rate) / rate;

        float gain = 1.0f - depth * (1.0f - getEnvelopeGain((float)phase, curve));
        smoothedGain.setTargetValue(gain);
        float smoothGain = smoothedGain.getNextValue();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            float *channelData = buffer.getWritePointer(channel);
            channelData[sample] *= smoothGain;
        }
    }
}

bool WaddleAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *WaddleAudioProcessor::createEditor()
{
    return new WaddleAudioProcessorEditor(*this);
}

void WaddleAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void WaddleAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new WaddleAudioProcessor();
}