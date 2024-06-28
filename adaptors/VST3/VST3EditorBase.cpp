#include "VST3EditorBase.h"
#include "adelaycontroller.h"

ParameterHelper::ParameterHelper(VST3EditorBase* editor)
{
	editor_ = editor;
}

gmpi::ReturnCode ParameterHelper::setParameter(int32_t parameterHandle, gmpi::FieldType fieldId, int32_t voice, int32_t size, const void* data)
{
	editor_->onParameterUpdate(parameterHandle, fieldId, voice, data, size);
    return gmpi::ReturnCode::Ok;
}

gmpi::ReturnCode ParameterHelper::queryInterface(const gmpi::api::Guid* iid, void** returnInterface)
{
    GMPI_QUERYINTERFACE(gmpi::api::IEditorHost);
    GMPI_QUERYINTERFACE(gmpi::api::IParameterObserver);

	return editor_->queryInterfaceFromHelper(iid, returnInterface);
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
VST3EditorBase::VST3EditorBase(gmpi::shared_ptr<gmpi::api::IEditor>& peditor, Steinberg::Vst::VST3Controller* pcontroller, int pwidth, int pheight) :
	controller(pcontroller)
	, width(pwidth)
	, height(pheight)
	, pluginParameters_GMPI(peditor)
	, helper(this)
{
	pluginGraphics_GMPI = peditor.As<gmpi::api::IDrawingClient>();

	controller->RegisterGui2(&helper);
#ifdef _WIN32
	if (peditor)
		peditor->setHost(static_cast<gmpi::api::IEditorHost*>(&helper));
#endif
}

VST3EditorBase::~VST3EditorBase()
{
	controller->UnRegisterGui2(&helper);
}

void VST3EditorBase::onParameterUpdate(int32_t parameterHandle, gmpi::FieldType fieldId, int32_t voice, const void* data, int32_t size)
{
	if (!pluginParameters_GMPI)
		return;

	if (fieldId == gmpi::FieldType::MP_FT_VALUE) // value TODO: lookup pinID from parameterHandle
    {
        int32_t pinId = 0;
	    pluginParameters_GMPI->setPin(pinId, voice, size, data);
    }
}

