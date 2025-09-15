/* Copyright (c) 2007-2025 SynthEdit Ltd

Permission to use, copy, modify, and /or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS.IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#define _USE_MATH_DEFINES
#include <math.h>
#include "Processor.h"
#include "real_fft.h"

using namespace gmpi;

struct FreqAnalyser final : public Processor
{
	IntInPin pinFFTSize;
	IntInPin pinUpdateRate;
	BlobOutPin pinCaptureDataA;

	AudioInPin pinSignalinL;
	AudioInPin pinSignalinR;
	AudioOutPin pinSignaloutL;
	AudioOutPin pinSignaloutR;

	int index_ = 0;
	int timeoutCount_ = 0;
	int sleepCount = -1;
	std::vector<float> resultsA_;
	std::vector<float> resultsWindowed_;
	std::vector<float> window;
	int captureSamples = 0;

	FreqAnalyser() = default;

	gmpi::ReturnCode open(gmpi::api::IUnknown* host) override
	{
		auto r = gmpi::Processor::open(host);

		setSleep(false); // disable automatic sleeping.

		return r;
	}

	void subProcess( int sampleFrames )
	{
		// get pointers to in/output buffers.
		auto signalinL = getBuffer(pinSignalinL);
		auto signalinR = getBuffer(pinSignalinL);
		auto signaloutL = getBuffer(pinSignaloutL);
		auto signaloutR = getBuffer(pinSignaloutR);

		for( int s = sampleFrames; s > 0; --s )
		{
			// TODO: Signal processing goes here.
			*signaloutL = *signalinL;
			*signaloutR = *signalinR;

			assert(index_ < captureSamples);
			resultsA_[index_++] = *signalinL;

			if (index_ == captureSamples)
			{
				sendResultToGui(0);

				if (index_ >= captureSamples) // done
					return;
			}

			// Increment buffer pointers.
			++signalinL;
			++signalinR;
			++signaloutL;
			++signaloutR;
		}
	}

	// wait for waveform to restart.
	void waitAwhile(int sampleFrames)
	{
		timeoutCount_ -= sampleFrames;
		if (timeoutCount_ < 0)
		{
			index_ = 0;
			setSubProcess(&FreqAnalyser::subProcess);
		}
	}

	void sendResultToGui(int block_offset)
	{
		if (captureSamples < 100) // latency may mean this pin is initially zero for a short time.
			return;

		auto FFT_SIZE = captureSamples;

		// apply window
		for (int i = 0; i < FFT_SIZE; i++)
		{
			resultsWindowed_[i] = window[i] * resultsA_[i];
		}

		realft(resultsWindowed_.data() - 1, FFT_SIZE, 1);

		// calc power
		float nyquist = resultsWindowed_[1]; // DC & nyquist combined into 1st 2 entries

		// Always divide FFT results b n/2, also compensate for 50% loss in window function.
		const float inverseN = 4.0f / FFT_SIZE;
		for (int i = 1; i < FFT_SIZE / 2; i++)
		{
			const auto& a = resultsWindowed_[2 * i];
			const auto& b = resultsWindowed_[2 * i + 1];
			resultsWindowed_[i] = a * a + b * b; // remainder of dB calculation left to GUI.
		}

		//	resultsWindowed_[0] = fabs(resultsWindowed_[0]) * inverseN * 0.5f; // DC component is divided by 2
		resultsWindowed_[0] = fabsf(resultsWindowed_[0]) * 0.5f; // DC component is divided by 2
		resultsWindowed_[FFT_SIZE / 2] = fabsf(nyquist);

		// GUI does a square root on everything, compensate.
		resultsWindowed_[0] *= resultsWindowed_[0];
		resultsWindowed_[FFT_SIZE / 2] *= resultsWindowed_[FFT_SIZE / 2];

		// FFT produces half the original data.
		const int datasize = (1 + resultsWindowed_.size() / 2) * sizeof(resultsWindowed_[0]);

		// Add an extra member communicating sample-rate to GUI.
		// !! overwrites nyquist value?
		resultsWindowed_[resultsWindowed_.size() / 2] = host->getSampleRate();

		pinCaptureDataA.setValueRaw({ reinterpret_cast<uint8_t*>(resultsWindowed_.data()), static_cast<size_t>(datasize) });
		pinCaptureDataA.sendPinUpdate(block_offset);

		const int captureRateSamples = (int) host->getSampleRate() / pinUpdateRate.getValue();
		const int downtime = captureRateSamples - captureSamples;

		if (downtime >= 0)
		{
			timeoutCount_ = downtime;
			setSubProcess(&FreqAnalyser::waitAwhile);
		}
		else
		{
			const int retainSampleCount = -downtime;
			if (retainSampleCount < captureSamples)
			{
				// we need some overlap to maintain the frame rate
				std::copy(resultsA_.begin() + captureSamples - retainSampleCount, resultsA_.end(), resultsA_.begin());
				index_ = retainSampleCount;
			}
			else
			{
				index_ = 0;
			}
		}

		//timeoutCount_ = (std::max)(100, ((int)getSampleRate() - pinFFTSize.getValue()) / (std::max)(1, pinUpdateRate.getValue()));
		//setSubProcess(&FreqAnalyser3::waitAwhile);

		// if inputs aren't changing, we can sleep.
		if (sleepCount > 0)
		{
			if (--sleepCount == 0)
			{
				setSubProcess(&FreqAnalyser::subProcessNothing);
				setSleep(true);
			}
		}
	}

	void onSetPins() override
	{
		if( pinFFTSize.isUpdated() )
		{
			index_ = 0;
			captureSamples = pinFFTSize;
			resultsA_.assign(captureSamples, 0.0f);
			resultsWindowed_.assign(captureSamples, 0.0f);

			window.resize(captureSamples);

			// create window function
			for (int i = 0; i < captureSamples; i++)
			{
				// sin squared window
				float t = sin(i * ((float)M_PI) / captureSamples);
				window[i] = t * t;
			}
		}

		if( pinUpdateRate.isUpdated() )
		{
			const float sr = host->getSampleRate();

			Blob test;
			test.insert(test.begin(), (uint8_t*) &sr, (uint8_t*) &sr + sizeof(sr));
			pinCaptureDataA = test;
		}

		// Set processing method.
		if (pinSignalinL.isStreaming() || pinSignalinR.isStreaming())
		{
			sleepCount = -1; // indicates no sleep.
		}
		else
		{
			// need to send at least 2 captures to ensure flat-line signal captured.
			// Current capture may be half done.
			sleepCount = 2;
		}

		// Avoid resetting capture unless module is actually asleep.
		if (getSubProcess() == &FreqAnalyser::subProcessNothing)
		{
			index_ = 0;
			setSubProcess(&FreqAnalyser::subProcess);
		}
	}
};

namespace
{
auto r = Register<FreqAnalyser>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<PluginList>
    <Plugin id="GMPI: Freq Analyser" name="Freq Analyser" category="Controls">
        <Parameters>
            <Parameter id="0" datatype="blob" name="Capture Data A" private="true" ignorePatchChange="true" persistant="false"/>
        </Parameters>
        <Audio>
            <Pin name="FFT Size" datatype="enum" default="2048" isMinimised="true" metadata="1024=1024,2048=2048,4096=4096,8192=8192,16384=16384"/>
            <Pin name="Update Rate" datatype="enum" default="10" isMinimised="true" metadata="1 Hz=1,5 Hz=5,10 Hz=10,20 Hz=20,40 Hz=40,60 Hz=60"/>
            <Pin name="Capture Data A" datatype="blob" direction="out" private="true" parameterId="0"/>
            <Pin name="Signal L" datatype="float" rate="audio"/>
            <Pin name="Signal R" datatype="float" rate="audio"/>
            <Pin name="Signal L" direction="out" datatype="float" rate="audio"/>
            <Pin name="Signal R" direction="out" datatype="float" rate="audio"/>
        </Audio>
        <GUI graphicsApi="GmpiGui">
            <Pin name="Capture Data A" datatype="blob" private="true" parameterId="0"/>
            <Pin name="Mode" datatype="enum" isMinimised="true" metadata="Log,Linear"/>
            <Pin name="dB High" datatype="int" default="20" isMinimised="true"/>
            <Pin name="dB Low" datatype="int" default="-80" isMinimised="true"/>
        </GUI>
    </Plugin>
</PluginList>
)XML");
}
