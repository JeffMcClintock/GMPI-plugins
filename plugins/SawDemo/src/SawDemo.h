/*
 * (originally) ClapSawDemo
 * https://github.com/surge-synthesizer/clap-saw-demo
 *
 * Copyright 2022 Paul Walker and others as listed in the git history
 *
 * Released under the MIT License. See LICENSE.md for full text.
 */

#include <array>
#include "Processor.h"
#include "saw-voice.h"

struct SawDemo final : public gmpi::Processor
{
    static constexpr int max_voices = 64;

    SawDemo() = default;
    ~SawDemo() override = default;

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
        setSubProcess(&SawDemo::subProcess);

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
    void subProcess(int frames_count);

    void pushParamsToVoices();
    void handleNoteOn(int port_index, int channel, int key, int noteid);
    void handleNoteOff(int port_index, int channel, int key);
    void activateVoice(sst::clap_saw_demo::SawDemoVoice &v, int port_index, int channel, int key, int noteid);
    /*
     * CLAP plugins should implement ::paramsFlush. to update the UI when processing isn't active.
     * GMPI dosen't require this. Parameter updates are communicated to the UI regardless of if the processor is sleeping or not.
     */

    void onMidiMessage(int pin, const uint8_t *midiMessage, int size) override;
    void onSetPins() override;

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

    // "Voice Management" is "randomly pick a voice to kill and put it in stolen voices"
    std::array<sst::clap_saw_demo::SawDemoVoice, max_voices> voices;
};
