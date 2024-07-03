#include "base/source/fstring.h"
#include "base/source/updatehandler.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstpresetfile.h"
#include "public.sdk/source/common/memorystream.h"
#include "adelaycontroller.h"
#include "MyVstPluginFactory.h"
#include "adelayprocessor.h"
#include "se_datatypes.h" // kill this
#include "../Shared/RawConversions.h"
#include "../Shared/BundleInfo.h"
#include "unicode_conversion.h"
#include "GmpiApiEditor.h"
#include "it_enum_list.h"
//#include "MyVstPluginFactory.h"


#ifdef _WIN32
#include "SEVSTGUIEditorWin.h"
//#include "../../se_vst3/source/MyVstPluginFactory.h"
//#include "pluginterfaces/base/funknown.h"
//#include "../shared/unicode _conversion.h"
#else
#include "SEVSTGUIEditorMac.h"
#endif

extern "C"
gmpi::ReturnCode MP_GetFactory( void** returnInterface );

/*
#include "midi_defs.h"
#include "conversion.h"
#include "it_enum_list.h"
#include "IPluginGui.h"
#include "HostControls.h"
#include "../Shared/jsoncpp/json/json.h"
#include "modules/shared/FileFinder.h"
#include "UgDatabase.h"
#include "../se_sdk3_hosting/GuiPatchAutomator3.h"
#include "tinyxml/tinyxml.h" // anoyingly defines DEBUG as blank which messes with vstgui. put last.
#include "../tinyXml2/tinyxml2.h"
#include "VstPreset.h"
#include "Vst2Preset.h"

#ifdef _WIN32
#include "SEVSTGUIEditorWin.h"
#include "../../se_vst3/source/MyVstPluginFactory.h"
#include "pluginterfaces/base/funknown.h"
#include "../shared/unicode _conversion.h"
#else
#include "SEVSTGUIEditorMac.h"
#endif
#include "AuPreset.h"
#include "mfc_emulation.h"

using namespace std;
using namespace tinyxml2;
*/

typedef gmpi::ReturnCode(*MP_DllEntry)(void**);

using namespace JmUnicodeConversions;

#if 0
void SafeMessagebox(
	void* hWnd,
	const wchar_t* lpText,
	const wchar_t* lpCaption = L"",
	int uType = 0
)
{
	_RPTW1(0, L"%s\n", lpText);
}
#endif

MpParameterVst3::MpParameterVst3(Steinberg::Vst::VST3Controller* controller, /*int strictIndex, */int ParameterTag, bool isInverted) :
	MpParameter_native(controller),
	vst3Controller(controller),
	isInverted_(isInverted),
	hostTag(ParameterTag)
{
}

void MpParameterVst3::updateProcessor(gmpi::Field fieldId, int32_t voice)
{
	switch (fieldId)
	{
		/* double up
	case gmpi::Field::MP_FT_GRAB:
		controller_->ParamGrabbed(this, voice);
		break;
		*/

	case gmpi::Field::MP_FT_VALUE:
	case gmpi::Field::MP_FT_NORMALIZED:
		vst3Controller->ParamToProcessorViaHost(this, voice);
		break;
	}
}


namespace Steinberg {
namespace Vst {

VST3Controller::VST3Controller(pluginInfoSem& pinfo) :
	isInitialised(false)
	, isConnected(false)
	, info(pinfo)
{
// using tinxml 1?	TiXmlBase::SetCondenseWhiteSpace(false); // ensure text parameters preserve multiple spaces. e.g. "A     B" (else it collapses to "A B")

	// Scan all presets for preset-browser.
	ScanPresets();
}

VST3Controller::~VST3Controller()
{
	StopTimer();
}

tresult PLUGIN_API VST3Controller::connect(IConnectionPoint* other)
{
//	_RPT0(_CRT_WARN, "ADelayController::connect\n");
	auto r = EditController::connect(other);

	isConnected = true;

	// Can only init controllers after both VST controller initialised AND controller is connected to processor.
	// So VST2 wrapper aeffect pointer makes it to Processor.
	if(isConnected && isInitialised)
		initSemControllers();

	return r;
}

tresult PLUGIN_API VST3Controller::notify( IMessage* message )
{
	// WARNING: CALLED FROM PROCESS THREAD (In Ableton Live).
	if( !message )
		return kInvalidArgument;

	if( !strcmp( message->getMessageID(), "BinaryMessage" ) )
	{
		const void* data;
		uint32 size;
		if( message->getAttributes()->getBinary( "MyData", data, size ) == kResultOk )
		{
			message_que_dsp_to_ui.pushString(size, (unsigned char*) data);
			message_que_dsp_to_ui.Send();
			return kResultOk;
		}
	}

	return EditController::notify( message );
}

bool VST3Controller::sendMessageToProcessor(const void* data, int size)
{
	auto message = allocateMessage();
	if (message)
	{
		FReleaser msgReleaser(message);
		message->setMessageID("BinaryMessage");

		message->getAttributes()->setBinary("MyData", data, size);
		sendMessage(message);
	}

	return true;
}

void VST3Controller::ParamGrabbed(MpParameter_native* param)
{
	auto paramID = param->getNativeTag();

	if (param->isGrabbed())
	{
//		_RPT0(0, "DAW GRAB\n");
		beginEdit(paramID);
	}
	else
	{
//		_RPT0(0, "DAW UN-GRAB\n");
		endEdit(paramID);
	}
}

void VST3Controller::ParamToProcessorViaHost(MpParameterVst3* param, int32_t voice)
{
	const auto paramID = param->getNativeTag();

	// Usually parameter will have sent beginEdit() already (if it has mouse-down connected properly, else fake it.
	if (!param->isGrabbed())
		beginEdit(paramID);

 //   _RPT2(0, "param[%d] %f => DAW\n", paramID, param->getNormalized());
	performEdit(paramID, param->convertNormalized(param->getNormalized())); // Send the value to DSP.

	if (!param->isGrabbed())
		endEdit(paramID);
}

void VST3Controller::ResetProcessor()
{
	// Currently called when polyphony etc changes, VST2 wrapper ignores this completely, at least in Live.
//	componentHandler->restartComponent(kLatencyChanged); // or kIoChanged might be less overhead for DAW
}

enum class ElatencyContraintType
{
	Off, Constrained, Full
};

struct pluginInformation
{
	int32_t pluginId = -1;
	std::string manufacturerId;
	int32_t inputCount;
	int32_t outputCount;
	int32_t midiInputCount;
	ElatencyContraintType latencyConstraint;
	std::string pluginName;
	std::string vendorName;
	std::string subCategories;
	bool outputsAsStereoPairs;
	bool emulateIgnorePC = {};
	bool monoUseOk;
	bool vst3Emulate16ChanCcs;
	std::vector<std::string> outputNames;
};

void VST3Controller::setPinFromUi(int32_t pinId, int32_t voice, int32_t size, const void* data)
{
	for (auto& pin : info.guiPins)
	{
		if (pin.id == pinId && /*pin.direction == gmpi::PinDirection::In && pin.datatype == gmpi::PinDatatype::Float32 &&*/ pin.parameterId != -1)
		{
			for (auto& param : info.parameters)
			{
				if (param.id == pin.parameterId)
				{
					auto param = tagToParameter[pin.parameterId];
					setParameterValue({ data, static_cast<size_t>(size)}, param->parameterHandle_, gmpi::Field::MP_FT_VALUE, voice); // TODO figure out correct fieldtype
					break;
				}
			}
			break;
		}
	}
}

// send initial value of all parameters to GUI
void VST3Controller::initUi(gmpi::api::IParameterObserver* gui)
{
	for (auto& it : tagToParameter)
	{
		initializeGui(gui, it.second->parameterHandle_, gmpi::Field::MP_FT_VALUE);
	}
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::initialize (FUnknown* context)
{
//	_RPT0(_CRT_WARN, "ADelayController::initialize\n");

	UpdateHandler::instance();

	tresult result = EditController::initialize (context);
	if (result == kResultTrue)
	{
		// Modules database, needed here to load any controllers. e.g. VST2 child plugins.
		{
//?			ModuleFactory()->RegisterExternalPluginsXmlOnce(nullptr);
		}

		MpController::Initialize();

		supportedChannels = countPins(info, gmpi::PinDirection::In, gmpi::PinDatatype::Midi) == 0 ? 0 : 16;

		// Parameters
		{
			int hostParameterIndex = 0;
			int ParameterHandle = 0;

			for (auto& i : ParameterHandleIndex)
			{
				ParameterHandle = (std::max)(ParameterHandle, i.first + 1);
			}

			for (auto& param : info.parameters)
			{
				bool isPrivate =
					param.is_private ||
					param.datatype == gmpi::PinDatatype::String ||
					param.datatype == gmpi::PinDatatype::Blob;

				float pminimum = 0.0f;
				float pmaximum = 1.0f;

				if (!param.meta_data.empty())
				{
					it_enum_list it(::Utf8ToWstring(param.meta_data));

					pminimum = it.RangeLo();
					pmaximum = it.RangeHi();
				}

				MpParameter_base* seParameter = {};
				if (isPrivate)
				{
					auto param = new MpParameter_private(this);
					seParameter = param;
//					param->isPolyphonic_ = isPolyphonic_;
				}
				else
				{
//					assert(ParameterTag >= 0);
					seParameter = makeNativeParameter(hostParameterIndex++, pminimum > pmaximum);
				}

				seParameter->hostControl_ = -1; // TODO hostControl;
				seParameter->minimum = pminimum;
				seParameter->maximum = pmaximum;
				seParameter->parameterHandle_ = ParameterHandle;
				seParameter->datatype_ = param.datatype;
				seParameter->moduleHandle_ = 0;
				seParameter->moduleParamId_ = param.id;
				seParameter->stateful_ = true; // stateful_;
				seParameter->name_ = convert.from_bytes(param.name);
				seParameter->enumList_ = convert.from_bytes(param.meta_data); // enumList_;
				seParameter->ignorePc_ = false; // ignorePc != 0;

				// add one patch value
				seParameter->rawValues_.push_back(ParseToRaw(seParameter->datatype_, param.default_value));

				parameters_.push_back(std::unique_ptr<MpParameter>(seParameter));
				ParameterHandleIndex.insert(std::make_pair(ParameterHandle, seParameter));
				moduleParameterIndex.insert(std::make_pair(std::make_pair(seParameter->moduleHandle_, seParameter->moduleParamId_), ParameterHandle));

				// Ensure host queries return correct value.
				seParameter->upDateImmediateValue();

				++ParameterHandle;
			}
		}

//		const auto supportedChannels = info.midiInputCount ? 16 : 0;
		const auto supportAllCC = true; // info.vst3Emulate16ChanCcs;

		// MIDI CC SUPPORT
		wchar_t ccName[] = L"MIDI CC     ";
		for (int chan = 0; chan < supportedChannels; ++chan)
		{
			for (int cc = 0; cc < numMidiControllers; ++cc)
			{
				if (chan == 0 || supportAllCC || cc > 127 || cc == 74) // Channel Presure, Pitch Bend and Brightness
				{
					const ParamID ParameterId = MidiControllersParameterId + chan * numMidiControllers + cc;
					swprintf(ccName + 9, std::size(ccName), L"%3d", cc);

					auto param = makeNativeParameter(ParameterId, false);
					param->datatype_ = gmpi::PinDatatype::Float32;
					param->name_ = ccName;
					param->minimum = 0;
					param->maximum = 1;
					param->hostControl_ = -1;
					param->isPolyphonic_ = false;
					param->parameterHandle_ = -1;
					param->moduleHandle_ = -1;
					param->moduleParamId_ = -1;
					param->stateful_ = false;

					auto initialVal(ParseToRaw(param->datatype_, "0"));
					param->rawValues_.push_back(initialVal);				// preset 0/

					parameters_.push_back(std::unique_ptr<MpParameter>(param));
				}
			}
		}
	}

	isInitialised = true;

	// Can only init controllers after both VST controller initialised AND controller is connected to processor.
	// So VST2 wrapper aeffect pointer makes it to Processor.
	if (isConnected && isInitialised)
	initSemControllers();

	StartTimer(timerPeriodMs);

	return kResultTrue;
}

// Parameter updated from DAW.
void VST3Controller::update( FUnknown* changedUnknown, int32 message )
{
	EditController::update( changedUnknown, message );

	if( message == IDependent::kChanged )
	{
	assert(false);
		auto parameter = dynamic_cast<Parameter*>( changedUnknown );
		if( parameter )
		{
			/*
			auto& it = vst3IndexToParameter.find(parameter->getInfo().id);
			if (it != vst3IndexToParameter.end())
			{
				const int voiceId = 0;
				update Guis((*it).second, voiceId);
			}
			*/
		}
	}
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::getMidiControllerAssignment (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& tag/*out*/)
{
	if (busIndex == 0)
	{
		const int possibleTag = MidiControllersParameterId + channel * numMidiControllers + midiControllerNumber;
		if (tagToParameter.find(possibleTag) != tagToParameter.end())
		{
			tag = possibleTag;
			return kResultTrue;
		}
	}

	return kResultFalse;
}

IPlugView* PLUGIN_API VST3Controller::createView (FIDString name)
{
	if (ConstString (name) == ViewType::kEditor)
	{
		auto load_filename = info.pluginPath;
#if 0
		gmpi_dynamic_linking::DLL_HANDLE plugin_dllHandle = {};
//		if (!plugin_dllHandle)
//		{
			if (load_filename.empty()) // plugin is statically linked.
			{
				// no need to load DLL, it's already linked.
				gmpi_dynamic_linking::MP_GetDllHandle(&plugin_dllHandle);
			}
			else
			{
#if defined( _WIN32)
				/* TODO
				plugin_dllHandle_to_unload = {};

				// Load the DLL.
				if (gmpi_dynamic_linking::MP_DllLoad(&plugin_dllHandle, load_filename.c_str()))
				{
					plugin_dllHandle_to_unload = plugin_dllHandle;
					assert(false);
					// TODO.
					//// load failed, try it as a bundle.
					//const auto bundleFilepath = load_filename + L"/Contents/x86_64-win/" + filename;
					//gmpi_dynamic_linking::MP_DllLoad(&dllHandle, bundleFilepath.c_str());
				}
				*/

#else
#if 0
			// int32_t r = MP_DllLoad( &dllHandle, load_filename.c_str() );

			// Create a path to the bundle
			CFStringRef pluginPathStringRef = CFStringCreateWithCString(NULL,
				WStringToUtf8(load_filename).c_str(), kCFStringEncodingASCII);

			CFURLRef bundleUrl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
				pluginPathStringRef, kCFURLPOSIXPathStyle, true);
			if (bundleUrl == NULL) {
				printf("Couldn't make URL reference for plugin\n");
				return;
			}

			// Open the bundle
			dllHandle = (DLL_HANDLE)CFBundleCreate(kCFAllocatorDefault, bundleUrl);
			if (dllHandle == 0) {
				printf("Couldn't create bundle reference\n");
				CFRelease(pluginPathStringRef);
				CFRelease(bundleUrl);
				return;
			}
#endif
#endif
            }

			// Factory
			MP_DllEntry dll_entry_point = {};
//#ifdef _WIN32
			const auto fail = gmpi_dynamic_linking::MP_DllSymbol(plugin_dllHandle, "MP_GetFactory", (void**)&dll_entry_point);
//#else
//			dll_entry_point = (gmpi::MP_DllEntry)CFBundleGetFunctionPointerForName((CFBundleRef)plugin_dllHandle, CFSTR("MP_GetFactory"));
//#endif

			if (!dll_entry_point)
			{
				return {};
			}
#endif
			gmpi::shared_ptr<gmpi::api::IUnknown> factoryBase;
			//auto r = dll_entry_point(factoryBase.asIMpUnknownPtr());
            auto r = MP_GetFactory(factoryBase.asIMpUnknownPtr());

			gmpi::shared_ptr<gmpi::api::IPluginFactory> factory;
			auto r2 = factoryBase->queryInterface(&gmpi::api::IPluginFactory::guid, factory.asIMpUnknownPtr());

			if (!factory || r != gmpi::ReturnCode::Ok)
			{
				return {};
			}

			gmpi::shared_ptr<gmpi::api::IUnknown> pluginUnknown;
			r2 = factory->createInstance(info.id.c_str(), gmpi::api::PluginSubtype::Editor, pluginUnknown.asIMpUnknownPtr());
			if (!pluginUnknown || r != gmpi::ReturnCode::Ok)
			{
				return {};
			}

			if (auto editor = pluginUnknown.As<gmpi::api::IEditor>(); editor)
			{
				// TODO currently in pixels, should be DIPs???
				int width{ 200 };
				int height{ 200 };
#ifdef _WIN32
				return new SEVSTGUIEditorWin(info, editor, this, width, height);
#else
				return new SEVSTGUIEditorMac(info, editor, this, width, height);
#endif
			}



#if 0 // TODO
		// somewhat inefficient to parse entire JSON file just for GUI size
		// would be nice to pass ownership of document to presenter to save it doing the same all over again.
		Json::Value document_json;
		{
			Json::Reader reader;
			reader.parse(BundleInfo::instance()->getResource("gui.se.json"), document_json);
		}

		auto& gui_json = document_json["gui"];

		int width = gui_json["width"].asInt();
		int height = gui_json["height"].asInt();

#ifdef _WIN32
		// DPI of system. only a GUESS at this point of DPI we will be using. (until we know DAW window handle).
		// But we need a rough idea of plugin window size to allow Cubase to arrange menu bar intelligently. Else we get the blank area at top of plugin.
		HDC hdc = ::GetDC(NULL);
		width = (width * GetDeviceCaps(hdc, LOGPIXELSX)) / 96;
		height = (height * GetDeviceCaps(hdc, LOGPIXELSY)) / 96;
		::ReleaseDC(NULL, hdc);
#endif

		//ViewRect estimatedViewRect;
		//estimatedViewRect.top = estimatedViewRect.left = 0;
		//estimatedViewRect.bottom = height;
		//estimatedViewRect.right = width;
#endif

	}
	return {};
}

// Preset Loaded.
tresult PLUGIN_API VST3Controller::setComponentState (IBStream* state)
{
	int32 bytesRead;
	int32 chunkSize = 0;
	std::string chunk;
	state->read( &chunkSize, sizeof(chunkSize), &bytesRead );
	chunk.resize(chunkSize);
	state->read((void*) chunk.data(), chunkSize, &bytesRead);

	DawPreset preset(parametersInfo, chunk);
	setPreset(&preset);

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::queryInterface (const char* iid, void** obj)
{
	QUERY_INTERFACE(iid, obj, IMidiMapping::iid, IMidiMapping)
	QUERY_INTERFACE(iid, obj, INoteExpressionController::iid, INoteExpressionController)
	QUERY_INTERFACE(iid, obj, INoteExpressionPhysicalUIMapping::iid, INoteExpressionPhysicalUIMapping)

	return EditController::queryInterface (iid, obj);
}

tresult VST3Controller::setParamNormalized( ParamID tag, ParamValue value )
{
//	_RPT2(_CRT_WARN, "setParamNormalized(%d, %f)\n", tag, value);

	if (auto p = getDawParameter(tag); p)
	{
		const auto n = p->convertNormalized(value);
		p->MpParameter_base::setParameterRaw(gmpi::Field::MP_FT_NORMALIZED, sizeof(n), &n);
	}

	return kResultTrue;
}

void VST3Controller::OnLatencyChanged()
{
	getComponentHandler()->restartComponent(kLatencyChanged);
//	_RPT0(_CRT_WARN, "restartComponent(kLatencyChanged)\n");
}

tresult VST3Controller::getParameterInfo(int32 paramIndex, ParameterInfo& info)
{
	auto p = nativeGetParameterByIndex(paramIndex);

	if (!p)
		return kInvalidArgument;

	info.flags = Steinberg::Vst::ParameterInfo::kCanAutomate;
	info.defaultNormalizedValue = 0.0;

	auto temp = ToUtf16(p->name_);
	
	_tstrncpy(info.shortTitle, (const TChar*)temp.c_str(), static_cast<Steinberg::uint32>(std::size(info.shortTitle)));
	_tstrncpy(info.title, (const TChar*)temp.c_str(), static_cast<Steinberg::uint32>(std::size(info.title)));

	info.id = p->getNativeTag();
	info.unitId = kRootUnitId;
    info.stepCount = 0;
	info.units[0] = 0;

	if ((p->datatype_ == gmpi::PinDatatype::Int32 || p->datatype_ == gmpi::PinDatatype::Int64) && !p->enumList_.empty())
	{
		info.flags |= Steinberg::Vst::ParameterInfo::kIsList;
		it_enum_list it(p->enumList_);
		info.stepCount = (std::max)(0, it.size() - 1);
	}

	// Support for VSTs special bypass parameter. Make a bool param called "BYPASS" 
	if (p->datatype_ == gmpi::PinDatatype::Bool && p->name_ == L"BYPASS")
	{
		info.flags |= Steinberg::Vst::ParameterInfo::kIsBypass;
	}

	return kResultOk;
}

tresult PLUGIN_API VST3Controller::getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string)
{
	if (auto p = getDawParameter(tag); p)
	{
		const auto s_wide = p->normalisedToString(p->convertNormalized(valueNormalized));
		const auto s_UTF16 = ToUtf16(s_wide);
		strncpy16(string, (const TChar*) s_UTF16.c_str(), 128);

		return kResultOk;
	}

	return kInvalidArgument;
}

std::string VST3Controller::loadNativePreset(std::wstring sourceFilename)
{
#if 0 // TODO
	auto filetype = GetExtension(sourceFilename);

	if (filetype == L"vstpreset")
	{
		return VstPresetUtil::ReadPreset(sourceFilename);
	}

	if (filetype == L"aupreset")
	{
		return AuPresetUtil::ReadPreset(sourceFilename);
	}

	if (filetype == L"fxp")
	{
		auto factory = MyVstPluginFactory::GetInstance();
		const int pluginIndex = 0;
		const auto xml = Vst2PresetUtil::ReadPreset(ToPlatformString(sourceFilename), factory->getVst2Id(pluginIndex), factory->getVst2Id64(pluginIndex));

		if(!xml.empty())
		{
			return xml;
		}

		// Prior to v1.4.518 - 2020-08-04, Preset Browser saves VST3 format instead of VST2 format. Fallback to VST3.
		return VstPresetUtil::ReadPreset(sourceFilename);
	}
#endif
	return {};
}

std::vector< MpController::presetInfo > VST3Controller::scanFactoryPresets()
{
#if 0 // todo
	platform_string factoryPresetsFolder(_T("vst2FactoryPresets/"));
	auto factoryPresetFolder = ToPlatformString(BundleInfo::instance()->getImbeddedFileFolder());
	auto fullpath = combinePathAndFile(factoryPresetFolder, factoryPresetsFolder);

	if (fullpath.empty())
	{
		return {};
	}

	return scanPresetFolder(fullpath, _T("xmlpreset"));
#endif
	return {};
}

void VST3Controller::loadFactoryPreset(int index, bool fromDaw)
{
#if 0 // TODO
	platform_string vst2FactoryPresetsFolder(_T("vst2FactoryPresets/"));
	auto PresetFolder = ToPlatformString(BundleInfo::instance()->getImbeddedFileFolder());
	PresetFolder = combinePathAndFile(PresetFolder, vst2FactoryPresetsFolder);

	auto fullFilePath = PresetFolder + ToPlatformString(presets[index].filename);
	auto filenameUtf8 = ToUtf8String(fullFilePath);
	ImportPresetXml(filenameUtf8.c_str());
#endif
}

void VST3Controller::setPresetFromSelf(DawPreset const* preset)
{
	// since there is no explicit sharing between controller and processor, we need to send entire preset in one hit via queue
	const auto xml = preset->toString(BundleInfo::instance()->getPluginId());
	setPresetXmlFromSelf(xml);
}

void VST3Controller::setPresetXmlFromSelf(const std::string& xml)
{
	// send to processor
	auto message = allocateMessage();
	if (message)
	{
		FReleaser msgReleaser(message);
		message->setMessageID("BinaryMessage");

		message->getAttributes()->setBinary("Preset", xml.data(), xml.size());
		sendMessage(message);
	}

	// send to controller
	DawPreset preset(parametersInfo, xml);
	setPreset(&preset);

#if 0
	// since there is not explicit sharing between controller and processor, we need to send entire preset in one hit via queue
	
	const auto freespace = getQueueToDsp()->freeSpace();
	const uint32_t messageSize = static_cast<uint32_t>(sizeof(int32_t) + xml.size());

	if (freespace < messageSize)
	{
		assert(false); // Preset too large for message queue
		return;
	}

	my_msg_que_output_stream s(getQueueToDsp(), UniqueSnowflake::APPLICATION, "PROG"); // Program Change

	s << messageSize;
	s << xml;

	s.Send();
#endif
}

platform_string VST3Controller::calcFactoryPresetFolder()
{
	// TODO
	return {};
}

std::string VST3Controller::getFactoryPresetXml(std::string filename)
{
	auto PresetFolder = calcFactoryPresetFolder();
	auto fullFilePath = PresetFolder + ToPlatformString(filename);
	return loadNativePreset(toWstring(fullFilePath));
}

void VST3Controller::saveNativePreset(const char* filename, const std::string& presetName, const std::string& xml)
{
#if 0
	const auto filetype = GetExtension(std::string(filename));

	// VST2 preset format.
	if(filetype == "fxp")
	{
		auto factory = MyVstPluginFactory::GetInstance();
		Vst2PresetUtil::WritePreset(ToPlatformString(filename), factory->getVst2Id64(0), presetName, xml);
		return;
	}

	// VST3 preset format.
	{
		// Add Company name.
		auto factory = MyVstPluginFactory::GetInstance();
		auto processorId = factory->getPluginInfo().processorId;

		std::string categoryName; // TODO !!!
		VstPresetUtil::WritePreset(JmUnicodeConversions::Utf8ToWstring(filename), categoryName, factory->getVendorName(), factory->getProductName(), &processorId, xml);
	}
#endif
}

bool VST3Controller::OnTimer()
{
    if (!queueToDsp_.empty())
	{
		sendMessageToProcessor(queueToDsp_.data(), queueToDsp_.size());
		queueToDsp_.clear();
	}

	return MpController::OnTimer();
}

int32 VST3Controller::getNoteExpressionCount(int32 busIndex, int16 channel)
{
    // we accept only the first bus and 1 channel
    if (busIndex != 0 || channel != 0)
        return 0;

	return 4;
}

tresult VST3Controller::getNoteExpressionInfo (int32 busIndex, int16 channel, int32 noteExpressionIndex, NoteExpressionTypeInfo& info)
{
    // we accept only the first bus and 1 channel
    if (busIndex != 0 || channel != 0 || noteExpressionIndex > 3)
        return kResultFalse;

	memset (&info, 0, sizeof (NoteExpressionTypeInfo));
 
	// might be better to support two generic types than volume and pan.
	NoteExpressionTypeID typeIds[] =
	{
		kVolumeTypeID,
		kPanTypeID,
		kTuningTypeID,
		kBrightnessTypeID,
	};

	info.typeId = typeIds[noteExpressionIndex];
	info.valueDesc.minimum = 0.0;
	info.valueDesc.maximum = 1.0;
	info.valueDesc.defaultValue = 0.5;
	info.valueDesc.stepCount = 0; // we want continuous (no step)
	info.unitId = -1;

	switch (info.typeId)
	{
	case kVolumeTypeID:
	{
		assert(info.typeId == kVolumeTypeID);

		// set some strings
		USTRING("Volume").copyTo(info.title, 128);
		USTRING("Vol").copyTo(info.shortTitle, 128);
		info.valueDesc.defaultValue = 1.0;
	}
	break;

	case kPanTypeID: // polyphonic pan.
	{
		assert(info.typeId == kPanTypeID);

		// set some strings
		USTRING("Pan").copyTo(info.title, 128);
		USTRING("Pan").copyTo(info.shortTitle, 128);

		info.flags = NoteExpressionTypeInfo::kIsBipolar | NoteExpressionTypeInfo::kIsAbsolute; // event is bipolar (centered)
	}
	break;

	case kTuningTypeID: // polyphonic pitch-bend. ('tuning')
	{
		// set the tuning type
		assert(info.typeId == kTuningTypeID);

		// set some strings
		USTRING("Tuning").copyTo(info.title, 128);
		USTRING("Tun").copyTo(info.shortTitle, 128);
		USTRING("Half Tone").copyTo(info.units, 128);

		info.flags = NoteExpressionTypeInfo::kIsBipolar; // event is bipolar (centered)

		// The range you set here, determins the default range in Cubase Note Expression Inspector.
		// Set it to +- 24 for Roli MPE Controller

		// for Tuning the convert functions are : plain = 240 * (norm - 0.5); norm = plain / 240 + 0.5;
		// we want to support only +/- one octave
		constexpr double kNormTuningOneOctave = 0.5 * 24.0 / 120.0;

		info.valueDesc.minimum = 0.5 - kNormTuningOneOctave;
		info.valueDesc.maximum = 0.5 + kNormTuningOneOctave;
//		info.valueDesc.defaultValue = 0.5; // middle of [0, 1] => no detune (240 * (0.5 - 0.5) = 0)
	}
	break;

	case kBrightnessTypeID:
		USTRING("Brightness").copyTo(info.title, 128);
		USTRING("Brt").copyTo(info.shortTitle, 128);
		info.flags = NoteExpressionTypeInfo::kIsAbsolute;
		break;

	default:
        return kResultFalse;
	}
		
	return kResultTrue;
}

tresult VST3Controller::getNoteExpressionStringByValue (int32 busIndex, int16 channel, NoteExpressionTypeID id, NoteExpressionValue valueNormalized , String128 string)
{
    // we accept only the first bus and 1 channel
    if (busIndex != 0 || channel != 0)
        return kResultFalse;

	switch (id)
	{
	case kTuningTypeID: // polyphonic pitch-bend.
	{
		// here we have to convert a normalized value to a Tuning string representation
		UString128 wrapper;
		valueNormalized = (240 * valueNormalized) - 120; // compute half Tones
		wrapper.printFloat (valueNormalized, 2);
		wrapper.copyTo (string, 128);
	}
	break;

	case kPanTypeID: // polyphonic pan.
	{
		// here we have to convert a normalized value to a Tuning string representation
		UString128 wrapper;
		valueNormalized = valueNormalized * 2.0 - 1.0;
		wrapper.printFloat (valueNormalized, 2);
		wrapper.copyTo (string, 128);
	}
	break;

	default:
	{
		// here we have to convert a normalized value to a Tuning string representation
		UString128 wrapper;
		wrapper.printFloat (valueNormalized, 2);
		wrapper.copyTo (string, 128);
	}
	break;
	}

	return kResultOk;
}

tresult VST3Controller::getNoteExpressionValueByString (int32 busIndex, int16 channel, NoteExpressionTypeID id, const TChar* string, NoteExpressionValue& valueNormalized)
{
    // we accept only the first bus and 1 channel
    if (busIndex != 0 || channel != 0)
        return kResultFalse;

 	switch (id) // polyphonic pitch-bend.
	{
	case kTuningTypeID:
	{
	   // here we have to convert a given tuning string (half Tone) to a normalized value
		String wrapper ((TChar*)string);
		ParamValue tmp;
		if (wrapper.scanFloat (tmp))
		{
			valueNormalized = (tmp + 120) / 240;
			return kResultTrue;
		}
	}
	break;

	case kPanTypeID:
	{
		String wrapper ((TChar*)string);
		ParamValue tmp;
		if (wrapper.scanFloat (tmp))
		{
			valueNormalized = (tmp + 1.0) * 0.5;
			return kResultTrue;
		}
	}
	break;

	default:
	{
		String wrapper ((TChar*)string);
		ParamValue tmp;
		if (wrapper.scanFloat (tmp))
		{
			valueNormalized = tmp;
			return kResultTrue;
		}
	}
	break;
	}

	return kResultOk;
}

tresult PLUGIN_API VST3Controller::getPhysicalUIMapping(int32 busIndex, int16 channel,
	PhysicalUIMapList& list)
{
	if (busIndex == 0 && channel == 0)
	{
		for (uint32 i = 0; i < list.count; ++i)
		{
			NoteExpressionTypeID type = kInvalidTypeID;

			switch (list.map[i].physicalUITypeID)
			{
			case kPUIXMovement:
				list.map[i].noteExpressionTypeID = kTuningTypeID;
				break;

			case kPUIYMovement:
				list.map[i].noteExpressionTypeID = kBrightnessTypeID;
				break;

			//case kPUIPressure:
			//	list.map[i].noteExpressionTypeID = ?;
			//	break;

			default:
				list.map[i].noteExpressionTypeID = kInvalidTypeID;
				break;
			}
		}
		return kResultTrue;
	}
	return kResultFalse;
}

}// namespaces
} // namespaces
