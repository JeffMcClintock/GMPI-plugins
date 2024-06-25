#pragma once

#include <map>
#include <string>
#include <unordered_map>
//#include "mp_api.h"
//#include "mp_sdk_gui2.h"
#include "GmpiApiCommon.h"

enum class GmpiResourceType
{
	Image,
	Audio,
	Soundfont,
	Midi
};

class GmpiResourceManager
{
public:
	static GmpiResourceManager* Instance(); // ref: platform_plugin.cpp or GmpiResourceManager_editor.cpp

	std::multimap< int32_t, std::string > resourceUris_;
	std::unordered_map< GmpiResourceType, std::wstring > resourceFolders;
	virtual bool isEditor() { return false; }

	void ClearResourceUris(int32_t moduleHandle);
	void ClearAllResourceUris();
	gmpi::ReturnCode RegisterResourceUri(int32_t moduleHandle, const std::string skinName, const char* resourceName, const char* resourceType, gmpi::api::IString* returnString, bool isIMbeddedResource = true);
	gmpi::ReturnCode FindResourceU(int32_t moduleHandle, const std::string skinName, const char* resourceName, const char* resourceType, gmpi::api::IString* returnString);
//TODO	virtual int32_t OpenUri(const char* fullUri, gmpi::IProtectedFile2** returnStream);
};

