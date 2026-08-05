/* Processor half of the drawing demo.
 *
 * The plugin exists for its editor (see DrawingDemoGui.cpp); the audio path is a
 * pass-through so it can be loaded in any VST3/CLAP host without doing anything
 * surprising to the signal. The XML below is what registers BOTH halves, so the
 * id here must match the Register<>::withId in the editor.
 */

#include "Processor.h"

using namespace gmpi;

struct DrawingDemo final : public Processor
{
    static constexpr size_t numChannels = 2;

    AudioInPin  pinInputs[numChannels];
    AudioOutPin pinOutputs[numChannels];

    DrawingDemo()
    {
        setSubProcess(&DrawingDemo::subProcess);
    }

    void subProcess(int sampleFrames)
    {
        for (size_t chan = 0; chan < numChannels; ++chan)
        {
            const float* in  = getBuffer(pinInputs[chan]);
            float*       out = getBuffer(pinOutputs[chan]);

            for (int i = 0; i < sampleFrames; ++i)
                out[i] = in[i];
        }
    }
};

namespace
{
auto r = Register<DrawingDemo>::withXml(R"XML(
<?xml version="1.0" encoding="utf-8" ?>

<Plugin id="GMPI: DrawingDemo" name="Drawing Demo" category="GMPI/SDK Examples" vendor="Jeff McClintock">
  <Parameters>
    <Parameter id="0" name="Page" datatype="int" default="0"/>
  </Parameters>

  <GUI graphicsApi="GmpiGui">
    <Pin name="Page" datatype="int" private="true" parameterId="0" />
  </GUI>

  <Audio>
    <Pin name="Input" datatype="float" rate="audio" />
    <Pin name="Input" datatype="float" rate="audio" />
    <Pin name="Output" datatype="float" rate="audio" direction="out" />
    <Pin name="Output" datatype="float" rate="audio" direction="out" />
  </Audio>

</Plugin>
)XML");
}
