/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct ChainSettings
{
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};

using Filter = juce::dsp::IIR::Filter<float>;
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

enum ChainPositions
{
    LowCut,
    Peak,
    HighCut
};

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(Coefficients& old, const Coefficients& replacements);

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);

template<int index, typename ChainType, typename CoefficientType>
void update(ChainType& cut, const CoefficientType& cutCoefficients)
{
    updateCoefficients(cut.template get<index>().coefficients, cutCoefficients[index]);
    cut.template setBypassed<index>(false);
}

template<typename ChainType, typename CoefficientType>
void updateCutFilter(
    ChainType& cut,
    const CoefficientType& cutCoefficients,
    const Slope& cutSlope)
{
    cut.template setBypassed<0>(true);
    cut.template setBypassed<1>(true);
    cut.template setBypassed<2>(true);
    cut.template setBypassed<3>(true);

    switch (cutSlope)
    {
    case Slope_48:
        update<3>(cut, cutCoefficients);
    case Slope_36:
        update<2>(cut, cutCoefficients);
    case Slope_24:
        update<1>(cut, cutCoefficients);
    case Slope_12:
        update<0>(cut, cutCoefficients);
        break;
    }
}

inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowCutFreq,
        sampleRate,
        2 * (chainSettings.lowCutSlope + 1)
    );
}

inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
        chainSettings.highCutFreq,
        sampleRate,
        2 * (chainSettings.highCutSlope + 1)
    );
}

//==============================================================================
/**
*/
class _3Band_EQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    _3Band_EQAudioProcessor();
    ~_3Band_EQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()};

private:
    MonoChain leftChain, rightChain;

    void updatePeakFilter(const ChainSettings& chainSettings);
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);
    void updateFilters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (_3Band_EQAudioProcessor)
};
