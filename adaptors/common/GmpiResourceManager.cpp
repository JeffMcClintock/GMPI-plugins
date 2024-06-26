
#include "GmpiResourceManager.h"
#include <string>
#include <regex> 
#include "conversion.h"
#include "BundleInfo.h"
// TODO #include "ProtectedFile.h"
//#include "mfc_emulation.h"
#include "unicode_conversion.h"

using namespace std;
using namespace JmUnicodeConversions;

bool ResourceExists(const std::wstring& path)
{
	return BundleInfo::instance()->ResourceExists(JmUnicodeConversions::WStringToUtf8(path).c_str());
}

gmpi::ReturnCode GmpiResourceManager::FindResourceU(int32_t moduleHandle, const std::string skinName, const char* resourceName, const char* resourceType, gmpi::api::IString* returnString)
{
	return RegisterResourceUri(moduleHandle, skinName, resourceName, resourceType, returnString, false);
}

gmpi::ReturnCode GmpiResourceManager::RegisterResourceUri(int32_t moduleHandle, const std::string skinName, const char* resourceName, const char* resourceType, gmpi::api::IString* returnString, bool isIMbeddedResource)
{
	gmpi::api::IString* returnValue{};

	if (gmpi::ReturnCode::Ok != returnString->queryInterface(&gmpi::api::IString::guid, reinterpret_cast<void**>(&returnValue)))
	{
		return gmpi::ReturnCode::NoSupport;
	}

	wstring uri;
	wstring returnUri;
	wstring resourceNameL = JmUnicodeConversions::Utf8ToWstring(resourceName);

	vector<wstring> searchExtensions;
	bool searchWithSkin = false;
	auto bare = StripExtension(resourceNameL);

	wstring standardFolder = resourceFolders[GmpiResourceType::Image];

	if (isEditor())
	{
		if (resourceNameL == L"__fontMetrics" || resourceNameL == L"global") // special magic psudo file.
		{
			returnUri = combine_path_and_file(JmUnicodeConversions::Utf8ToWstring(skinName), JmUnicodeConversions::Utf8ToWstring(resourceName)) + L".txt";
			goto storeFullUri;
		}
	}
	else
	{
		if (resourceNameL == L"__nativePresetsFolder") // special magic 'folder'.
		{
			returnUri = BundleInfo::instance()->getPresetFolder();
			goto storeFullUri;
		}

//		standardFolder = BundleInfo::instance()->getResourceFolder();
	}

	if (strcmp(resourceType, "ImageMeta") == 0)
	{
		searchExtensions.push_back(L".txt");
		searchWithSkin = true;
	}
	else
	{
		if (strcmp(resourceType, "Image") == 0 || strcmp(resourceType, "png") == 0 || strcmp(resourceType, "svg") == 0)
		{
			// see also: isSkinImageFile()
			searchExtensions.push_back(L".png");
			searchExtensions.push_back(L".bmp");
			searchExtensions.push_back(L".jpg");
			searchExtensions.push_back(L".svg");
			searchWithSkin = true;
		}
		else
		{
			if (strcmp(resourceType, "Audio") == 0 || strcmp(resourceType, "wav") == 0)
			{
				searchExtensions.push_back(L".wav");
				standardFolder = resourceFolders[GmpiResourceType::Audio];
			}
			else
			{
				if (strcmp(resourceType, "Instrument") == 0 || strcmp(resourceType, "sfz") == 0 || strcmp(resourceType, "sf2") == 0)
				{
					searchExtensions.push_back(L".sf2");
					searchExtensions.push_back(L".sfz");
					standardFolder = resourceFolders[GmpiResourceType::Soundfont];
				}
				else
				{
					if (strcmp(resourceType, "MIDI") == 0 || strcmp(resourceType, "mid") == 0)
					{
						searchExtensions.push_back(L".mid");
						standardFolder = resourceFolders[GmpiResourceType::Midi];
					}
				}
			}
		}
	}


	// Full filenames.
	if (resourceNameL.find(L':') != string::npos)
	{
		std::vector<wstring> searchPaths;

		if (!isEditor())
		{
			// Cope with "encoded" full filenames. e.g. "C___synth edit projects__scat graphics__duck.png" ( was "C:\\synth edit projects\scat graphics\duck.png")
			std::wstring imbeddedName(resourceNameL);
			imbeddedName = std::regex_replace(imbeddedName, std::basic_regex<wchar_t>(L"/"), L"__");
			imbeddedName = std::regex_replace(imbeddedName, std::basic_regex<wchar_t>(L"\\\\"), L"__"); // single backslash (escaped twice).
			imbeddedName = std::regex_replace(imbeddedName, std::basic_regex<wchar_t>(L":"), L"_");
			searchPaths.push_back(combine_path_and_file(standardFolder, imbeddedName));

			// Paths to non-default, non-standard skin files.  e.g. skins/PD303/UniqueKnob.png (where same image is NOT in default folder).
			// These are stored as full paths by modules to prevent fallback to default skin. However export routine stores these with short names "PD303__UniqueKnob.png".
			if (searchWithSkin)
			{
				auto p = resourceNameL.find(L"\\skins\\");
				if (p != string::npos)
				{
					auto imbeddedName2 = resourceNameL.substr(p + 7);
					imbeddedName2 = std::regex_replace(imbeddedName2, std::basic_regex<wchar_t>(L"\\\\"), L"__"); // single backslash (escaped twice).
					searchPaths.push_back(combine_path_and_file(standardFolder, imbeddedName2)); // prepend resource folder.
				}
			}
		}

		searchPaths.push_back(resourceNameL); // literal long filename. e.g. "C:\Program Files\Whatever\knob.bmp", "C:\Program Files\Whatever\knob" (.txt)

		auto originalExtension = GetExtension(resourceNameL);

		assert(returnUri.empty());

		for (auto path : searchPaths)
		{
			if (returnUri.empty())
			{
				// If extension provided, search that first.
				if (!originalExtension.empty())
				{
					if (FileExists(path))
					{
						returnUri = path;
						break;
					}

					if (ResourceExists(path))
					{
						returnUri = JmUnicodeConversions::Utf8ToWstring(BundleInfo::resourceTypeScheme) + path;
						break;
					}
				}

				// Search different extensions. png, bpm, jpg.
				auto barePath = StripExtension(path);
				for (auto ext : searchExtensions)
				{
					auto temp = barePath + ext;
					if (FileExists(temp))
					{
						returnUri = temp;
						break;
					}
					if (ResourceExists(temp))
					{
						returnUri = JmUnicodeConversions::Utf8ToWstring(BundleInfo::resourceTypeScheme) + temp;
						break;
					}
				}
			}
		}
	}
	else
	{
		// partial filenames (no drive or root slash)
		wstring filenameTemplate = bare;
		std::vector<wstring> searchSkins;

		if (searchWithSkin)
		{
			if (isEditor())
			{
				// ../skins/blue/filename
				filenameTemplate = combine_path_and_file(L"$SKIN", filenameTemplate);
			}
			else
			{
				// VST3/MyPlugin/blue__filename
				filenameTemplate = L"$SKIN__" + bare;
			}

			searchSkins.push_back(JmUnicodeConversions::Utf8ToWstring(skinName));
			searchSkins.push_back(L"default");
		}
		else
		{
			searchSkins.push_back(L"");
		}

		filenameTemplate = combine_path_and_file(standardFolder, filenameTemplate);

		for( auto searchSkin : searchSkins)
		{
			// substitute string name if applicable.
			uri = filenameTemplate;
			auto start_pos = uri.find(L"$SKIN");
			if (start_pos != string::npos)
			{
				uri.replace(start_pos, 5, searchSkin);
			}

			// First try given extension.
			if (FileExists(uri))
			{
				returnUri = uri;
				break;
			}
			if (ResourceExists(uri))
			{
				returnUri = JmUnicodeConversions::Utf8ToWstring(BundleInfo::resourceTypeScheme) + uri;
				break;
			}

			// Search different extensions. png, bpm, jpg.
			auto barePath = uri; // Already stripped. Don't double-strip filenames containing dots (g.a.bmp). StripExtension(uri);
			for (auto ext : searchExtensions)
			{
				auto temp = barePath + ext;
				if (FileExists(temp))
				{
					returnUri = temp;
					break;
				}
				if (ResourceExists(temp))
				{
					returnUri = JmUnicodeConversions::Utf8ToWstring(BundleInfo::resourceTypeScheme) + temp;
					break;
				}
			}

			if (! returnUri.empty() )
			{
				break;
			}
		}
	}

	storeFullUri:

	string fullUri = JmUnicodeConversions::WStringToUtf8(returnUri);

	// NOTE: Had to disable whole-program-optimisation to prevent compiler inlining this(which corrupted std::string). Not sure how it figured out to inline it.
	returnValue->setData(fullUri.data(), (int32_t) fullUri.size());

	if( returnUri.empty() )
	{
#ifdef _WIN32
		_RPT1(0, "GmpiResourceManager::RegisterResourceUri(%s) Not Found\n", resourceName);
#endif
		return gmpi::ReturnCode::Fail;
	}

	if(isEditor() && isIMbeddedResource)
	{
		// Cache name.
		assert(moduleHandle > -1);
		resourceUris_.insert({ moduleHandle, fullUri });
	}

	return gmpi::ReturnCode::Ok;
}

void GmpiResourceManager::ClearResourceUris(int32_t moduleHandle)
{
	resourceUris_.erase(moduleHandle);
}

void GmpiResourceManager::ClearAllResourceUris()
{
	resourceUris_.clear();
}

#if 0 // TODO
int32_t GmpiResourceManager::OpenUri(const char* fullUri, gmpi::IProtectedFile2** returnStream)
{
	std::string uriString(fullUri);
	if (uriString.find(BundleInfo::resourceTypeScheme) == 0)
	{
		*returnStream = new ProtectedMemFile2(BundleInfo::instance()->getResource(fullUri + strlen(BundleInfo::resourceTypeScheme)));
	}
	else
	{
		*returnStream = ProtectedFile2::FromUri(fullUri);
	}

	return *returnStream != nullptr ? (gmpi::MP_OK) : (gmpi::MP_FAIL);
}
#endif
