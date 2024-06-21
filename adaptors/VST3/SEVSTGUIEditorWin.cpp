#include "SEVSTGUIEditorWin.h"
#include "adelaycontroller.h"

ParameterHelper::ParameterHelper(SEVSTGUIEditorWin* editor)
{
	editor_ = editor;
}

gmpi::ReturnCode ParameterHelper::setParameter(int32_t parameterHandle, int32_t fieldId, int32_t voice, int32_t size, const void* data)
{
	editor_->onParameterUpdate(parameterHandle, fieldId, voice, data, size);
    return gmpi::ReturnCode::Ok;
}

gmpi::ReturnCode ParameterHelper::queryInterface(const gmpi::api::Guid* iid, void** returnInterface)
{
    GMPI_QUERYINTERFACE(gmpi::api::IEditorHost);
    GMPI_QUERYINTERFACE(gmpi::api::IParameterObserver);

    return editor_->drawingframe.queryInterface(iid, returnInterface);
}

// GMPI Editor sending a parameter update back to the wrapper.
gmpi::ReturnCode ParameterHelper::setPin(int32_t pinId, int32_t voice, int32_t size, const void* data)
{
	editor_->controller->setPinFromUi(pinId, voice, size, data);
    return gmpi::ReturnCode::Ok;
}

int32_t ParameterHelper::getHandle()
{
    return 0;
}

// TODO !!! pass IUnknown to constructor, then QueryInterface for IDrawingClient
SEVSTGUIEditorWin::SEVSTGUIEditorWin(gmpi::shared_ptr<gmpi::api::IEditor>& peditor, Steinberg::Vst::VST3Controller* pcontroller, int pwidth, int pheight) :
controller(pcontroller)
, width(pwidth)
, height(pheight)
, pluginParameters_GMPI(peditor)
, helper(this)
{
    pluginGraphics_GMPI = peditor.As<gmpi::api::IDrawingClient>();

    controller->RegisterGui2(&helper);

    if(peditor)
        peditor->setHost(static_cast<gmpi::api::IEditorHost*>(&helper));
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

    // TO DO !!! LOOKUP PARAM HANDLE
    //controller->initializeGui(&helper, 3, gmpi::FieldType::MP_FT_VALUE);
    controller->initUi(&helper);

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
