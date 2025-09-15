/*
 * (originally) ClapSawDemo
 * https://github.com/surge-synthesizer/clap-saw-demo
 *
 * Copyright 2022 Paul Walker and others as listed in the git history
 *
 * Released under the MIT License. See LICENSE.md for full text.
 */

#ifndef CLAP_SAW_DEMO_H
#define CLAP_SAW_DEMO_H
/*
 * GmpiSawDemo is the core synthesizer class. It uses the GMPI C++ projection
 * to present the GMPI C API as a C++ object model.
 *
 * The core features here are
 *
 * - Hold the plugin description
 * - Advertise parameters and ports
 * - Provide an event handler which responds to events and returns sound
 * - Do voice management. Which is really not very sophisticated (it's just an array of 64
 *   voice objects and we choose the next free one, and if you ask for a 65th voice, nothing
 *   happens).
 * - UI creation is handled by a separate editor object,
 *   coded in gmpi-saw-demo-editor
 *
 * This demo is coded to be relatively familiar and close to programming styles from other
 * formats where the editor and synth collaborate without undue dependancies; as described in gmpi-saw-demo-editor
 * the editor and synth communicate via the parameters; and the editor code is
 * cleanly separated from the Processor code.
 */
#include "Processor.h"
#include <array>
#include "saw-voice.h"

namespace sst::clap_saw_demo
{
struct GmpiSawDemo final : public gmpi::Processor
{
    static constexpr int max_voices = 64;

    GmpiSawDemo() = default;
    ~GmpiSawDemo() override = default;

    /*
     * open() makes sure sampleRate is distributed through
     * the data structures, in this case by stamping the sampleRate
     * onto each pre-allocated voice object.
     */
    gmpi::ReturnCode open(gmpi::api::IUnknown *phost) override
    {
        const auto r = Processor::open(phost);

        for (auto &v : voices)
            v.sampleRate = host->getSampleRate();

        // specify the member function to process audio.
        setSubProcess(&GmpiSawDemo::subProcess);

        return r;
    }

    /*
     * State dump and restore is handled automatically by the GMPI framework.
     * no code needed here.
     */

    /*
     * subProcess is the meat of the operation. It does obvious things like trigger
     * voices but also handles all the polyphonic modulation and so on. Please see the
     * comments in the cpp file to understand it and the helper functions we have
     * delegated to.
     */
     
    // Convert 0-1 linear into 0-4s exponential
    float scaleTimeParamToSeconds(float param);

    void subProcess(int frames_count);
    void onSetPins() override; // handle inbound parameter events
    void onMidiMessage(int pin, std::span<const uint8_t> midiMessage) override; // handle MIDI events
    void pushParamsToVoices();
    void handleNoteOn(int port_index, int channel, int key, int noteid);
    void handleNoteOff(int port_index, int channel, int key);
    void activateVoice(SawDemoVoice &v, int port_index, int channel, int key, int noteid);
    SawDemoVoice* getVoice(int port_index, int channel, int key);

    /*
     * CLAP plugins should implement ::paramsFlush. to update the UI when processing isn't active.
     * GMPI dosen't require this. Parameter updates are communicated to the UI regardless of if the processor is sleeping or not.
     */

    // I/O Pins
    gmpi::MidiInPin pinMIDI;
    gmpi::AudioOutPin pinLeft;
    gmpi::AudioOutPin pinRight;

    // Parameters
    // These items are ONLY read and written on the audio thread, so they
    // are safe to be non-atomic. The GMPI framework keeps a map to locate them
    // for parameter updates.
    gmpi::IntInPin unisonCount;
    gmpi::FloatInPin unisonSpread;
    gmpi::FloatInPin oscDetune;
    gmpi::FloatInPin ampAttack;
    gmpi::FloatInPin ampRelease;
    gmpi::BoolInPin ampIsGate;
    gmpi::FloatInPin preFilterVCA;
    gmpi::FloatInPin cutoff;
    gmpi::FloatInPin resonance;
    gmpi::IntInPin filterMode;
    gmpi::IntOutPin polyCount;
    gmpi::BoolInPin isBypassed;

    // "Voice Management" is "randomly pick a voice to kill and put it in stolen voices"
    std::array<SawDemoVoice, max_voices> voices;
};
} // namespace sst::clap_saw_demo

#endif