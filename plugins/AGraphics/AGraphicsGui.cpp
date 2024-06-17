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
class AGraphicsGui final : public gmpi::api::IEditor, public gmpi::api::IDrawingClient
{
	Rect bounds;

public:
	AGraphicsGui()
	{
#if 1
// hack for now, to prevent linker from discarding plugin factory
		auto test = GetPluginFactory();
#endif
	}

	// IEditor
	ReturnCode setHost(gmpi::api::IUnknown* host) override
	{
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

	ReturnCode onRender(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

		ClipDrawingToBounds _(g, bounds);

		g.clear(Colors::YellowGreen);

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
		return ReturnCode::Ok;
	}

	ReturnCode getClipArea(drawing::Rect* returnRect) override
	{
		*returnRect = bounds;
		return ReturnCode::Ok;
	}

	// IUnknown
	ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
	{
		GMPI_QUERYINTERFACE(gmpi::api::IEditor);
		GMPI_QUERYINTERFACE(gmpi::api::IDrawingClient);
		return ReturnCode::NoSupport;
	}
	GMPI_REFCOUNT;

#if 0
	ReturnCode OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override // perhaps: ReturnCode OnRender(Graphics& g)
	{
		Graphics g(drawingContext);

		auto textFormat = GetGraphicsFactory().CreateTextFormat();
		auto brush = g.CreateSolidColorBrush(Color::Red);

		g.DrawTextU("Hello World!", textFormat, 0.0f, 0.0f, brush);

		return ReturnCode::Ok;
	}
#endif

};

namespace
{
auto r = Register<AGraphicsGui>::withXml(R"XML(
<?xml version="1.0" encoding="utf-8" ?>

<PluginList>
  <Plugin id="JM Graphics" name="Graphics" category="GMPI/SDK Examples">
    <GUI graphicsApi="GmpiGui"/>
  </Plugin>
</PluginList>
)XML");
}