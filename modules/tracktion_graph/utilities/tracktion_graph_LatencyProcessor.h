/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/

#pragma once

namespace tracktion_graph
{

//==============================================================================
//==============================================================================
struct LatencyProcessor
{
    LatencyProcessor() = default;
    
    /** Returns ture if the the sample rate, channels etc. are the same between the two objects. */
    bool hasSameConfigurationAs (const LatencyProcessor& o) const
    {
        return latencyNumSamples == o.latencyNumSamples
            && sampleRate == o.sampleRate
            && fifo.getNumChannels() == o.fifo.getNumChannels();
    }
    
    int getLatencyNumSamples() const
    {
        return latencyNumSamples;
    }
    
    void setLatencyNumSamples (int numLatencySamples)
    {
        latencyNumSamples = numLatencySamples;
    }
    
    void prepareToPlay (double sampleRateToUse, int blockSize, int numChannels)
    {
        sampleRate = sampleRateToUse;
        latencyTimeSeconds = sampleToTime (latencyNumSamples, sampleRate);
        
        fifo.setSize (numChannels, latencyNumSamples + blockSize + 1);
        fifo.writeSilence (latencyNumSamples);
        jassert (fifo.getNumReady() == latencyNumSamples);
    }
    
    void writeAudio (const juce::dsp::AudioBlock<float>& src)
    {
        if (fifo.getNumChannels() == 0)
            return;

        jassert (fifo.getNumChannels() == (int) src.getNumChannels());
        fifo.write (src);
    }
    
    void writeMIDI (const tracktion_engine::MidiMessageArray& src)
    {
        midi.mergeFromWithOffset (src, latencyTimeSeconds);
    }

    void readAudio (juce::dsp::AudioBlock<float>& dst)
    {
        if (fifo.getNumChannels() == 0)
            return;

        jassert (fifo.getNumReady() >= (int) dst.getNumSamples());
        fifo.readAdding (dst);
    }
    
    void readMIDI (tracktion_engine::MidiMessageArray& dst, int numSamples)
    {
        // And read out any delayed items
        const double blockTimeSeconds = sampleToTime (numSamples, sampleRate);

        for (int i = midi.size(); --i >= 0;)
        {
           auto& m = midi[i];
           
           if (m.getTimeStamp() <= blockTimeSeconds)
           {
               dst.add (m);
               midi.remove (i);
               // TODO: This will deallocate, we need a MIDI message array that doesn't adjust its storage
           }
        }

        // Shuffle down remaining items by block time
        midi.addToTimestamps (-blockTimeSeconds);

        // Ensure there are no negative time messages
        for (auto& m : midi)
        {
           juce::ignoreUnused (m);
           jassert (m.getTimeStamp() >= 0.0);
        }
    }
    
private:
    int latencyNumSamples = 0;
    double sampleRate = 44100.0;
    double latencyTimeSeconds = 0.0;
    AudioFifo fifo { 1, 32 };
    tracktion_engine::MidiMessageArray midi;
};

}
