#define _USE_MATH_DEFINES
#include <math.h>
#include "GmpiApiEditor.h"
#include "GmpiApiDrawingClient.h"
#include "RefCountMacros.h"
#include "Common.h"
#include "Drawing.h"
// hack for now, to prevent linker from discarding plugin factory
#include "public.sdk/source/main/pluginfactory.h"
#include "helpers/GraphicsRedrawClient.h"

using namespace gmpi;
using namespace gmpi::drawing;

// TODO IDrawingClient and IGraphicsClient are essentially the same thing, merge them. Currently gimpi_ui expects IDrawingClient
class AGraphicsGui final : public gmpi::api::IEditor, public gmpi::api::IDrawingClient, public gmpi::api::IInputClient
{
	Rect bounds;
	float pinGain = 0.0f;
	gmpi::shared_ptr<gmpi::api::IDrawingHost> host;
	gmpi::shared_ptr<gmpi::api::IInputHost> inputhost;
	gmpi::shared_ptr<gmpi::api::IEditorHost> editorhost;
	
public:
	AGraphicsGui()
	{
#if 1
// hack for now, to prevent linker from discarding plugin factory
		auto test = GetPluginFactory();
#endif
	}

	// IEditor
	ReturnCode setHost(gmpi::api::IUnknown* phost) override
	{
		gmpi::shared_ptr<gmpi::api::IUnknown> unknown(phost);

		phost->queryInterface(&gmpi::api::IDrawingHost::guid, host.asIMpUnknownPtr());
		inputhost = unknown.As<gmpi::api::IInputHost>();
		editorhost = unknown.As<gmpi::api::IEditorHost>();

		return ReturnCode::Ok;
	}

	ReturnCode initialize() override
	{
		return ReturnCode::Ok;
	}

	ReturnCode setPin(int32_t pinId, int32_t voice, int32_t size, const void* data) override
	{
		if (pinId == 0 && size == sizeof(pinGain))
		{
			pinGain = *(const float*) data;

			if(host)
				host->invalidateRect(nullptr);
		}
		return ReturnCode::Ok;
	}

	ReturnCode notifyPin(int32_t pinId, int32_t voice) override
	{
		return ReturnCode::Ok;
	}

	// IDrawingClient
	ReturnCode open(gmpi::api::IUnknown* host) override
	{
		return ReturnCode::Ok;
	}
	ReturnCode measure(const gmpi::drawing::Size* availableSize, gmpi::drawing::Size* returnDesiredSize) override
//		ReturnCode measure(drawing::SizeU availableSize, drawing::SizeU* returnDesiredSize) override
	{
		*returnDesiredSize = *availableSize;
		return ReturnCode::Ok;
	}

	ReturnCode arrange(const gmpi::drawing::Rect* finalRect) override
//		ReturnCode arrange(drawing::RectL const* finalRect) override
	{
		bounds = *finalRect;
		//drawing::Rect{
		//	static_cast<float>(finalRect->left),
		//	static_cast<float>(finalRect->top),
		//	static_cast<float>(finalRect->right),
		//	static_cast<float>(finalRect->bottom)
		//};
		return ReturnCode::Ok;
	}

	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

		ClipDrawingToBounds _(g, bounds);

		g.clear(Colors::YellowGreen);

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

#if 0


		auto brush = g.createSolidColorBrush(Colors::Red);

		auto factory = g.getFactory();
		if (factory.get())
		{
			auto textFormat = g.getFactory().createTextFormat();

			g.drawTextU("Hello World!", textFormat, { 0.0f, 0.0f, 100.f, 100.f }, brush);
		}
		else
		{
			g.drawLine({ 0.0f, 0.0f }, {100.f, 100.f}, brush, 1.0f);
		}
#endif
		return ReturnCode::Ok;
	}

	ReturnCode getClipArea(drawing::Rect* returnRect) override
	{
		*returnRect = bounds;
		return ReturnCode::Ok;
	}

	// IInputClient
	gmpi::ReturnCode onPointerDown(gmpi::drawing::Point point, int32_t flags) override
	{
		inputhost->setCapture();
		return ReturnCode::Ok;
	}
	gmpi::ReturnCode onPointerMove(gmpi::drawing::Point point, int32_t flags) override
	{
		bool isCaptured = false;
		inputhost->getCapture(isCaptured);

		if (isCaptured)
		{
			pinGain = std::clamp((point.x - bounds.left) / (bounds.right - bounds.left), 0.0f, 1.0f);
			editorhost->setPin(0, 0, sizeof(pinGain), &pinGain);

			host->invalidateRect(nullptr);
		}
		return ReturnCode::Ok;
	}
	gmpi::ReturnCode onPointerUp(gmpi::drawing::Point point, int32_t flags) override
	{
		inputhost->releaseCapture();
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

namespace
{
	auto r = Register<AGraphicsGui>::withId("GMPI: GainGui");
}