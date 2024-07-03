#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "base/source/fobject.h"
#include "GmpiSdkCommon.h"
#include "GmpiApiEditor.h"
#include "helpers/GraphicsRedrawClient.h"
#include "VST3EditorBase.h"

class SEVSTGUIEditorMac : public VST3EditorBase //Steinberg::FObject, public Steinberg::IPlugView
{
    void* nsView = {};
    
public:
    SEVSTGUIEditorMac(pluginInfoSem const& info, gmpi::shared_ptr<gmpi::api::IEditor>& peditor, Steinberg::Vst::VST3Controller* controller, int width, int height);

	//---from IPlugView-------
	Steinberg::tresult PLUGIN_API isPlatformTypeSupported (Steinberg::FIDString type) SMTG_OVERRIDE { return Steinberg::kResultTrue; }
	Steinberg::tresult PLUGIN_API attached (void* parent, Steinberg::FIDString type) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API removed () SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API onWheel (float /*distance*/) SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API onKeyDown (Steinberg::char16 /*key*/, Steinberg::int16 /*keyMsg*/,
	                              Steinberg::int16 /*modifiers*/) SMTG_OVERRIDE {return Steinberg::kResultFalse;}
	Steinberg::tresult PLUGIN_API onKeyUp (Steinberg::char16 /*key*/, Steinberg::int16 /*keyMsg*/, Steinberg::int16 /*modifiers*/) SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API getSize (Steinberg::ViewRect* size) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API onSize (Steinberg::ViewRect* newSize) SMTG_OVERRIDE { return Steinberg::kResultTrue; }

	Steinberg::tresult PLUGIN_API onFocus (Steinberg::TBool /*state*/) SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API setFrame (Steinberg::IPlugFrame* frame) SMTG_OVERRIDE	{return Steinberg::kResultTrue;	}

	Steinberg::tresult PLUGIN_API canResize () SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API checkSizeConstraint (Steinberg::ViewRect* /*rect*/) SMTG_OVERRIDE	{return Steinberg::kResultFalse;}
};
