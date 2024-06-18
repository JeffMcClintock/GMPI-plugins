#include "SEVSTGUIEditorWin.h"
#include "adelaycontroller.h"
//#include "JsonDocPresenter.h"

ParameterHelper::ParameterHelper(class SEVSTGUIEditorWin* editor)
{
	editor_ = editor;
}

int32_t ParameterHelper::setParameter(int32_t parameterHandle, int32_t fieldId, int32_t voice, const void* data, int32_t size)
{
	editor_->onParameterUpdate(parameterHandle, fieldId, voice, data, size);
	return 0;
}

GuiHelper::GuiHelper(class SEVSTGUIEditorWin* editor)
{
    editor_ = editor;
}

void GuiHelper::invalidateRect(const gmpi::drawing::Rect* invalidRect)
{
    editor_->drawingframe.invalidateRect(invalidRect);
}

// TODO !!! pass IUnknown to constructor, then QueryInterface for IDrawingClient
SEVSTGUIEditorWin::SEVSTGUIEditorWin(gmpi::shared_ptr<gmpi::api::IEditor>& peditor, IGuiHost2* pcontroller, int pwidth, int pheight) :
controller(pcontroller)
, width(pwidth)
, height(pheight)
, pluginParameters_GMPI(peditor)
, helper(this)
, guiHelper(this)
{
    pluginGraphics_GMPI = peditor.As<gmpi::api::IDrawingClient>();

    controller->RegisterGui2(&helper);

    if(peditor)
        peditor->setHost((gmpi::api::IUnknown*) &guiHelper);
}

SEVSTGUIEditorWin::~SEVSTGUIEditorWin()
{
	controller->UnRegisterGui2(&helper);
}

void SEVSTGUIEditorWin::onParameterUpdate(int32_t parameterHandle, int32_t fieldId, int32_t voice, const void* data, int32_t size)
{
	if (!pluginParameters_GMPI)
		return;

	if (fieldId == 0) // value TODO: lookup pinID from parameterHandle
    {
        int32_t pinId = 0;
	    pluginParameters_GMPI->setPin(pinId, voice, size, data);
    }
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorWin::attached (void* parent, Steinberg::FIDString type)
{
    if (pluginGraphics_GMPI)
    {
        drawingframe.AddView(pluginGraphics_GMPI.get());

        const gmpi::drawing::SizeL overrideSize{ width, height };
        drawingframe.open(parent, &overrideSize);
    }
#if 0
    auto presenter = new JsonDocPresenter(controller);
    presenter->RefreshView();

    // Ableton opens at the wrong size, then resizes to correct size. Which we handle in onSize()
    // Voltage Modular never sets the size. Have to just force it.
    const GmpiDrawing_API::MP1_SIZE_L overrideSize{ width, height };

    drawingframe.open(parent, &overrideSize);
#endif

    // TODO !!! LOOKUP PARAM HANDLE
    controller->initializeGui(&helper, 3, gmpi::FieldType::MP_FT_VALUE);

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
