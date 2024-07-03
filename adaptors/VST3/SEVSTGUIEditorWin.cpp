#include "SEVSTGUIEditorWin.h"
#include "adelaycontroller.h"

// TODO !!! pass IUnknown to constructor, then QueryInterface for IDrawingClient
SEVSTGUIEditorWin::SEVSTGUIEditorWin(pluginInfoSem const& info, gmpi::shared_ptr<gmpi::api::IEditor>& peditor, Steinberg::Vst::VST3Controller* pcontroller, int pwidth, int pheight) :
	VST3EditorBase(info, peditor, pcontroller, pwidth, pheight)
{
}

SEVSTGUIEditorWin::~SEVSTGUIEditorWin()
{
	controller->UnRegisterGui2(&helper);
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorWin::attached (void* parent, Steinberg::FIDString type)
{
    if (pluginGraphics_GMPI)
    {
        drawingframe.AddView(static_cast<gmpi::api::IParameterObserver*>(&helper), pluginGraphics_GMPI.get());

        const gmpi::drawing::SizeL overrideSize{ width, height };
        drawingframe.open(parent, &overrideSize);

        controller->initUi(&helper);
    }

	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorWin::removed ()
{
//    onCloseNativeView(nsView);
    
	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorWin::getSize (Steinberg::ViewRect* size)
{
    *size = {0, 0, width, height};
	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorWin::onSize(Steinberg::ViewRect* newSize)
{
    drawingframe.reSize(newSize->left, newSize->top, newSize->right, newSize->bottom);
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorWin::canResize()
{
    if (pluginGraphics_GMPI)
    {
        const gmpi::drawing::Size availableSize1{ 0.0f, 0.0f };
        const gmpi::drawing::Size availableSize2{ 10000.0f, 10000.0f };
        gmpi::drawing::Size desiredSize1{ availableSize1 };
        gmpi::drawing::Size desiredSize2{ availableSize1 };
        pluginGraphics_GMPI->measure(&availableSize1, &desiredSize1);
        pluginGraphics_GMPI->measure(&availableSize2, &desiredSize2);

        if (desiredSize1 != desiredSize2)
        {
            return Steinberg::kResultTrue;
        }
    }
    return Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorWin::checkSizeConstraint(Steinberg::ViewRect* rect)
{
    if (pluginGraphics_GMPI)
    {
		const gmpi::drawing::Size availableSize{ static_cast<float>(rect->right - rect->left), static_cast<float>(rect->bottom - rect->top) };
        gmpi::drawing::Size desiredSize{ availableSize };
		pluginGraphics_GMPI->measure(&availableSize, &desiredSize);

        if (availableSize == desiredSize)
        {
            return Steinberg::kResultTrue;
        }
    }
    return Steinberg::kResultFalse;
}
