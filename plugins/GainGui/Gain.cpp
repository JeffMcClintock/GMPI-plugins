#include "Processor.h"

using namespace gmpi;

struct GainGui final : public Processor
{
	static constexpr size_t numChannels = 2;

	AudioInPin pinInputs[numChannels];
	AudioOutPin pinOutputs[numChannels];

	FloatInPin pinGain;

	GainGui()
	{
		init(pinInputs[0]);
		init(pinInputs[1]);
		init(pinOutputs[0]);
		init(pinOutputs[1]);
		init(pinGain);
	}

	ReturnCode open(IUnknown* phost) override
	{
		// specify which member to process audio.
		setSubProcess(&GainGui::subProcess);

		return Processor::open(phost);
	}

	void subProcess(int sampleFrames)
	{
		// get pointers to in/output buffers.
		const float* in[numChannels]{ getBuffer(pinInputs[0]),  getBuffer(pinInputs[1]) };
		float*      out[numChannels]{ getBuffer(pinOutputs[0]), getBuffer(pinOutputs[1])};

		// get parameter value.
		const float gain = pinGain;

		// Apply audio processing.
		for (int chan = 0; chan < numChannels; ++chan)
		{
			for (int i = 0; i < sampleFrames; ++i)
			{
				out[chan][i] = gain * in[chan][i];
			}
		}
	}
};

// Describe the plugin and register it with the framework.
namespace
{
auto r = Register<GainGui>::withXml(R"XML(
<?xml version="1.0" encoding="utf-8" ?>

<PluginList>
  <Plugin id="GMPI: GainGui" name="Gain" category="GMPI/SDK Examples" vendor="Jeff McClintock" helpUrl="Gain.htm">
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
</PluginList>
)XML");
}
