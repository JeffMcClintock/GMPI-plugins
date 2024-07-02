#include "helpers/GmpiPluginEditor.h"
#include "drawKnob.h"

using namespace gmpi;

class AGraphicsGui final : public gmpi::PluginEditor
{
	Pin<float> pinGain;
	
public:
	AGraphicsGui()
	{
		init(pinGain);

		pinGain.onUpdate = [this](PinBase*)
		{
			drawingHost->invalidateRect(nullptr);
		};
	}

	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

		ClipDrawingToBounds _(g, bounds);

		g.clear(Colors::YellowGreen);

		drawKnob(g, bounds, pinGain.value);

		return ReturnCode::Ok;
	}

	gmpi::ReturnCode onPointerDown(gmpi::drawing::Point point, int32_t flags) override
	{
		return inputHost->setCapture();
	}

	gmpi::ReturnCode onPointerMove(gmpi::drawing::Point point, int32_t flags) override
	{
		bool isCaptured = false;
		inputHost->getCapture(isCaptured);

		if (isCaptured)
		{
			pinGain = std::clamp((point.x - bounds.left) / (bounds.right - bounds.left), 0.0f, 1.0f);
			drawingHost->invalidateRect(nullptr);
		}
		return ReturnCode::Ok;
	}

	gmpi::ReturnCode onPointerUp(gmpi::drawing::Point point, int32_t flags) override
	{
		return inputHost->releaseCapture();
	}
};

namespace
{
	auto r = Register<AGraphicsGui>::withId("GMPI: GainGui");
}
