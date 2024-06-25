#define _USE_MATH_DEFINES
#include <math.h>
#include "GmpiApiEditor.h"
#include "RefCountMacros.h"
#include "Common.h"
#include "Drawing.h"
#include "helpers/GraphicsRedrawClient.h"


namespace gmpi
{

class PluginEditor : public gmpi::api::IEditor, public gmpi::api::IDrawingClient, public gmpi::api::IInputClient
{
protected:
	gmpi::drawing::Rect bounds;

	gmpi::shared_ptr<gmpi::api::IInputHost> inputHost;
	gmpi::shared_ptr<gmpi::api::IEditorHost> editorHost;
	gmpi::shared_ptr<gmpi::api::IDrawingHost> drawingHost;

public:

	PluginEditor()
	{
	}

	// IEditor
	ReturnCode setHost(gmpi::api::IUnknown* phost) override
	{
		gmpi::shared_ptr<gmpi::api::IUnknown> unknown(phost);

		phost->queryInterface(&gmpi::api::IDrawingHost::guid, drawingHost.asIMpUnknownPtr());
		inputHost = unknown.As<gmpi::api::IInputHost>();
		editorHost = unknown.As<gmpi::api::IEditorHost>();

		return ReturnCode::Ok;
	}

	ReturnCode initialize() override
	{
		return ReturnCode::Ok;
	}

	ReturnCode setPin(int32_t pinId, int32_t voice, int32_t size, const void* data) override
	{
		return ReturnCode::Ok;
	}

	ReturnCode notifyPin(int32_t pinId, int32_t voice) override
	{
		return ReturnCode::Ok;
	}

	ReturnCode measure(const gmpi::drawing::Size* availableSize, gmpi::drawing::Size* returnDesiredSize) override
	{
		*returnDesiredSize = *availableSize;
		return ReturnCode::Ok;
	}

	ReturnCode arrange(const gmpi::drawing::Rect* finalRect) override
	{
		bounds = *finalRect;
		return ReturnCode::Ok;
	}

	// IDrawingClient
	ReturnCode open(gmpi::api::IUnknown* host) override
	{
		return ReturnCode::Ok;
	}

	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		return ReturnCode::Ok;
	}

	ReturnCode getClipArea(drawing::Rect* returnRect) override
	{
		*returnRect = bounds;
		return ReturnCode::Ok;
	}

	gmpi::ReturnCode onMouseWheel(gmpi::drawing::Point point, int32_t flags, int32_t delta) override
	{
		return ReturnCode::Ok;
	}
	gmpi::ReturnCode setHover(bool isMouseOverMe) override
	{
		return ReturnCode::Ok;
	}

	// IInputClient
	gmpi::ReturnCode onPointerDown(gmpi::drawing::Point point, int32_t flags) override
	{
		return ReturnCode::Unhandled;
	}
	gmpi::ReturnCode onPointerMove(gmpi::drawing::Point point, int32_t flags) override
	{
		return ReturnCode::Unhandled;
	}
	gmpi::ReturnCode onPointerUp(gmpi::drawing::Point point, int32_t flags) override
	{
		return ReturnCode::Unhandled;
	}

	// IUnknown
	ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
	{
		GMPI_QUERYINTERFACE(gmpi::api::IEditor);
		GMPI_QUERYINTERFACE(gmpi::api::IInputClient);
		GMPI_QUERYINTERFACE(gmpi::api::IDrawingClient);
		return ReturnCode::NoSupport;
	}
	GMPI_REFCOUNT;
};

} // namespace gmpi

using namespace gmpi;
using namespace gmpi::drawing;

void drawKnob(Graphics& g, const Rect& bounds, float pinGain)
{
	auto r = bounds;

	auto center = Point{ (r.left + r.right) * 0.5f, (r.top + r.bottom) * 0.5f };
	auto radius = (std::min)(r.right, r.bottom) * 0.4f;
	auto thickness = radius * 0.2f;

	auto brush = g.createSolidColorBrush(Colors::Chocolate);

	// inner Gray circle
	auto dimBrush = g.createSolidColorBrush(Colors::LightSlateGray);

	const float startAngle = 35.0f; // angle between "straight-down" and start of arc. In degrees.
	const float startAngleRadians = startAngle * M_PI / 180.f; // angle between "straight-down" and start of arc. In degrees.
	const float quarterTurnClockwise = M_PI * 0.5f;

	Point startPoint{ center.x + radius * cosf(quarterTurnClockwise + startAngleRadians), center.y + radius * sinf(quarterTurnClockwise + startAngleRadians) };
	StrokeStyleProperties strokeStyleProperties;
	strokeStyleProperties.lineCap = CapStyle::Round;
	auto strokeStyle = g.getFactory().createStrokeStyle(strokeStyleProperties);

	// Background gray arc.
	{
		float sweepAngle = (M_PI * 2.0f - startAngleRadians * 2.0f);

		Point endPoint{ center.x + radius * cosf(quarterTurnClockwise + startAngleRadians + sweepAngle), center.y + radius * sinf(quarterTurnClockwise + startAngleRadians + sweepAngle) };

		auto arcGeometry = g.getFactory().createPathGeometry();
		auto sink = arcGeometry.open();
		sink.beginFigure(startPoint);
		sink.addArc(ArcSegment{ endPoint, Size{ radius, radius }, 0.0f, SweepDirection::Clockwise, sweepAngle > M_PI ? ArcSize::Large : ArcSize::Small });
		sink.endFigure(FigureEnd::Open);
		sink.close();

		g.drawGeometry(arcGeometry, dimBrush, thickness, strokeStyle);
	}

	// foreground coloured arc
	{
		float nomalised = pinGain;
		float sweepAngle = nomalised * (static_cast<float>(M_PI) * 2.0f - startAngleRadians * 2.0f);

		Point endPoint{ center.x + radius * cosf(quarterTurnClockwise + startAngleRadians + sweepAngle), center.y + radius * sinf(quarterTurnClockwise + startAngleRadians + sweepAngle) };

		auto arcGeometry = g.getFactory().createPathGeometry();
		auto sink = arcGeometry.open();
		sink.beginFigure(startPoint);
		sink.addArc(ArcSegment{ endPoint, Size{radius, radius}, 0.0f, SweepDirection::Clockwise, sweepAngle > M_PI ? ArcSize::Large : ArcSize::Small });
		sink.endFigure(FigureEnd::Open);
		sink.close();

		g.drawGeometry(arcGeometry, brush, thickness, strokeStyle);
	}
}

class AGraphicsGui final : public PluginEditor
{
	float pinGain = 0.0f;
	
public:
	AGraphicsGui() {}

	ReturnCode setPin(int32_t pinId, int32_t voice, int32_t size, const void* data) override
	{
		if (pinId == 0 && size == sizeof(pinGain))
		{
			pinGain = *(const float*) data;

			if(drawingHost)
				drawingHost->invalidateRect(nullptr);
		}
		return ReturnCode::Ok;
	}

	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

		ClipDrawingToBounds _(g, bounds);

		g.clear(Colors::YellowGreen);

		drawKnob(g, bounds, pinGain);

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
			editorHost->setPin(0, 0, sizeof(pinGain), &pinGain);

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