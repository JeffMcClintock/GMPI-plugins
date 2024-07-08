#include "helpers/GmpiPluginEditor.h"
#include "drawKnob.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

class AGraphicsGui final : public PluginEditor
{
	Pin<float> pinGain;
	Point lastMouse{};

public:
	AGraphicsGui()
	{
		pinGain.onUpdate = [this](PinBase*)
		{
			drawingHost->invalidateRect(nullptr);
		};

		init(pinGain);
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
		lastMouse = point;
		return inputHost->setCapture();
	}

	ReturnCode onPointerMove(Point point, int32_t flags) override
	{
		bool isCaptured = false;
		inputHost->getCapture(isCaptured);

		if (isCaptured)
		{
			pinGain = std::clamp(pinGain.value + (lastMouse.y - point.y) * 0.02f, 0.0f, 1.0f);
			drawingHost->invalidateRect(nullptr);
		}
		lastMouse = point;
		return ReturnCode::Ok;
	}

	ReturnCode onPointerUp(Point point, int32_t flags) override
	{
		return inputHost->releaseCapture();
	}

	gmpi::ReturnCode OnKeyPress(wchar_t c) override
	{
		_RPTWN(0, L"key %c\n", c);
		return ReturnCode::Unhandled;
	}
};

namespace
{
	auto r = Register<AGraphicsGui>::withId("GMPI: GainGui");
}
