#include "helpers/GmpiPluginEditor.h"
#include "drawKnob.h"

using namespace gmpi;
using namespace gmpi::drawing;

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

	ReturnCode onPointerDown(Point point, int32_t flags) override
	{
		return inputHost->setCapture();
	}

	ReturnCode onPointerMove(Point point, int32_t flags) override
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

	ReturnCode onPointerUp(Point point, int32_t flags) override
	{
		return inputHost->releaseCapture();
	}
};

namespace
{
	auto r = Register<AGraphicsGui>::withId("GMPI: GainGui");
}
