#include "VST3EditorBase.h"
#include "adelaycontroller.h"
#include "MyVstPluginFactory.h"

ParameterHelper::ParameterHelper(VST3EditorBase* editor)
{
	editor_ = editor;
}

gmpi::ReturnCode ParameterHelper::setParameter(int32_t parameterHandle, gmpi::Field fieldId, int32_t voice, int32_t size, const void* data)
{
	editor_->onParameterUpdate(parameterHandle, fieldId, voice, data, size);
    return gmpi::ReturnCode::Ok;
}

gmpi::ReturnCode ParameterHelper::queryInterface(const gmpi::api::Guid* iid, void** returnInterface)
{
    GMPI_QUERYINTERFACE(gmpi::api::IEditorHost);
    GMPI_QUERYINTERFACE(gmpi::api::IParameterObserver);
	return gmpi::ReturnCode::NoSupport;
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
VST3EditorBase::VST3EditorBase(pluginInfoSem const& info, gmpi::shared_ptr<gmpi::api::IEditor>& peditor, Steinberg::Vst::VST3Controller* pcontroller, int pwidth, int pheight) :
	controller(pcontroller)
	, width(pwidth)
	, height(pheight)
	, pluginParameters_GMPI(peditor)
	, helper(this)
	, info(info)
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

void VST3EditorBase::onParameterUpdate(int32_t parameterHandle, gmpi::Field fieldId, int32_t voice, const void* data, int32_t size)
{
	if (!pluginParameters_GMPI)
		return;

	int32_t moduleHandle{-2};
	int32_t moduleParameterId{-2};
	controller->getParameterModuleAndParamId(parameterHandle, &moduleHandle, &moduleParameterId);

	for (const auto& pin : info.guiPins)
	{
		if (pin.parameterId == moduleParameterId && pin.parameterFieldType == fieldId)
		{
			pluginParameters_GMPI->setPin(pin.id, voice, size, data);
			break;
		}
	}
}

