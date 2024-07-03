#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "base/source/fobject.h"
#include "backends/DrawingFrameWin.h"
#include "GmpiSdkCommon.h"
#include "GmpiApiEditor.h"
#include "VST3EditorBase.h"

#if 0
namespace Steinberg
{
namespace Vst
{
	class VST3Controller;
}
}

class ParameterHelper : public gmpi::api::IParameterObserver, public gmpi::api::IEditorHost
{
	class SEVSTGUIEditorWin* editor_ = {};

public:
	ParameterHelper(class SEVSTGUIEditorWin* editor);

	//---IParameterObserver------
	gmpi::ReturnCode setParameter(int32_t parameterHandle, gmpi::Field fieldId, int32_t voice, int32_t size, const void* data) override;

	//---IEditorHost------
	gmpi::ReturnCode setPin(int32_t pinId, int32_t voice, int32_t size, const void* data) override;
	int32_t getHandle() override;

	gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override;
	GMPI_REFCOUNT;
};
#endif

class SEVSTGUIEditorWin : public VST3EditorBase //Steinberg::FObject, public Steinberg::IPlugView
{
	GmpiGuiHosting::DrawingFrame drawingframe;

public:
    SEVSTGUIEditorWin(pluginInfoSem const& info, gmpi::shared_ptr<gmpi::api::IEditor>& peditor, Steinberg::Vst::VST3Controller* controller, int width, int height);
	~SEVSTGUIEditorWin();

//	void onParameterUpdate(int32_t parameterHandle, gmpi::Field fieldId, int32_t voice, const void* data, int32_t size);

	//---from IPlugView-------
	Steinberg::tresult PLUGIN_API isPlatformTypeSupported (Steinberg::FIDString type) SMTG_OVERRIDE { return Steinberg::kResultTrue; }
	Steinberg::tresult PLUGIN_API attached (void* parent, Steinberg::FIDString type) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API removed () SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API onWheel (float /*distance*/) SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API onKeyDown (Steinberg::char16 /*key*/, Steinberg::int16 /*keyMsg*/,
	                              Steinberg::int16 /*modifiers*/) SMTG_OVERRIDE {return Steinberg::kResultFalse;}
	Steinberg::tresult PLUGIN_API onKeyUp (Steinberg::char16 /*key*/, Steinberg::int16 /*keyMsg*/, Steinberg::int16 /*modifiers*/) SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API getSize (Steinberg::ViewRect* size) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* newSize) SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API onFocus (Steinberg::TBool /*state*/) SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API setFrame (Steinberg::IPlugFrame* frame) SMTG_OVERRIDE	{return Steinberg::kResultTrue;	}

	Steinberg::tresult PLUGIN_API canResize() SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect* /*rect*/) SMTG_OVERRIDE;

#if 0
	//---Interface------
	OBJ_METHODS (SEVSTGUIEditorWin, Steinberg::FObject)
	DEFINE_INTERFACES
	DEF_INTERFACE (IPlugView)
	END_DEFINE_INTERFACES (Steinberg::FObject)
	REFCOUNT_METHODS (Steinberg::FObject)
#endif
};
