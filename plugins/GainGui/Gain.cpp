#include "Processor.h"

using namespace gmpi;

struct Gain final : public Processor
{
	static constexpr size_t numChannels = 2;

	// provide connections to the audio I/O
	AudioInPin pinInputs[numChannels];
	AudioOutPin pinOutputs[numChannels];

	// provide a connection to the parameter
	FloatInPin pinGain;

	Gain()
	{
		// specify the member function to process audio.
		setSubProcess(&Gain::subProcess);
	}

	void subProcess(int sampleFrames)
	{
		// get pointers to in/output buffers.
		const float* in[]{ getBuffer(pinInputs[0]),  getBuffer(pinInputs[1]) };
		float*      out[]{ getBuffer(pinOutputs[0]), getBuffer(pinOutputs[1])};

		// get parameter value.
		const float gain = pinGain;

		// Apply audio processing.
		for (int chan = 0; chan < numChannels; ++chan)
			for (int i = 0; i < sampleFrames; ++i)
				out[chan][i] = gain * in[chan][i];

	}
};

// Describe the plugin and register it with the framework.
namespace
{
auto r = Register<Gain>::withXml(R"XML(
<?xml version="1.0" encoding="utf-8" ?>

<Plugin id="GMPI: GainGui" name="Gain GUI" category="GMPI/SDK Examples" vendor="Jeff McClintock" helpUrl="Gain.htm">
  <Parameters>
    <Parameter id="0" name="Gain" datatype="float" default="0.8"/>
  </Parameters>

  <GUI graphicsApi="GmpiGui">
    <Pin name="Input" private="true" parameterId="0" />
  </GUI>

  <Audio>
    <Pin name="Input" datatype="float" rate="audio" />
    <Pin name="Input" datatype="float" rate="audio" />
    <Pin name="Output" datatype="float" rate="audio" direction="out" />
    <Pin name="Output" datatype="float" rate="audio" direction="out" />
    <Pin parameterId="0" />
  </Audio>

</Plugin>
)XML");
}
