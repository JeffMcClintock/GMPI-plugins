#include "MyVstPluginFactory.h"

// need to export the factory symbol from the plugin DLL, the static library can't export symbols on mac.
SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory* PLUGIN_API GetPluginFactory()
{
	return MyVstPluginFactory::GetInstance();
}

bool InitModule() { return true; }
bool DeinitModule() { return true; }
