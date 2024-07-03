#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstcomponent.h"
#include "MyVstPluginFactory.h"
#include "adelaycontroller.h"
#include "adelayprocessor.h"
#include "tinyXml2/tinyxml2.h"
#include "dynamic_linking.h"
#include "Common.h"

#if 0
#include "it_enum_list.h"
#include "BundleInfo.h"
#include "FileFinder.h"
#include "FileFinder.h"
#include "GmpiApiAudio.h"
#include "GmpiSdkCommon.h"
#endif

#if defined( _WIN32 )
extern HINSTANCE ghInst;
#else
	#include <dlfcn.h>
	#include <CoreFoundation/CoreFoundation.h>

/*
// Hacky, access functions meant only for VSTGUI.
namespace VSTGUI
{
    static void CreateVSTGUIBundleRef();
    static void ReleaseVSTGUIBundleRef();
}
*/

#if 0
// Copied from: public.sdk/source/vst/vstguieditor.cpp
//void* gBundleRef = 0;
//static int openCount = 0;
//CFBundleRef getBundleRef(){return (CFBundleRef) gBundleRef;};
//------------------------------------------------------------------------
CFBundleRef CreatePluginBundleRef ()
{
    CFBundleRef rBundleRef = 0;
    
	Dl_info info;
	if (dladdr ((const void*)CreatePluginBundleRef, &info))
	{
		if (info.dli_fname)
		{
			Steinberg::String name;
			name.assign (info.dli_fname);
			for (int i = 0; i < 3; i++)
			{
				int delPos = name.findLast ('/');
				if (delPos == -1)
				{
					fprintf (stdout, "Could not determine bundle location.\n");
					return 0; // unexpected
				}
				name.remove (delPos, name.length () - delPos);
			}
			CFURLRef bundleUrl = CFURLCreateFromFileSystemRepresentation (0, (const UInt8*)name.text8 (), name.length (), true);
			if (bundleUrl)
			{
				rBundleRef = CFBundleCreate (0, bundleUrl);
				CFRelease (bundleUrl);
			}
		}
	}
    
    return rBundleRef;
}

void ReleasePluginBundleRef (CFBundleRef bundleRef)
{
	CFRelease (bundleRef);
}
#endif
/*
//------------------------------------------------------------------------
void ReleaseVSTGUIBundleRef ()
{
	openCount--;
	if (gBundleRef)
		CFRelease (gBundleRef);
	if (openCount == 0)
		gBundleRef = 0;
}
 */
#endif

using namespace Steinberg;
using namespace Steinberg::Vst;

extern "C"
gmpi::ReturnCode MP_GetFactory( void** returnInterface );

#if 0
SMTG_EXPORT_SYMBOL IPluginFactory* PLUGIN_API GetPluginFactory ()
{
	return MyVstPluginFactory::GetInstance();
}
#endif

bool InitModule() { return true; }
bool DeinitModule() { return true; }

MyVstPluginFactory* MyVstPluginFactory::GetInstance()
{
	static MyVstPluginFactory singleton;
	return &singleton;
}

MyVstPluginFactory::MyVstPluginFactory()
{
    // ensure linker retains GMPI plugin
//    void* returnInterface{};
//    const auto r =  MP_GetFactory(&returnInterface);

//    const auto r = gmpi::api::MpFactory::createInstance();
}

MyVstPluginFactory::~MyVstPluginFactory()
{
}

/** Fill a PFactoryInfo structure with information about the Plug-in vendor. */
tresult MyVstPluginFactory::getFactoryInfo (PFactoryInfo* info)
{
	initialize();

	strncpy8 (info->vendor, vendorName_.c_str(), PFactoryInfo::kNameSize);
	strncpy8 (info->url, vendorUrl_.c_str(), PFactoryInfo::kURLSize);
	strncpy8 (info->email, vendorEmail_.c_str(), PFactoryInfo::kEmailSize);
	info->flags = PFactoryInfo::kUnicode;
	return kResultOk;
}

/** Returns the number of exported classes by this factory. 
If you are using the CPluginFactory implementation provided by the SDK, it returns the number of classes you registered with CPluginFactory::registerClass. */
int32 MyVstPluginFactory::countClasses ()
{
	initialize();
	return plugins.size() * 2;
}

uint32 hashString(const std::string& s)
{
	uint32_t hash = 5381;
	{
		for (auto c : s)
		{
			hash = ((hash << 5) + hash) + c; // magic * 33 + c
		}
	}

	return hash;
}
	
void textIdtoUuid(const std::string& id, bool isController, Steinberg::TUID& ret) // else Processor
{
	// hash the id
	const auto hash = hashString(id);// ^ (isController ? 0xff : 0);

	//union helper_t
	//{
	//	char plain[16] = { "PluginGMPI     " }; // UUID version 4 because 'G' is 0x47
	//	Steinberg::TUID tuid;
	//};

	//helper_t helper;
	memcpy(ret, "PluginGMPI     ", 16);
	ret[11] = isController ? 'C' : 'P';

	memcpy(ret + 12, &hash, sizeof(hash));

//	helper.plain[6] = 0x40; // UUID version 4

//	return helper.tuid;
}

/** Fill a PClassInfo structure with information about the class at the specified index. */
tresult MyVstPluginFactory::getClassInfo (int32 index, PClassInfo* info)
{
	initialize();

	if (index < 0 || index >= plugins.size())
	{
		return kInvalidArgument;
	}

	const int pluginIndex = index / 2;
	const int classIndex = index % 2;

	const auto& sem = plugins[pluginIndex];

	Steinberg::TUID procUUid{};
	Steinberg::TUID ctrlUUid{};
	textIdtoUuid(sem.id, false, procUUid);
	textIdtoUuid(sem.id,  true, ctrlUUid);

	switch(classIndex)
	{
	case 0:
		strncpy8 (info->category, kVstAudioEffectClass, PClassInfo::kCategorySize );
		memcpy(info->cid, &procUUid/*(pluginInfo_.processorId.toTUID()*/, sizeof(TUID));
		break;
	case 1:
		strncpy8 (info->category, kVstComponentControllerClass, PClassInfo::kCategorySize );
		memcpy(info->cid, &ctrlUUid/*(pluginInfo_.controllerId.toTUID()*/, sizeof(TUID));
		break;
	}

	info->cardinality = PClassInfo::kManyInstances;

	strncpy8 (info->name, sem.name.c_str(), PClassInfo::kNameSize );

	return kResultOk;
}

/** Returns the class info (version 2) for a given index. */
tresult MyVstPluginFactory::getClassInfo2 (int32 index, PClassInfo2* info)
{
	initialize();

	if (index < 0 || index >= plugins.size())
	{
		return kInvalidArgument;
	}

	std::string version{ "1.0.0" }; // for now
	std::string subCategories{ "" }; // for now

	const int pluginIndex = index / 2;
	const int classIndex = index % 2;

	const auto& sem = plugins[pluginIndex];

	Steinberg::TUID procUUid{};
	Steinberg::TUID ctrlUUid{};
	textIdtoUuid(sem.id, false, procUUid);
	textIdtoUuid(sem.id, true, ctrlUUid);

	info->cardinality = PClassInfo::kManyInstances;

	strncpy8 (info->name, plugins[pluginIndex].name.c_str(), PClassInfo::kNameSize );
	strncpy8 (info->sdkVersion, kVstVersionString, PClassInfo2::kVersionSize );
	strncpy8 (info->vendor, vendorName_.c_str(), PClassInfo2::kVendorSize );
	strncpy8 (info->version, /*pluginInfo_.version_*/version.c_str(), PClassInfo2::kVersionSize );

	switch(classIndex)
	{
	case 0:
		info->classFlags = Vst::kDistributable;
		strncpy8 (info->subCategories, /*pluginInfo_.subCategories_*/subCategories.c_str(), PClassInfo2::kSubCategoriesSize );
		strncpy8 (info->category, kVstAudioEffectClass, PClassInfo::kCategorySize );
		memcpy (info->cid, &procUUid/*(pluginInfo_.processorId.toTUID())*/, sizeof (TUID));
		break;
	case 1:
		info->classFlags = 0;
		strncpy8 (info->subCategories, "", PClassInfo2::kSubCategoriesSize );
		strncpy8 (info->category, kVstComponentControllerClass, PClassInfo::kCategorySize );
		memcpy (info->cid, &ctrlUUid/*(pluginInfo_.controllerId.toTUID())*/, sizeof (TUID));
		break;
	}

	return kResultOk;
}

//int32_t MyVstPluginFactory::getVst2Id(int32_t index) // not used for historic reasons. 32-bit comptible.
//{
//	initialize();
//
//	if (index != 0)
//	{
//		return kInvalidArgument;
//	}
//
//	return backwardCompatible4charId;
//}

int32_t MyVstPluginFactory::getVst2Id64(int32_t pluginIndex) // generated from hash of GUID. Not compatible w 32-bit VSTs.
{
	initialize();

	if (pluginIndex != 0)
	{
		return kInvalidArgument;
	}

	Steinberg::PClassInfoW info;
	getClassInfoUnicode(pluginIndex, &info);

	// generate an VST2 ID from VST3 GUIID.
	// see also CContainer::VstUniqueID()
	unsigned char vst2ID[4]{};
	int i2 = 0;
	for (int i = 0; i < sizeof(info.cid); ++i)
	{
		vst2ID[i2] = vst2ID[i2] + info.cid[i];
		i2 = (i2 + 1) & 0x03;
	}

	for (auto& c : vst2ID)
	{
		c = 0x20 + (std::min)(0x7e, c & 0x7f); // make printable
	}

	return *((int32_t*)vst2ID);
}

/** Returns the unicode class info for a given index. */
tresult MyVstPluginFactory::getClassInfoUnicode (int32 index, PClassInfoW* info)
{
	initialize();
 
	if( index < 0 || index >= plugins.size() )
	{
		return kInvalidArgument;
	}

	std::string version{ "1.0.0" }; // for now
	std::string subCategories{ "" }; // for now

	const int pluginIndex = index / 2;
	const int classIndex = index % 2;

	const auto& sem = plugins[pluginIndex];

	Steinberg::TUID procUUid{};
	Steinberg::TUID ctrlUUid{};
	textIdtoUuid(sem.id, false, procUUid);
	textIdtoUuid(sem.id, true, ctrlUUid);

	switch(classIndex)
	{
	case 0:
		info->classFlags = Vst::kDistributable;
		strncpy8 (info->subCategories, /*pluginInfo_.subCategories_*/subCategories.c_str(), PClassInfo2::kSubCategoriesSize );
		strncpy8 (info->category, kVstAudioEffectClass, PClassInfo::kCategorySize );
		memcpy(info->cid, &procUUid/*(pluginInfo_.processorId.toTUID())*/, sizeof(TUID));
		break;
	case 1:
		info->classFlags = 0;
		strncpy8 (info->subCategories, "", PClassInfo2::kSubCategoriesSize );
		strncpy8 (info->category, kVstComponentControllerClass, PClassInfo::kCategorySize );
		memcpy(info->cid, &ctrlUUid/*(pluginInfo_.controllerId.toTUID())*/, sizeof(TUID));
		break;
	}

	info->cardinality = PClassInfo::kManyInstances;

	str8ToStr16 (info->sdkVersion, kVstVersionString, PClassInfo2::kVersionSize );
	str8ToStr16 (info->version, /*pluginInfo_.version_*/version.c_str(), PClassInfo2::kVersionSize );
	str8ToStr16 (info->vendor, vendorName_.c_str(), PClassInfo2::kVendorSize );
	str8ToStr16 (info->name, plugins[pluginIndex].name.c_str(), PClassInfo::kNameSize );

	return kResultOk;
}
	
/** Receives information about host*/
tresult MyVstPluginFactory::setHostContext (FUnknown* context)
{
	return kNotImplemented;
}

/** Create a new class instance. */
tresult MyVstPluginFactory::createInstance (FIDString cid, FIDString iid, void** obj)
{
	FUID classId = (FUID) *(TUID*)cid;
	FUID interfaceId = (FUID) *(TUID*)iid;

	/* testing
	{
		GetApp()->SeMessageBox( L"Yo", L"Yo", 0);
		char txt[65];
		classId.toString(txt);
		char txt2[65];
		interfaceId.toString(txt2);

		_RPT2(_CRT_WARN, "MyVstPluginFactory::createInstance(%s,%s)", txt, txt2);
	}
	*/

	initialize();

	FUnknown* instance{};

	for (auto& sem : plugins)
	{
		Steinberg::TUID procUUid{};
		Steinberg::TUID ctrlUUid{};
		textIdtoUuid(sem.id, false, procUUid);
		textIdtoUuid(sem.id, true, ctrlUUid);

		if (/*interfaceId == IComponent::iid ||*/ classId == Steinberg::FUID(procUUid))
		{
			auto i = new SeProcessor(sem);
			/* Now done by detecting MIDI input
					if( pluginInfo_.subCategories_.find( "Instrument" ) != std::string::npos )
					{
						i->setSynth();
					}
			*/
			i->setControllerClass(ctrlUUid); // associate with controller.
			instance = (IAudioProcessor*)i;
			break;
		}
		else if(classId == Steinberg::FUID(ctrlUUid))
		{
			instance = static_cast<Steinberg::Vst::IEditController*>(new VST3Controller(sem));
			break;
		}
	}

	if (instance)
	{
		if (instance->queryInterface (iid, obj) == kResultOk)
		{
			instance->release ();
			return kResultOk;
		}
		else
			instance->release ();
	}

	*obj = 0;
	return kNoInterface;
}

void MyVstPluginFactory::initialize()
{
	static auto callOnce = initializeFactory();
}

void MyVstPluginFactory::RegisterPin(
	tinyxml2::XMLElement* pin,
	std::vector<pinInfoSem>* pinlist,
	gmpi::api::PluginSubtype plugin_sub_type,
	int nextPinId
)
{
	assert(pin);
	int type_specific_flags = 0;

	if (plugin_sub_type != gmpi::api::PluginSubtype::Audio)
	{
//?		type_specific_flags = IO_UI_COMMUNICATION;
	}

	pinInfoSem pind{};

	//		_RPT1(_CRT_WARN, "%s\n", pins[i]->GetXML() );
	// id
//	int pin_id; // = XStr2Int( pin->Attribute("id") ));
	pind.id = nextPinId;
	pin->QueryIntAttribute("id", &(pind.id));
	// name
	pind.name = FixNullCharPtr(pin->Attribute("name"));

	// datatype
	std::string pin_datatype = FixNullCharPtr(pin->Attribute("datatype"));
	std::string pin_rate = FixNullCharPtr(pin->Attribute("rate"));

	// Datatype.
	int temp;
	if (XmlStringToDatatype(pin_datatype, temp))
	{
		assert(0 <= temp && temp < (int)gmpi::PinDatatype::Blob);
		pind.datatype = (gmpi::PinDatatype)temp;
/*
		if (pind.datatype == DT_CLASS) // e.g. "class:geometry"
		{
			if (pin_datatype.size() > 6)
				pind.classname = pin_datatype.substr(6);
		}
*/
	}
	else
	{
		//std::wostringstream oss;
		//oss << L"err. module XML file (" << Filename() << L"): parameter id " << pin_id << L" unknown datatype '" << Utf8ToWstring(pin_datatype) << L"'. Valid [float, int ,string, blob, midi ,bool ,enum ,double]";

#if defined( SE_EDIT_SUPPORT )
		Messagebox(oss);
#endif
	}

	// default
	pind.default_value = FixNullCharPtr(pin->Attribute("default"));

	if (pin_rate.compare("audio") == 0)
	{
		if (!pind.default_value.empty())
		{
			// multiply default by 10 (to Volts). DoubleToString() removes trilaing zeros.
			char* temp;	// convert string to SAMPLE
			pind.default_value = std::to_string(10.0f * (float)strtod(pind.default_value.c_str(), &temp));
		}

		pind.datatype = gmpi::PinDatatype::Audio;

		if (pin_datatype.compare("float") != 0)
		{
			//std::wostringstream oss;
			//oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" audio-rate not supported.";
			//Messagebox(oss);
		}
	}

	// direction
	const std::string pin_direction = FixNullCharPtr(pin->Attribute("direction"));

	if (pin_direction.compare("out") == 0)
	{
		pind.direction = gmpi::PinDirection::Out;

#if defined( SE_EDIT_SUPPORT )
		if (!pind.default_value.empty())
		{
			SafeMessagebox(0, (L"module data: default not supported on output pin"));
		}
#endif
	}
	else
	{
		pind.direction = gmpi::PinDirection::In;
	}

	// flags
	pind.flags = type_specific_flags;
#if 0 // TODO
	SetPinFlag("private", IO_HIDE_PIN, pin, pind.flags);
	SetPinFlag("autoRename", IO_RENAME, pin, pind.flags);
	SetPinFlag("isFilename", IO_FILENAME, pin, pind.flags);
	SetPinFlag("linearInput", IO_LINEAR_INPUT, pin, pind.flags);
	SetPinFlag("ignorePatchChange", IO_IGNORE_PATCH_CHANGE, pin, pind.flags);
	SetPinFlag("autoDuplicate", IO_AUTODUPLICATE, pin, pind.flags);
	SetPinFlag("isMinimised", IO_MINIMISED, pin, pind.flags);
	SetPinFlag("isPolyphonic", IO_PAR_POLYPHONIC, pin, pind.flags);
	SetPinFlag("autoConfigureParameter", IO_AUTOCONFIGURE_PARAMETER, pin, pind.flags);
	SetPinFlag("noAutomation", IO_PARAMETER_SCREEN_ONLY, pin, pind.flags);
	SetPinFlag("redrawOnChange", IO_REDRAW_ON_CHANGE, pin, pind.flags);
	SetPinFlag("isContainerIoPlug", IO_CONTAINER_PLUG, pin, pind.flags);
	SetPinFlag("isAdderInputPin", IO_ADDER, pin, pind.flags);

	std::wstring pin_automation = Utf8ToWstring(pin->Attribute("automation"));
#endif

#if defined( SE_EDIT_SUPPORT )
	if (pin->Attribute("isParameter") != 0)
	{
		SafeMessagebox(0, (L"'isParameter' not allowed in pin XML"));
	}
	if (!pin_automation.empty())
	{
		SafeMessagebox(0, (L"'automation' not allowed in pin XML"));
	}
#endif

	// parameter ID. Defaults to ssame as pin ID, but can be overridden.
	// Pins can be driven from patch-store.
	int parameterId = -1;
	pin->QueryIntAttribute("parameterId", &parameterId);

	//if (parameterId != -1)
	//{
	//	pind.flags |= (IO_PATCH_STORE | IO_HIDE_PIN);
	//}

	// host-connect
	pind.hostConnect = FixNullCharPtr(pin->Attribute("hostConnect"));

	// parameterField.
	if (!pind.hostConnect.empty() || parameterId != -1)
	{
#if defined( SE_TARGET_PLUGIN)
		// In exported VST3s, just using int in XML. Faster, more compact.
		int ft{ (int) gmpi::Field::MP_FT_VALUE };
		pin->QueryIntAttribute("parameterField", &ft);

		pind.parameterFieldType = (gmpi::Field)ft;
#else

		// see matching enum ParameterFieldType
		string parameterField(FixNullCharPtr(pin->Attribute("parameterField")));
		if (!XmlStringToParameterField(parameterField, parameterFieldId))
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" unknown Parameter Field ID.";

#if defined( SE_EDIT_SUPPORT )
			Messagebox(oss);
#endif
		}
#endif
	}
#if 0

	if (!pind.hostConnect.empty())
	{
		pind.flags |= IO_HOST_CONTROL | IO_HIDE_PIN;
		HostControls hostControlId = (HostControls)StringToHostControl(pind.hostConnect.c_str());
		if (hostControlId == HC_NONE)
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML: '" << pind.hostConnect << L"' unknown HOST CONTROL.";
			Messagebox(oss);
		}

		if (parameterFieldId == FT_VALUE)
		{
			int expectedDatatype = GetHostControlDatatype(hostControlId);
			if (expectedDatatype == DT_ENUM)
			{
				expectedDatatype = DT_INT;
			}

			if (parameterFieldId == FT_VALUE && expectedDatatype != -1 && expectedDatatype != pind.datatype)
			{
				std::wostringstream oss;
				oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" hostConnect wrong datatype. Expected: " << expectedDatatype;
				Messagebox(oss);
			}
		}

		if (pind.direction != DR_IN && (hostControlId < HC_USER_SHARED_PARAMETER_INT0 || hostControlId > HC_USER_SHARED_PARAMETER_INT4))
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" hostConnect pin wrong direction. Expected: direction=\"in\"";
			Messagebox(oss);
		}
	}
#endif
	// meta data
	pind.meta_data = FixNullCharPtr(pin->Attribute("metadata"));
	
	// notes
//	pind.notes = Utf8ToWstring(pin->Attribute("notes"));
#if 0
	if (pinlist->find(pin_id) != pinlist->end())
	{
		//std::wostringstream oss;
		//oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" used twice.";
		//Messagebox(oss);
	}
	else
#endif
	{
		//InterfaceObject* iob = new InterfaceObject(pin_id, pind);
		//// iob->setAutomation(controllerType);
		//iob->setParameterId(parameterId);
		//iob->setParameterFieldId(parameterFieldId);

		pind.parameterId = parameterId;

		// constraints
#if 0
		// Can't have isParameter on output Gui Pin.
		if (iob->GetDirection() == DR_OUT && iob->isUiPlug(0) && iob->isParameterPlug(0))
		{
			if (
				m_unique_id != L"Slider" && // exception for old crap.
				m_unique_id != L"List Entry" &&
				m_unique_id != L"Text Entry" &&
				m_group_name != L"Debug"
				)
			{
				std::wostringstream oss;
				oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" Can't have isParameter on output Gui Pin";
				Messagebox(oss);
			}
		}

		// Patch Store pins must be hidden.
		if (iob->isParameterPlug(0) && !iob->DisableIfNotConnected(0))
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" Patch-store input pins must have private=true.";
			Messagebox(oss);
		}

		// DSP parameters only support FT_VALUE
		if (iob->getParameterFieldId(0) != FT_VALUE && !iob->isUiPlug(0) && iob->isParameterPlug(0))
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" parameterField not supported on Audio (DSP) pins.";
			Messagebox(oss);
		}

		// parameter used by pin but not declared in "Parameters" section.
		if (iob->isParameterPlug(0) && m_parameters.find(iob->getParameterId(0)) == m_parameters.end())
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" parameter: " << iob->getParameterId(0) << L" not defined";
			Messagebox(oss);
		}

		// parameter used on autoduplicate pin.
		if (iob->isParameterPlug(0) && iob->autoDuplicate(0))
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L"  - can't have both 'autoDuplicate' and 'parameterId'";
			Messagebox(oss);
		}
#endif
		//std::pair< module_info_pins_t::iterator, bool > res = pinlist->insert(std::pair<int, InterfaceObject*>(pin_id, iob));
		//assert(res.second && "pin already registered");

		pinlist->push_back(pind);
	}
}

void MyVstPluginFactory::RegisterXml(const platform_string& pluginPath, const char* xml)
{
	tinyxml2::XMLDocument doc;
	doc.Parse(xml);

	if (doc.Error())
	{
		//std::wostringstream oss;
		//oss << L"Module XML Error: [" << full_path << L"]" << doc.ErrorDesc() << L"." << doc.Value();
		//SafeMessagebox(0, oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);
		return;
	}

	//	RegisterPluginsXml(&doc, pluginPath);
	auto pluginList = doc.FirstChildElement("PluginList");
	if (!pluginList)
		return;

	for (auto pluginE = pluginList->FirstChildElement("Plugin"); pluginE; pluginE = pluginE->NextSiblingElement())
	{
		const std::string id(pluginE->Attribute("id"));
		if (id.empty())
		{
			continue; // error
		}

		plugins.push_back({});

		auto& info = plugins.back();
		info.id = id;
		info.pluginPath = pluginPath;

		info.name = pluginE->Attribute("name");
		if (info.name.empty())
		{
			info.name = info.id;
		}

		if (auto s = pluginE->Attribute("vendor"); s)
			vendorName_ = s;
		else
			vendorName_ = "GMPI";

		// PARAMETERS
		if (auto parametersE = pluginE->FirstChildElement("Parameters"); parametersE)
		{
			int nextId = 0;
			for (auto paramE = parametersE->FirstChildElement("Parameter"); paramE; paramE = paramE->NextSiblingElement("Parameter"))
			{
				paramInfoSem param{};

				// I.D.
				paramE->QueryIntAttribute("id", &(param.id));
				// Datatype.
				std::string pin_datatype = FixNullCharPtr(paramE->Attribute("datatype"));
				// Name.
				param.name = FixNullCharPtr(paramE->Attribute("name"));
				// File extension or enum list.
				param.meta_data = FixNullCharPtr(paramE->Attribute("metadata"));
				// Default.
				param.default_value = FixNullCharPtr(paramE->Attribute("default"));

#if 0
				// Automation.
				int controllerType = ControllerType::None;
				if (!XmlStringToController(FixNullCharPtr(paramE->Attribute("automation")), controllerType))
				{
					//					std::wostringstream oss;
					//					oss << L"err. module XML file (" << Filename() << L"): pin id " << param.id << L" unknown automation type.";
					//#if defined( SE_EDIT_SUPPORT )
					//					Messagebox(oss);
					//#endif
					controllerType = controllerType << 24;
				}

				param.automation = controllerType;
#endif
				// Datatype.
				int temp;
				if (XmlStringToDatatype(pin_datatype, temp))//&& temp != DT_CLASS)
				{
					param.datatype = (gmpi::PinDatatype)temp;
				}
				else
				{
					//					std::wostringstream oss;
					//					oss << L"err. module XML file (" << Filename() << L"): parameter id " << param.id << L" unknown datatype '" << Utf8ToWstring(pin_datatype) << L"'. Valid [float, int ,string, blob, midi ,bool ,enum ,double]";
					//#if defined( SE_EDIT_SUPPORT )
					//					Messagebox(oss);
					//#endif
				}

				paramE->QueryBoolAttribute("private", &param.is_private);
#if 0
				SetPinFlag(("private"), IO_PAR_PRIVATE, paramE, param.flags);
				SetPinFlag(("ignorePatchChange"), IO_IGNORE_PATCH_CHANGE, paramE, param.flags);
				// isFilename don't appear to be used???? !!!!!
				SetPinFlag(("isFilename"), IO_FILENAME, paramE, param.flags);
				SetPinFlag(("isPolyphonic"), IO_PAR_POLYPHONIC, paramE, param.flags);
				SetPinFlag(("isPolyphonicGate"), IO_PAR_POLYPHONIC_GATE, paramE, param.flags);
				// exception. default for 'persistant' is true (set in param constructor).
				std::string persistantFlag = FixNullCharPtr(paramE->Attribute("persistant"));

				if (persistantFlag == "false")
				{
					CLEAR_BITS(param.flags, IO_PARAMETER_PERSISTANT);
			}
#endif

#if 0
				// can't handle poly parameters with 128 patches, bogs down dsp_patch_parameter_base::getQueMessage()
				// !! these are not really parameters anyhow, more like controlers (host genereated)
				if ((param.flags & IO_PAR_POLYPHONIC) && (param.flags & IO_IGNORE_PATCH_CHANGE) == 0)
				{
					std::wostringstream oss;
					oss << L"err. '" << GetName() << L"' module XML file: pin id " << param.id << L" - Polyphonic parameters need ignorePatchChange=true";
					Messagebox(oss);
				}
				if (m_parameters.find(param.id) != m_parameters.end())
				{
#if defined( SE_EDIT_SUPPORT )
					std::wostringstream oss;
					oss << L"err. module XML file: parameter id " << param.id << L" used twice.";
					Messagebox(oss);
#endif
				}
				else
				{
					//bool res = m_parameters.insert(std::pair<int, parameter_description*>(param.id, pind)).second;
					//assert(res && "parameter already registered");
		}
#endif
				info.parameters.push_back(param);

				++nextId;
			}
		}

		gmpi::api::PluginSubtype scanTypes[] = { gmpi::api::PluginSubtype::Audio, gmpi::api::PluginSubtype::Editor, gmpi::api::PluginSubtype::Controller };
		std::vector<pinInfoSem>* pinList = {};
		for (auto sub_type : scanTypes)
		{
			const char* sub_type_name = {};

			switch (sub_type)
			{
			case gmpi::api::PluginSubtype::Audio:
				sub_type_name = "Audio";
				pinList = &info.dspPins;
				break;

			case gmpi::api::PluginSubtype::Editor:
				sub_type_name = "GUI";
				pinList = &info.guiPins;
				break;

			case gmpi::api::PluginSubtype::Controller:
				sub_type_name = "Controller";
				pinList = nullptr;
				break;
			}

			auto classE = pluginE->FirstChildElement(sub_type_name);
			if (!classE)
				continue;

			if (sub_type == gmpi::api::PluginSubtype::Audio)
			{
				// plugin_class->ToElement()->QueryIntAttribute("latency", &latency);
			}

			if (pinList)
			{
				int nextPinId = 0;
				for (auto pinE = classE->FirstChildElement("Pin"); pinE; pinE = pinE->NextSiblingElement("Pin"))
				{
					RegisterPin(pinE, pinList, sub_type, nextPinId);
					++nextPinId;
				}
			}
		}
	}
}

typedef gmpi::ReturnCode (*MP_DllEntry)(void**);

bool MyVstPluginFactory::initializeFactory()
{
	platform_string pluginPath;
#if 0
	// load SEM dynamic.
	const auto semFolderSearch = BundleInfo::instance()->getSemFolder() + L"/*.gmpi";

	FileFinder it(semFolderSearch.c_str());
	for (; !it.done(); ++it)
	{
		if (!(*it).isFolder)
		{
			pluginPath = (*it).fullPath;
			break;
		}
	}

	if (pluginPath.empty())
	{
		return true;
	}

	gmpi_dynamic_linking::DLL_HANDLE hinstLib;
	gmpi_dynamic_linking::MP_DllLoad(&hinstLib, pluginPath.c_str());
#else
	gmpi_dynamic_linking::DLL_HANDLE hinstLib{};
	gmpi_dynamic_linking::MP_GetDllHandle(&hinstLib);

#endif
	if (!hinstLib)
	{
		return true;
	}

#if _WIN32
	// Use XML data to get list of plugins
	auto hInst = (HINSTANCE)hinstLib;
	HRSRC hRsrc = ::FindResource(hInst,
		MAKEINTRESOURCE(1), // ID
		L"GMPXML");			// type GMPI XML

	if (hRsrc)
	{
		const BYTE* lpRsrc = (BYTE*)LoadResource(hInst, hRsrc);

		if (lpRsrc)
		{
			const BYTE* locked_mem = (BYTE*)LockResource((HANDLE)lpRsrc);
			const std::string xmlFile((char*)locked_mem);

			// cleanup
			UnlockResource((HANDLE)lpRsrc);
			FreeResource((HANDLE)lpRsrc);
			gmpi_dynamic_linking::MP_DllUnload(hinstLib);

			RegisterXml(pluginPath, xmlFile.c_str());

			return true;
		}
	}
#else
  // not needed for built-in XML  #error implement this for mac
#endif

	// set some fallbacks
	vendorName_ = "SynthEdit";
//	pluginInfo_.name_ = "SEM Wrapper";
	// TODO derive from SEM id
//	pluginInfo_.processorId.fromRegistryString("{8A389500-D21D-45B6-9FA7-F61DEFA68328}");
//	pluginInfo_.controllerId.fromRegistryString("{D3047E63-2F3F-43A8-96AC-68D068F56106}");

	// Shell plugins
	// GMPI & sem V3 export function
	// ReturnCode MP_GetFactory( void** returnInterface )
    /*
     MP_DllEntry dll_entry_point;

	const char* gmpi_dll_entrypoint_name = "MP_GetFactory";
	auto fail = gmpi_dynamic_linking::MP_DllSymbol(hinstLib, gmpi_dll_entrypoint_name, (void**)&dll_entry_point);

	if (fail) // GMPI/SDK3 Plugin
	{
		gmpi_dynamic_linking::MP_DllUnload(hinstLib);
		return false;
	}
*/
	{ // restrict scope of 'vst_factory' and 'gmpi_factory' so smart pointers RIAA before dll is unloaded

		// Instantiate factory and query sub-plugins.
//		gmpi::shared_ptr<gmpi::IMpShellFactory> vst_factory;
		gmpi::shared_ptr<gmpi::api::IPluginFactory> gmpi_factory;
		{
			gmpi::shared_ptr<gmpi::api::IUnknown> com_object;
 //           auto r = dll_entry_point(com_object.asIMpUnknownPtr());
            auto r = MP_GetFactory(com_object.asIMpUnknownPtr());

//			r = com_object->queryInterface(gmpi::MP_IID_SHELLFACTORY, vst_factory.asIMpUnknownPtr());
			r = com_object->queryInterface(&gmpi::api::IPluginFactory::guid, gmpi_factory.asIMpUnknownPtr());
		}

		if (/*!vst_factory &&*/ !gmpi_factory)
		{
			//std::wostringstream oss;
			//oss << L"Module missing XML resource, and has no factory\n" << full_path;
			//SafeMessagebox(0, oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);

			gmpi_dynamic_linking::MP_DllUnload(hinstLib);
			return false;
		}

		int index = 0;
		while (gmpi_factory)
		{
			gmpi::ReturnString s;
			// have to cast GMPI 2 types to GMPI 1 types
			const auto r = gmpi_factory->getPluginInformation(index++, &s); // FULL XML

			if (r != gmpi::ReturnCode::Ok)
				break;

			RegisterXml(pluginPath, s.c_str());

			//ModuleFactory()->RegisterExternalPluginsXml(&doc2, full_path, group_name, scanVstsOnly);
		}
	}

	return true;
}

//------------------------------------------------------------------------
//IMPLEMENT_REFCOUNT (MyVstPluginFactory)

//------------------------------------------------------------------------
tresult PLUGIN_API MyVstPluginFactory::queryInterface (FIDString iid, void** obj)
{
	QUERY_INTERFACE (iid, obj, IPluginFactory::iid, IPluginFactory)
	QUERY_INTERFACE (iid, obj, IPluginFactory2::iid, IPluginFactory2)
	QUERY_INTERFACE (iid, obj, IPluginFactory3::iid, IPluginFactory3)
	QUERY_INTERFACE (iid, obj, FUnknown::iid, FUnknown)
	*obj = 0;
	return kNoInterface;
}

#if 0 // ?? !defined( _WIN32 ) // Mac Only.
std::wstring MyVstPluginFactory::getSemFolder()
{
    std::string result;

    CFBundleRef br = CreatePluginBundleRef();
    
	if ( br ) // getBundleRef ())
	{
        CFURLRef url2 = CFBundleCopyBuiltInPlugInsURL (br);
        char filePath2[PATH_MAX] = "";
        if (url2)
        {
            if (CFURLGetFileSystemRepresentation (url2, true, (UInt8*)filePath2, PATH_MAX))
            {
                result = filePath2;
            }
        }
        ReleasePluginBundleRef(br);
    }
    
    return Utf8ToWstring(result.c_str());
}            
#endif


bool MyVstPluginFactory::GetOutputsAsStereoPairs()
{
	return true; // for now.  pluginInfo_.outputsAsStereoPairs;
}

std::string MyVstPluginFactory::getVendorName()
{
	return vendorName_;
}

//std::wstring MyVstPluginFactory::GetOutputsName(int index)
//{
//	it_enum_list it( Utf8ToWstring( pluginInfo_.outputNames ));
//
//	it.FindIndex(index);
//	if( !it.IsDone() )
//	{
//		return ( *it )->text;
//	}
//
//	return L"AudioOutput";
//}

