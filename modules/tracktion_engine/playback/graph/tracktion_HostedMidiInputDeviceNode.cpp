/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/

namespace tracktion { inline namespace engine
{

HostedMidiInputDeviceNode::HostedMidiInputDeviceNode (InputDeviceInstance& idi, MidiInputDevice&, MidiMessageArray::MPESourceID msi,
                                                      tracktion::graph::PlayHeadState&)
    : instance (idi),
      midiSourceID (msi)
{
}

HostedMidiInputDeviceNode::~HostedMidiInputDeviceNode()
{
    instance.removeConsumer (this);
}

tracktion::graph::NodeProperties HostedMidiInputDeviceNode::getNodeProperties()
{
    tracktion::graph::NodeProperties props;
    props.hasMidi = true;
    return props;
}

void HostedMidiInputDeviceNode::prepareToPlay (const tracktion::graph::PlaybackInitialisationInfo& info)
{
    sampleRate = info.sampleRate;

    instance.addConsumer (this);
}

bool HostedMidiInputDeviceNode::isReadyToProcess()
{
    return true;
}

void HostedMidiInputDeviceNode::process (ProcessContext& pc)
{
    SCOPED_REALTIME_CHECK

    const auto localTimeRange = tracktion::graph::sampleToTime (pc.referenceSampleRange, sampleRate).withStart (0.0);
    auto& destMidi = pc.buffers.midi;

    const std::lock_guard<tracktion::graph::RealTimeSpinLock> lock (bufferMutex);

    for (auto m : incomingMessages)
        if (localTimeRange.contains (m.getTimeStamp()))
            destMidi.add (m);
    
    // Subtract time from messages and trim any negative
    incomingMessages.addToTimestamps (-localTimeRange.getLength());
    
    for (int i = incomingMessages.size(); --i >= 0;)
        if (incomingMessages[i].getTimeStamp() < 0.0)
            incomingMessages.remove (i);
}

void HostedMidiInputDeviceNode::handleIncomingMidiMessage (const juce::MidiMessage& message)
{
    // Timestamps will be offsets form the next buffer in seconds
    const std::lock_guard<tracktion::graph::RealTimeSpinLock> lock (bufferMutex);
    incomingMessages.addMidiMessage (message, midiSourceID);
}

}} // namespace tracktion { inline namespace engine