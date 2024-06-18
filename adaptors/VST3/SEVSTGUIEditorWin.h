#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "base/source/fobject.h"
#include "backends/DrawingFrameWin.h"
#include "GmpiSdkCommon.h"
#include "GmpiApiEditor.h"
#include "GmpiApiDrawingClient.h"

#include "mp_sdk_common.h" // TODO remove dependency (on legacy IMpParameterObserver)

class ParameterHelper : public gmpi::IMpParameterObserver
{
	class SEVSTGUIEditorWin* editor_ = {};

public:
	ParameterHelper(class SEVSTGUIEditorWin* editor);

	//---IMpParameterObserver------
	int32_t setParameter(int32_t parameterHandle, int32_t fieldId, int32_t voice, const void* data, int32_t size) override;

	GMPI_QUERYINTERFACE1(gmpi::MP_IID_PARAMETER_OBSERVER, gmpi::IMpParameterObserver);
	GMPI_REFCOUNT;
};

class GuiHelper : public gmpi::api::IDrawingHost
{
	class SEVSTGUIEditorWin* editor_ = {};

public:
	GuiHelper(class SEVSTGUIEditorWin* editor);

	// IDrawingHost
	gmpi::ReturnCode getDrawingFactory(gmpi::api::IUnknown** returnFactory) override
	{
		return gmpi::ReturnCode::NoSupport;
	}

	void invalidateRect(const gmpi::drawing::Rect* invalidRect) override;

	GMPI_QUERYINTERFACE_METHOD(gmpi::api::IDrawingHost);
	GMPI_REFCOUNT;
};

class SEVSTGUIEditorWin : public Steinberg::FObject, public Steinberg::IPlugView
{
	friend class GuiHelper;
	GmpiGuiHosting::DrawingFrame drawingframe;
	class IGuiHost2* controller = {};
    int width, height;
    
	gmpi::shared_ptr<gmpi::api::IEditor> pluginParameters_GMPI;
	gmpi::shared_ptr<gmpi::api::IDrawingClient> pluginGraphics_GMPI;
	ParameterHelper helper;
	GuiHelper guiHelper;

public:
    SEVSTGUIEditorWin(gmpi::shared_ptr<gmpi::api::IEditor>& peditor, IGuiHost2* controller, int width, int height);
	~SEVSTGUIEditorWin();

	void onParameterUpdate(int32_t parameterHandle, int32_t fieldId, int32_t voice, const void* data, int32_t size);

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

	Steinberg::tresult PLUGIN_API canResize () SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API checkSizeConstraint (Steinberg::ViewRect* /*rect*/) SMTG_OVERRIDE	{return Steinberg::kResultFalse;}

	//---Interface------
	OBJ_METHODS (SEVSTGUIEditorWin, Steinberg::FObject)
	DEFINE_INTERFACES
	DEF_INTERFACE (IPlugView)
	END_DEFINE_INTERFACES (Steinberg::FObject)
	REFCOUNT_METHODS (Steinberg::FObject)
};
