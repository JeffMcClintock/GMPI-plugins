#include "helpers/GmpiPluginEditor.h"
#include "drawKnob.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

class GainGui final : public PluginEditor
{
	// provide a connection to the parameter
	Pin<float> pinGain;

	// track the mouse position for dragging.
	Point lastMouse{};

public:
	GainGui()
	{
		// respond to parameter updates by redrawing the editor
		pinGain.onUpdate = [this](PinBase*)
		{
			drawingHost->invalidateRect(nullptr);
		};
	}

	// draw the knob
	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

		ClipDrawingToBounds _(g, bounds);

		g.clear(Colors::YellowGreen);

		drawKnob(g, bounds, pinGain.value);

		return ReturnCode::Ok;
	}

	// handle mouse dragging
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
			// update the parameter
			pinGain = std::clamp(pinGain.value + (lastMouse.y - point.y) * 0.02f, 0.0f, 1.0f);

			// request redraw
			drawingHost->invalidateRect(nullptr);
		}
		lastMouse = point;
		return ReturnCode::Ok;
	}

	ReturnCode onPointerUp(Point point, int32_t flags) override
	{
		return inputHost->releaseCapture();
	}
};

// Register the Processor with the framework. see also Gain.cpp for the metadata (parameters and I/O description)
namespace
{
	auto r = Register<GainGui>::withId("GMPI: GainGui");
}
