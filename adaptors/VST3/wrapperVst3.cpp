#include "MyVstPluginFactory.h"

#ifndef _WIN32
#include <CoreFoundation/CoreFoundation.h>

bool bundleEntry_internal (CFBundleRef ref);
bool bundleExit_internal (void);


extern "C" {
/** bundleEntry and bundleExit must be provided by the plug-in! */
SMTG_EXPORT_SYMBOL bool bundleEntry (CFBundleRef r)
{
    return bundleEntry_internal(r);
}

SMTG_EXPORT_SYMBOL bool bundleExit (void)
{
    return bundleExit_internal();
}

}
#endif

// need to export the factory symbol from the main plugin DLL, because the VST3_Wrapper static library can't export symbols on mac.
SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory* PLUGIN_API GetPluginFactory()
{
	return MyVstPluginFactory::GetInstance();
}
