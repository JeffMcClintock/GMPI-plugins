#include "Processor.h"

using namespace gmpi;

struct Gain final : public Processor
{
	AudioInPin pinInput;
	AudioOutPin pinOutput;
	FloatInPin pinGain;

	Gain()
	{
		setSubProcess(&Gain::subProcess); // specify the member function to process audio.
	}

	void subProcess(int sampleFrames)
	{
		// get pointers to in/output buffers.
		auto input = getBuffer(pinInput);
		auto output = getBuffer(pinOutput);

		// get parameter value.
		const float gain = pinGain;

		// Apply audio processing.
		for(int i = 0 ; i < sampleFrames ; ++i)
		{
			output[i] = gain * input[i];
		}
	}
};

namespace
{
auto r = Register<Gain>::withXml(R"XML(
<?xml version="1.0" encoding="utf-8" ?>

<PluginList>
  <Plugin id="GMPI Gain" name="Gain" category="GMPI/SDK Examples" vendor="Jeff McClintock" helpUrl="Gain.htm">
    <Parameters>
      <Parameter id="0" name="Gain" datatype="float" default="0.8"/>
    </Parameters>
    <Audio>
      <Pin name="Input" datatype="float" rate="audio" />
      <Pin name="Output" datatype="float" rate="audio" direction="out" />
      <Pin parameterId="0" />
    </Audio>
  </Plugin>
</PluginList>
)XML");
}
