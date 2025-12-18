/*
 * ClapSawDemo is Free and Open Source released under the MIT license
 *
 * Copright (c) 2021, Paul Walker
 */

#include "gmpi-saw-demo.h"
#include "GmpiMidi.h"
#include <algorithm>

using namespace gmpi;

namespace sst::clap_saw_demo
{

/*
 * PARAMETER SETUP SECTION
 */
namespace
{
auto r = Register<GmpiSawDemo>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<PluginList>
    <Plugin id="GMPI: GmpiSawDemo" name="SawDemo" category="">
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
            <Parameter id="10" name="Poly Count" datatype="int" private="true"/>
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

            <Pin parameterId="0" />                           <!-- Unison Count   -->
            <Pin parameterId="1" />                           <!-- Unison Spread  -->
            <Pin parameterId="2" />                           <!-- Detuning       -->
            <Pin parameterId="3" />                           <!-- Attack         -->
            <Pin parameterId="4" />                           <!-- Release        -->
            <Pin parameterId="5" />                           <!-- Deactivate Amp -->
            <Pin parameterId="6" />                           <!-- Pre Filter VCA -->
            <Pin parameterId="7" />                           <!-- Cutoff         -->
            <Pin parameterId="8" />                           <!-- Resonance      -->
            <Pin parameterId="9" />                           <!-- Filter Type    -->
            <Pin parameterId="10" direction="out" />          <!-- Poly Count     -->
			<Pin hostConnect="Process/Bypass" />
        </Audio>

        <GUI>
            <!--
                * below is where we specify which parameters are hooked up to the GUI
            -->
            <Pin parameterId="0" parameterField="Normalized"/> <!-- Unison Count   -->
            <Pin parameterId="1" parameterField="Normalized"/> <!-- Unison Spread  -->
            <Pin parameterId="2" parameterField="Normalized"/> <!-- Detuning       -->
            <Pin parameterId="3" parameterField="Normalized"/> <!-- Attack         -->
            <Pin parameterId="4" parameterField="Normalized"/> <!-- Release        -->
            <Pin parameterId="5" parameterField="Normalized"/> <!-- Deactivate Amp -->
            <Pin parameterId="6" parameterField="Normalized"/> <!-- Pre Filter VCA -->
            <Pin parameterId="7" parameterField="Normalized"/> <!-- Cutoff         -->
            <Pin parameterId="8" parameterField="Normalized"/> <!-- Resonance      -->
            <Pin parameterId="9" parameterField="Normalized"/> <!-- Filter Type    -->
            <Pin parameterId="10" /> 					       <!-- Poly Count     -->

            <!-- Here are the additional information provided by the DAW, tempo etc -->
			<Pin hostConnect="Time/BPM" />
			<Pin hostConnect="Time/SongPosition" />
			<Pin hostConnect="Time/Numerator" />
			<Pin hostConnect="Time/Denominator" />
			<Pin hostConnect="Process/Bypass" />
        </GUI>
    </Plugin>
</PluginList>
)XML");
}

/*
 * The process function is the heart of any plugin. It reads inbound events,
 * generates audio if appropriate, writes outbound events, and informs the host
 * to continue operating.
 *
 * In the SawDemo, the gmpi::Processor process loop has 3 basic stages
 *
 * 1. See if the UI has sent us any events on the thread-safe UI Queue
 *    , and apply them to my internal state by calling the onSetPins() member function.
 *
 * 2. See if the DAW has sent us any MIDI events on the thread-safe event list
 *    and apply them by calling the onMidiMessage() member function.
 *    
 * 3. Iterate over samples rendering the voices by calling my subProcess member function, and if an inbound event is coincident
 *    with a sample, process that event for note on, parameter automation, and so on before calling subProcess for the remaining samples.
 */

void GmpiSawDemo::subProcess(int frames_count)
{
    float *out[2] = {getBuffer(pinLeft), getBuffer(pinRight)};
    const auto chans = std::size(out);

    for (int i = 0; i < frames_count; ++i)
    {
        // NOTE: sample-accurate event handling is handled by the gmpi::Processor::process() member function, No boilerplate needed here.

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

/*
 * onSetPins provides the core event mechanism including
 * voice activation and deactivation, parameter modulation, note expression,
 * messages from the GUI and so on.
 */
void GmpiSawDemo::onSetPins()
{
    pushParamsToVoices();

    // Set state of output audio pins.
    pinLeft.setStreaming(true);
    pinRight.setStreaming(true);
}

void GmpiSawDemo::onMidiMessage(int pin, std::span<const uint8_t> midiMessage)
{
    // Parse MIDI 2.0
    gmpi::midi2::message_view msg(midiMessage);
    const auto header = gmpi::midi2::decodeHeader(msg);

    switch (header.status)
    {
    case gmpi::midi2::NoteOn:
    {
        if (!isBypassed)
        {
            const auto note = gmpi::midi2::decodeNote(msg);
            handleNoteOn(header.group, header.channel, note.noteNumber, -1);

            polyCount = polyCount + 1;

            pinLeft.setStreaming(true);
            pinRight.setStreaming(true);
        }
    }
    break;

    case gmpi::midi2::NoteOff:
    {
        const auto note = gmpi::midi2::decodeNote(msg);
        handleNoteOff(header.group, header.channel, note.noteNumber);

        polyCount = polyCount - 1;
    }
    break;

    case gmpi::midi2::PitchBend:
    {
        const auto normalized = gmpi::midi2::decodeController(msg).value;
        const auto pitchBend = (normalized - 0.5f) * 2.0f;

        for (auto& v : voices)
        {
            v.pitchBendWheel = pitchBend * 2; // just hardcode a pitch bend depth of 2
            v.recalcPitch();
        }
    }

	// Note-expression and Polyphonic modulation via per-note-controller messages.
    case gmpi::midi2::PolyAfterTouch:
    {
        const auto aftertouch = gmpi::midi2::decodePolyController(msg);

        if (auto v = getVoice(header.group, header.channel, aftertouch.noteNumber); v)
        {
            // Aftertouch to Filter resonance
            v->resMod = aftertouch.value;
            v->recalcFilter();
        }
    }
    break;

    case gmpi::midi2::PolyControlChange:
    {
        const auto polyController = gmpi::midi2::decodePolyController(msg);
        auto v = getVoice(header.group, header.channel, polyController.noteNumber);

        if(!v)
			break;

        if (polyController.type == gmpi::midi2::PolyPitch)
        {
            // Polyphonic pitch modulation
            const auto semitones = gmpi::midi2::decodeNotePitch(msg);
            v->pitchNoteExpressionValue = semitones;
//			_RPTN(0, "NoteExpression pitch %d %f\n", polyController.noteNumber, semitones);
            v->recalcPitch();
        }
        else
        {
            switch (polyController.type)
            {
            // Volume modulation
            case gmpi::midi2::PolyVolume:
                v->volumeNoteExpressionValue = polyController.value;
                break;

            // Brightness to Filter cutoff
            case gmpi::midi2::PolySoundController5:
                v->cutoffMod = 100.0f * polyController.value;
                v->recalcFilter();
                break;
            }
        }
    }
    break;
    }
}

/*
 * The note on, note off, and push params to voices implementations are, basically, completely
 * uninteresting.
 */
void GmpiSawDemo::handleNoteOn(int port_index, int channel, int key, int noteid)
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

void GmpiSawDemo::handleNoteOff(int port_index, int channel, int n)
{
    for (auto &v : voices)
    {
        if (v.isPlaying() && v.key == n && v.portid == port_index && v.channel == channel)
        {
            v.release();
        }
    }
}

void GmpiSawDemo::activateVoice(SawDemoVoice &v, int port_index, int channel, int key, int noteid)
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

SawDemoVoice* GmpiSawDemo::getVoice(int port_index, int channel, int key)
{
    for (auto& v : voices)
    {
        if (v.isPlaying() && v.key == key && v.portid == port_index && v.channel == channel)
        {
            return &v;
        }
    }
    return {};
}

void GmpiSawDemo::pushParamsToVoices()
{
    for (auto &v : voices)
    {
        if (v.isPlaying())
        {
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

float GmpiSawDemo::scaleTimeParamToSeconds(float param)
{
    auto scaleTime = std::clamp((param - 2.0 / 3.0) * 6, -100.0, 2.0);
    auto res = powf(2.f, scaleTime);
    return res;
}

} // namespace sst::clap_saw_demo
