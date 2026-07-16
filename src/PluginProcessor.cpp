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

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "shape", "Shape",
        juce::StringArray{"Exponential", "Linear", "Logarithmic", "Sine"},
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "attackShape", "Attack Shape", 0.0f, 1.0f, 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "recoveryShape", "Recovery Shape", 0.0f, 1.0f, 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"attackFrac", 1},
        "Attack Time",
        juce::NormalisableRange<float>(0.01f, 0.25f),
        0.05f,
        juce::AudioParameterFloatAttributes{}
            .withStringFromValueFunction([](float v, int)
                                         { return juce::String(v, 2); })
            .withValueFromStringFunction([](const juce::String &s)
                                         { return s.getFloatValue(); })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"recoverFrac", 1},
        "Recovery Time",
        juce::NormalisableRange<float>(0.01f, 0.25f),
        0.05f,
        juce::AudioParameterFloatAttributes{}
            .withStringFromValueFunction([](float v, int)
                                         { return juce::String(v, 2); })
            .withValueFromStringFunction([](const juce::String &s)
                                         { return s.getFloatValue(); })));

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

float WaddleAudioProcessor::getEnvelopeGain(float p, float curve, int shape)
{
    switch (shape)
    {
    case 0: // Exponential — slow start, fast finish (convex)
    {
        float exponent = 1.0f + curve * 4.0f; // 1.0 to 5.0
        return std::pow(p, exponent);
    }

    case 1: // Linear — always straight
        return p;

    case 2: // Logarithmic — fast start, slow finish (concave)
    {
        // base minimum is 2.0 to avoid log(1)=0 division by zero
        float base = 2.0f + curve * 48.0f;
        return std::log(1.0f + p * (base - 1.0f)) / std::log(base);
    }

    case 3: // Sine — curve 0 = almost straight, curve 1 = almost stair
    {
        float sharpness = 1.0f + curve * 19.0f; // range: 1 to 20
        float centered = (p - 0.5f) * sharpness;
        float sigmoid = 1.0f / (1.0f + std::exp(-centered));

        // Normalise so it goes from 0 to 1
        float lo = 1.0f / (1.0f + std::exp(0.5f * sharpness));
        float hi = 1.0f / (1.0f + std::exp(-0.5f * sharpness));
        return (sigmoid - lo) / (hi - lo);
    }

    default:
    {
        float exponent = 1.0f + curve * 4.0f;
        return std::pow(p, exponent);
    }
    }
}

static float sigmoidSecondHalf(float t, float amount)
{
    float sharpness = 1.0f + (1.0f - amount) * 19.0f;
    float inputVal = t * 0.5f * sharpness;
    float sigmoid = 1.0f / (1.0f + std::exp(-inputVal));
    float lo = 0.5f;
    float hi = 1.0f / (1.0f + std::exp(-0.5f * sharpness));
    return (sigmoid - lo) / (hi - lo); // Yields 0.0 -> 1.0
}

static float sigmoidFirstHalf(float t, float amount)
{
    float sharpness = 1.0f + (1.0f - amount) * 19.0f;
    float inputVal = (t - 1.0f) * 0.5f * sharpness;
    float sigmoid = 1.0f / (1.0f + std::exp(-inputVal));
    float lo = 1.0f / (1.0f + std::exp(0.5f * sharpness));
    float hi = 0.5f;
    return (sigmoid - lo) / (hi - lo);
}

float WaddleAudioProcessor::computeOpenness(float phase, float curve, int shape,
                                            float attackAmt, float recoveryAmt,
                                            float attackFrac, float recoverFrac)
{
    // Defensive: guarantee release never collapses to zero or negative width
    float releaseFrac = 1.0f - attackFrac - recoverFrac;
    if (releaseFrac < 0.01f)
    {
        float scale = (1.0f - 0.01f) / (attackFrac + recoverFrac);
        attackFrac *= scale;
        recoverFrac *= scale;
        releaseFrac = 0.01f;
    }

    if (phase < attackFrac)
    {
        float t = phase / attackFrac;
        float eased = sigmoidSecondHalf(t, attackAmt);
        return 0.5f * (1.0f - eased);
    }
    else if (phase < attackFrac + releaseFrac)
    {
        float t = (phase - attackFrac) / releaseFrac;
        return WaddleAudioProcessor::getEnvelopeGain(t, curve, shape);
    }
    else
    {
        float t = (phase - attackFrac - releaseFrac) / recoverFrac;
        float eased = sigmoidFirstHalf(t, recoveryAmt);
        return 1.0f - 0.5f * eased;
    }
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
    int shape = (int)apvts.getRawParameterValue("shape")->load();
    float attackAmt = apvts.getRawParameterValue("attackShape")->load();
    float recoveryAmt = apvts.getRawParameterValue("recoveryShape")->load();
    float attackFrac = apvts.getRawParameterValue("attackFrac")->load();
    float recoverFrac = apvts.getRawParameterValue("recoverFrac")->load();

    const double rateValues[] = {4.0, 2.0, 1.0, 0.5};
    double rate = rateValues[rateIndex];

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        double beatPosition = *ppqOpt + (sample / currentSampleRate) * (bpm / 60.0);
        phase = std::fmod(beatPosition, rate) / rate;

        float openness = computeOpenness((float)phase, curve, shape,
                                         attackAmt, recoveryAmt,
                                         attackFrac, recoverFrac);
        float gain = 1.0f - depth * (1.0f - openness);
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