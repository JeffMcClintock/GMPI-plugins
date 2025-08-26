#include <algorithm>
#include "GmpiMidi.h"
#include "SawDemo.h"

using namespace gmpi;
using namespace sst::clap_saw_demo;

float scaleTimeParamToSeconds(float param)
{
    auto scaleTime = std::clamp((param - 2.0 / 3.0) * 6, -100.0, 2.0);
    return powf(2.f, scaleTime);
}

void SawDemo::onMidiMessage(int pin, const uint8_t *midiMessage, int size)
{
    // Parse MIDI 2.0
    gmpi::midi::message_view msg(midiMessage, size);
    const auto header = gmpi::midi_2_0::decodeHeader(msg);

    switch (header.status)
    {
    case gmpi::midi_2_0::NoteOn:
    {
        const auto note = gmpi::midi_2_0::decodeNote(msg);
        handleNoteOn(0, header.channel, note.noteNumber, -1);
        pinLeft.setStreaming(true);
        pinRight.setStreaming(true);
    }
    break;

    case gmpi::midi_2_0::NoteOff:
    {
        const auto note = gmpi::midi_2_0::decodeNote(msg);
        handleNoteOff(0, header.channel, note.noteNumber);
    }
    break;
    }
}

void SawDemo::subProcess(int frames_count)
{
    float *out[2] = {getBuffer(pinLeft), getBuffer(pinRight)};
    const auto chans = std::size(out);

    for (int i = 0; i < frames_count; ++i)
    {
        // NOTE: sample-accurate event handling is handled by the GMPI framework. No boilerplate needed here.

        // This is a simple accumulator of output across our active voices.
        // See saw-voice.h for information on the individual voice.
        for (int ch = 0; ch < chans; ++ch)
        {
            out[ch][i] = 0.f;
        }
        for (auto &v : voices)
        {
            if (v.isPlaying())
            {
                v.step();
                if (chans >= 2)
                {
                    out[0][i] += v.L;
                    out[1][i] += v.R;
                }
                else if (chans == 1)
                {
                    out[0][i] += (v.L + v.R) * 0.5;
                }
            }
        }
    }

    for (auto &v : voices)
    {
        if (v.state == SawDemoVoice::NEWLY_OFF)
        {
            v.state = SawDemoVoice::OFF;
        }
    }

    // A little optimization - if we have any active voices continue
    bool voicesActive{};
    for (const auto &v : voices)
    {
        if (v.state != SawDemoVoice::OFF)
        {
            voicesActive = true;
            break;
        }
    }

    // Otherwise we have no voices - we can sleep
    // And our host can optionally skip processing
    if (voicesActive != pinLeft.isStreaming())
    {
        pinLeft.setStreaming(voicesActive, frames_count - 1);
        pinRight.setStreaming(voicesActive, frames_count - 1);
    }
}

void SawDemo::onSetPins()
{
    pushParamsToVoices();

    // Set state of output audio pins.
    pinLeft.setStreaming(true);
    pinRight.setStreaming(true);
}

void SawDemo::pushParamsToVoices()
{
    for (auto &v : voices)
    {
        if (v.isPlaying())
        {
            // !!!TODO Normalised values to real
            v.uniSpread = unisonSpread;
            v.oscDetune = oscDetune;
            v.cutoff = cutoff;
            v.res = resonance;
            v.preFilterVCA = preFilterVCA;
            v.ampRelease = scaleTimeParamToSeconds(ampRelease);
            v.ampAttack = scaleTimeParamToSeconds(ampAttack);
            v.ampGate = ampIsGate > 0.5;
            v.filterMode = filterMode;

            v.recalcPitch();
            v.recalcFilter();
        }
    }
}

void SawDemo::activateVoice(SawDemoVoice &v, int port_index, int channel, int key, int noteid)
{
    v.unison = std::max(1, std::min(7, (int)unisonCount));
    v.filterMode = filterMode;
    v.note_id = noteid;
    v.portid = port_index;
    v.channel = channel;

    v.uniSpread = unisonSpread;
    v.oscDetune = oscDetune;
    v.cutoff = cutoff;
    v.res = resonance;
    v.preFilterVCA = preFilterVCA;
    v.ampRelease = scaleTimeParamToSeconds(ampRelease);
    v.ampAttack = scaleTimeParamToSeconds(ampAttack);
    v.ampGate = ampIsGate > 0.5;

    // reset all the modulations
    v.cutoffMod = 0;
    v.oscDetuneMod = 0;
    v.resMod = 0;
    v.preFilterVCAMod = 0;
    v.uniSpreadMod = 0;
    v.volumeNoteExpressionValue = 0;
    v.pitchNoteExpressionValue = 0;

    v.start(key);
}

/*
 * The note on, note off, and push params to voices implementations are, basically,
 * completely uninteresting.
 */
void SawDemo::handleNoteOn(int port_index, int channel, int key, int noteid)
{
    bool foundVoice{false};
    for (auto &v : voices)
    {
        if (v.state == SawDemoVoice::OFF)
        {
            activateVoice(v, port_index, channel, key, noteid);
            foundVoice = true;
            break;
        }
    }

    if (!foundVoice)
    {
        // We could steal oldest. If you want to do that toss in a PR to add age
        // to the voice I guess. This is just a demo synth though.
        auto idx = rand() % max_voices;
        auto &v = voices[idx];

        activateVoice(v, port_index, channel, key, noteid);
    }
}

void SawDemo::handleNoteOff(int port_index, int channel, int n)
{
    for (auto &v : voices)
    {
        if (v.isPlaying() && v.key == n && v.portid == port_index && v.channel == channel)
        {
            v.release();
        }
    }
}

namespace
{
auto r = Register<SawDemo>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<PluginList>
    <Plugin id="GMPI: SawDemo" name="SawDemo" category="">
        <!--
            * Parameter Handling:
            *
            * metadata is provided in the XML here
            * 
        -->
        <Parameters>
            <Parameter id="0" name="Unison Count" datatype="int" default="3" min="1" max="7"/>
            <Parameter id="1" name="Unison Spread in Cents" datatype="float" default="10.0" max="100.0"/>
            <Parameter id="2" name="Oscillator Detuning (in cents)" datatype="float" min="-200.0" max="200.0"/>
            <Parameter id="3" name="Amplitude Attack (s)" datatype="float" default=".01" max="1"/>
            <Parameter id="4" name="Amplitude Release (s)" datatype="float" default="0.2" max="1"/>
            <Parameter id="5" name="Deactivate Amp Envelope Generator (Gate)" datatype="bool"/>
            <Parameter id="6" name="Pre Filter VCA Volume (modulation)" datatype="float" default="1.0" max="1.0"/>
            <Parameter id="7" name="Cutoff in Keys" datatype="float" default="69.0" min="1.0" max="127.0"/>
            <Parameter id="8" name="Resonance" datatype="float" default="0.7" max="1.0"/>
            <Parameter id="9" name="Filter Type" datatype="enum" metadata="LP,HP,BP,NOTCH,PEAK,ALL"/>
        </Parameters>

        <Audio>
            <!--
             * Many plugins will want input and output audio and MIDI ports, although
             * the spec doesn't require this. Here as a simple synth we set up a left
             * and right outputs and a single midi input.
             -->
            <Pin name="MIDI"  datatype="midi"/>
            <Pin name="Left"  datatype="float" rate="audio" direction="out"/>
            <Pin name="Right" datatype="float" rate="audio" direction="out"/>

            <!--
             * below is where we specify which parameters are hooked up to the Processor
             -->

            <Pin parameterId="0" />
            <Pin parameterId="1" />
            <Pin parameterId="2" />
            <Pin parameterId="3" />
            <Pin parameterId="4" />
            <Pin parameterId="5" />
            <Pin parameterId="6" />
            <Pin parameterId="7" />
            <Pin parameterId="8" />
            <Pin parameterId="9" />
        </Audio>

        <GUI>
            <!--
                * below is where we specify which parameters are hooked up to the GUI
            -->
            <Pin parameterId="0" />
            <Pin parameterId="1" parameterField="Normalized"/>
            <Pin parameterId="2" parameterField="Normalized"/>
            <Pin parameterId="3" parameterField="Normalized"/>
            <Pin parameterId="4" parameterField="Normalized"/>
            <Pin parameterId="5" parameterField="Normalized"/>
            <Pin parameterId="6" parameterField="Normalized"/>
            <Pin parameterId="7" parameterField="Normalized"/>
            <Pin parameterId="8" parameterField="Normalized"/>
            <Pin parameterId="9" parameterField="Normalized"/>

            <!-- Here are the additional information provided by the DAW, tempo etc -->
			<Pin name = "Host BPM" datatype = "float" hostConnect = "Time/BPM" />
			<Pin name = "Host SP" datatype = "float" hostConnect = "Time/SongPosition" />
			<Pin name = "Numerator" datatype = "int" hostConnect = "Time/Timesignature/Numerator" />
			<Pin name = "Denominator" datatype = "int" hostConnect = "Time/Timesignature/Denominator" />
			<Pin name = "BYPASS" datatype = "bool" />
        </GUI>
    </Plugin>
</PluginList>
)XML");
}
