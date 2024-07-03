#pragma once

/*
#include "Controller.h"
*/

#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include "helpers/TimerManager.h"
#include "../Shared/FileWatcher.h"
#include "interThreadQue.h"
#include "MpParameter.h"
#include "ControllerHost.h"
#include "ProcessorStateManager.h"
#include "GmpiApiEditor.h"

namespace SynthEdit2
{
	class IPresenter;
}
#if 0 // TODO
// Manages SEM plugin's controllers.
class ControllerManager : public gmpi::api::IParameterObserver
{
public:
// TODO: just one:	std::vector< std::pair<int32_t, std::unique_ptr<ControllerHost> > > childPluginControllers;
	IGuiHost2* patchManager;

    virtual ~ControllerManager(){}

	int32_t setParameter(int32_t parameterHandle, int32_t fieldId, int32_t voice, const void* data, int32_t size) override
	{
		int32_t moduleHandle = -1;
		int32_t moduleParameterId = -1;
		patchManager->getParameterModuleAndParamId(parameterHandle, &moduleHandle, &moduleParameterId);

#if 0 // TODO
		for (auto& m : childPluginControllers)
		{
			if (m.first == moduleHandle)
			{
				m.second->setPluginParameter(parameterHandle, fieldId, voice, data, size);
			}
		}
#endif
		return gmpi::MP_OK;
	}

	void addController(int32_t handle, gmpi_sdk::mp_shared_ptr<gmpi::IMpController> controller)
	{
#if 0 // TODO
		childPluginControllers.push_back({ handle, std::make_unique<ControllerHost>() });
		auto chost = childPluginControllers.back().second.get();

		chost->controller_ = controller;
		chost->patchManager = patchManager;
		chost->handle = handle;
		controller->setHost(chost);
#endif
		/* TODO
		int indx = 0;
		auto p = GetParameter(this, indx);
		while (p != nullptr)
		{
		p->RegisterWatcher(chost);
		// possibly don't need to unregister. Param will get deleted when this does anyhow.
		p = GetParameter(this, ++indx);
		}
		*/
	}

	GMPI_QUERYINTERFACE1(gmpi::MP_IID_PARAMETER_OBSERVER, gmpi::IMpParameterObserver);
	GMPI_REFCOUNT;
};
#endif

class MpController;

class UndoManager
{
	std::vector< std::pair< std::string, std::unique_ptr<const DawPreset> >> history;
	DawPreset AB_storage;

	int undoPosition = -1;
	bool AB_is_A = true;

	int size()
	{
		return static_cast<int>(history.size());
	}

	void setPreset(MpController* controller, DawPreset const* preset);
	DawPreset const* push(std::string description, std::unique_ptr<const DawPreset> preset);

public:
	bool enabled = {};


	void initial(MpController* controller, std::unique_ptr<const DawPreset> preset);

	void snapshot(class MpController* controller, std::string description);

	void undo(MpController* controller);
	void redo(MpController* controller);
	void getA(MpController* controller);
	void getB(MpController* controller);
	void copyAB(MpController* controller);
	void UpdateGui(MpController* controller);
	void debug();
	bool canUndo();
	bool canRedo();
};

class MpController : /*public IGuiHost2,*/ public interThreadQueUser, public TimerClient
{
	friend class UndoManager;
	static const int UI_MESSAGE_QUE_SIZE2 = 0x500000; // 5MB. see also AUDIO_MESSAGE_QUE_SIZE

public:
	// presets from factory.xmlpreset resource.
	struct presetInfo
	{
		std::string name;
		std::string category;
		int index = -1;			// Internal Factory presets only.
		std::wstring filename;	// External disk presets only.
		std::size_t hash = 0;
		bool isFactory = false;
		bool isSession = false; // is temporary preset to accommodate preset loaded from DAW session (but not an existing preset)
	};

protected:
	static const int timerPeriodMs = 35;

private:
//	ControllerManager semControllers;
	// When syncing Preset Browser to a preset from the DAW, inhibit normal preset loading behaviour. Else preset gets loaded twice.
	bool inhibitProgramChangeParameter = {};
    file_watcher::FileWatcher fileWatcher;

	// Ignore-Program-Change support
	static const int ignoreProgramChangeStartupTimeMs = 2000;
	static const int startupTimerInit = ignoreProgramChangeStartupTimeMs / timerPeriodMs;
	int startupTimerCounter = startupTimerInit;
	bool presetsFolderChanged = false;

protected:
	std::map<int32_t, paramInfo> parametersInfo;		// for parsing xml presets
	::UndoManager undoManager;

    bool isInitialized = {};

	std::vector< std::unique_ptr<MpParameter> > parameters_;
	std::map< std::pair<int, int>, int > moduleParameterIndex;		// Module Handle/ParamID to Param Handle.
	std::map< int, MpParameter* > ParameterHandleIndex;				// Param Handle to Parameter*.
//	std::vector<gmpi::IMpParameterObserver*> m_guis2;
	std::vector<gmpi::api::IParameterObserver*> m_guis3;
	
//	SE2::IPresenter* presenter_ = nullptr;

    interThreadQue message_que_dsp_to_ui;
	bool isSemControllersInitialised = false;

	// see also VST3Controller.programNames
	std::vector< presetInfo > presets;
	DawPreset session_preset;

#if 0 // TODO
	GmpiGui::FileDialog nativeFileDialog;
	GmpiGui::OkCancelDialog okCancelDialog;
#endif

	void OnFileDialogComplete(int mode, int32_t result);
	virtual void OnStartupTimerExpired();

public:

	MpController() :
		message_que_dsp_to_ui(UI_MESSAGE_QUE_SIZE2)
	{
//		semControllers.patchManager = this;
//		RegisterGui2(&semControllers);
	}
    
    ~MpController();

	void ScanPresets();
	void setPreset(DawPreset const* preset);
	void syncPresetControls(DawPreset const* preset);

	void SavePreset(int32_t presetIndex);
	void SavePresetAs(const std::string& presetName);
	void DeletePreset(int presetIndex);
	void UpdatePresetBrowser();

	void Initialize();

	void initSemControllers();

	//int32_t getController(int32_t moduleHandle, gmpi::IMpController ** returnController) override;

	//void setMainPresenter(SE2::IPresenter* presenter) override
	//{
	//	presenter_ = presenter;
	//}

	// Override these
	virtual void ParamGrabbed(MpParameter_native* param) = 0;

	// Presets
	// asking platform-specific controller to set preset on both controller and processor.
	// the preset originates from the plugins preset browser, not from the DAW.
	virtual void setPresetXmlFromSelf(const std::string& xml) = 0;
	virtual void setPresetFromSelf(DawPreset const* preset) = 0;
	virtual std::string loadNativePreset(std::wstring sourceFilename) = 0;
	virtual void saveNativePreset(const char* filename, const std::string& presetName, const std::string& xml) = 0;
	virtual std::wstring getNativePresetExtension() = 0;
	int getPresetCount()
	{
		return static_cast<int>(presets.size());
	}
	presetInfo getPresetInfo(int index) // return a copy on purpose so we can rescan presets from inside PresetMenu.callbackOnDeleteClicked lambda
	{
		if (index < 0 || index >= presets.size())
			return { {}, {}, -1, {}, 0, false, true };

		return presets[index];
	}
	bool isPresetModified();
	void FileToString(const platform_string& path, std::string& buffer);

	MpController::presetInfo parsePreset(const std::wstring& filename, const std::string& xml);
	std::vector< MpController::presetInfo > scanNativePresets();
	virtual std::vector< MpController::presetInfo > scanFactoryPresets() = 0;
	virtual void loadFactoryPreset(int index, bool fromDaw) = 0;
	virtual std::string getFactoryPresetXml(std::string filename) = 0;
	std::vector<MpController::presetInfo> scanPresetFolder(platform_string PresetFolder, platform_string extension);

	void ParamToDsp(MpParameter* param, int32_t voice = 0);
	void SerialiseParameterValueToDsp(my_msg_que_output_stream& stream, MpParameter* param, int32_t voice = 0);
	void UpdateProgramCategoriesHc(MpParameter * param);
	MpParameter* createHostParameter(int32_t hostControl);
//	virtual int32_t sendSdkMessageToAudio(int32_t handle, int32_t id, int32_t size, const void* messageData) override;
	void OnSetHostControl(int hostControl, gmpi::Field paramField, int32_t size, const void * data, int32_t voice);

	gmpi::ReturnCode RegisterGui2(gmpi::api::IParameterObserver* gui)
	{
		m_guis3.push_back(gui);
		return gmpi::ReturnCode::Ok;
	}
	gmpi::ReturnCode UnRegisterGui2(gmpi::api::IParameterObserver* gui)
	{
#if _HAS_CXX20
		std::erase(m_guis3, gui);
#else
		if (auto it = find(m_guis3.begin(), m_guis3.end(), gui); it != m_guis3.end())
			m_guis3.erase(it);
#endif
		return gmpi::ReturnCode::Ok;
	}
#if 0
	// IGuiHost2
	int32_t RegisterGui2(gmpi::IMpParameterObserver* gui) override
	{
		m_guis2.push_back(gui);
		return gmpi::MP_OK;
	}
	int32_t UnRegisterGui2(gmpi::IMpParameterObserver* gui) override
	{
#if _HAS_CXX20
		std::erase(m_guis2, gui);
#else
		if (auto it = find(m_guis2.begin(), m_guis2.end(), gui); it != m_guis2.end())
			m_guis2.erase(it);
#endif
		return gmpi::MP_OK;
	}
void initializeGui(gmpi::IMpParameterObserver* gui, int32_t parameterHandle, gmpi::Field FieldId) override;
#endif
	void initializeGui(gmpi::api::IParameterObserver* gui, int32_t parameterHandle, gmpi::Field FieldId);
	void setParameterValue(RawView value, int32_t parameterHandle, gmpi::Field moduleFieldId = gmpi::Field::MP_FT_VALUE, int32_t voice = 0);// override;
	int32_t getParameterModuleAndParamId(int32_t parameterHandle, int32_t* returnModuleHandle, int32_t* returnModuleParameterId);// override;
#if 0
	int32_t getParameterHandle(int32_t moduleHandle, int32_t moduleParameterId) override;
	RawView getParameterValue(int32_t parameterHandle, int32_t fieldId, int32_t voice = 0) override;
	virtual int32_t resolveFilename(const wchar_t* shortFilename, int32_t maxChars, wchar_t* returnFullFilename) override;
	void serviceGuiQueue() override
	{
		message_que_dsp_to_ui.pollMessage(this);
	}
#endif
	MpParameter* getHostParameter(int32_t hostControl);

	void ImportPresetXml(const char* filename, int presetIndex = -1);
	std::unique_ptr<const DawPreset> getPreset(std::string presetNameOverride = {});
	void ExportPresetXml(const char* filename, std::string presetNameOverride = {});
	void ImportBankXml(const char * filename);
	void setModified(bool presetIsModified);
	void ExportBankXml(const char * filename);

#if 0 // TODO
	gmpi_gui::IMpGraphicsHost * getGraphicsHost();
#endif

	void updateGuis(MpParameter* parameter, int voice)
	{
		const auto rawValue = parameter->getValueRaw(gmpi::Field::MP_FT_VALUE, voice);
		const float normalized = parameter->getNormalized(); // voice !!!?
#if 0 // TODO

		for (auto pa : m_guis2)
		{
			// Update value.
			pa->setParameter(parameter->parameterHandle_, gmpi::Field::MP_FT_VALUE, voice, rawValue.data(), (int32_t)rawValue.size());

			// Update normalized.
			pa->setParameter(parameter->parameterHandle_, gmpi::Field::MP_FT_NORMALIZED, voice, &normalized, (int32_t)sizeof(normalized));
		}
#endif

		for (auto pa : m_guis3)
		{
			// Update value.
			pa->setParameter(parameter->parameterHandle_, gmpi::Field::MP_FT_VALUE, voice, (int32_t)rawValue.size(), rawValue.data());

			// Update normalized.
			pa->setParameter(parameter->parameterHandle_, gmpi::Field::MP_FT_NORMALIZED, voice, (int32_t)sizeof(normalized), &normalized);
		}
	}

	void updateGuis(MpParameter* parameter, gmpi::Field fieldType, int voice = 0 )
	{
		auto rawValue = parameter->getValueRaw(fieldType, voice);

		//for (auto pa : m_guis2)
		//{
		//	pa->setParameter(parameter->parameterHandle_, fieldType, voice, rawValue.data(), (int32_t)rawValue.size());
		//}
		for (auto pa : m_guis3)
		{
			pa->setParameter(parameter->parameterHandle_, fieldType, voice, (int32_t)rawValue.size(), rawValue.data());
		}
	}

	// interThreadQueUser
	bool OnTimer() override;
	bool onQueMessageReady(int handle, int msg_id, class my_input_stream& p_stream) override;


	virtual IWriteableQue* getQueueToDsp() = 0;

	interThreadQue* getQueueToGui()
	{
		return &message_que_dsp_to_ui;
	}

	virtual void ResetProcessor() {}
	virtual void OnStartPresetChange() {}
	virtual void OnEndPresetChange();
	virtual void OnLatencyChanged() {}
	virtual MpParameter_native* makeNativeParameter(int ParameterTag, bool isInverted = false) = 0;
};
