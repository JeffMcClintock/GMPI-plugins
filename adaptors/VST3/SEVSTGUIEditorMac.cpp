#include "SEVSTGUIEditorMac.h"
#include "adelaycontroller.h"

// without including objective-C headers, we need to create an SynthEditCocoaView (NSView).
// forward declare function here to return the view, using void* as return type.
void* createNativeView(void* parent, class IUnknown* parameterHost, class IUnknown* controller, int width, int height);
void onCloseNativeView(void* ptr);

SEVSTGUIEditorMac::SEVSTGUIEditorMac(gmpi::shared_ptr<gmpi::api::IEditor>& peditor, Steinberg::Vst::VST3Controller* pcontroller, int pwidth, int pheight) :
    VST3EditorBase(peditor, pcontroller, pwidth, pheight)
//    controller(pcontroller)
//, width(pwidth)
//, height(pheight)
//, pluginParameters_GMPI(peditor)
{
    //pluginGraphics_GMPI = peditor.As<gmpi::api::IDrawingClient>();
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorMac::attached (void* parent, Steinberg::FIDString type)
{
    nsView = createNativeView(parent, (class IUnknown*) static_cast<gmpi::api::IEditorHost*>(&helper), (class IUnknown*) pluginGraphics_GMPI.get(), width, height);

	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorMac::removed ()
{
    onCloseNativeView(nsView);
    
	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorMac::getSize (Steinberg::ViewRect* size)
{
    *size = {0, 0, width, height};
	return Steinberg::kResultTrue;
}

gmpi::ReturnCode SEVSTGUIEditorMac::queryInterfaceFromHelper(const gmpi::api::Guid* iid, void** returnInterface)
{
    // need to get to drawingframe in nsview?????
 //   return drawingframe.queryInterface(iid, returnInterface);
}
