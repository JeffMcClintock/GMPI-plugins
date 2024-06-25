//#include "pch.h"

// Fix for <sstream> on Mac (sstream uses undefined int_64t)
//#include "mp_api.h"
#include <sstream>
#include <assert.h>

#include "HostControls.h"
#include "se_datatypes.h"
#include "midi_defs.h"

using namespace std;

struct HostControlStruct // holds XML -> enum info
{
	const wchar_t* display;
	HostControls id;
	gmpi::PinDatatype datatype;
	int automation;
};

/* categories:
- Voice - Note expression. Polyphonic parameters targeted at individual voices.
- Time  - Song position and tempo related.
*/

static const HostControlStruct lookup[] =
{
	{(L"PatchCommands")			,HC_PATCH_COMMANDS, gmpi::PinDatatype::Int32,ControllerType::None},

	{(L"MidiChannelIn")			,HC_MIDI_CHANNEL, gmpi::PinDatatype::Int32,ControllerType::None},

	{(L"ProgramNamesList")		,HC_PROGRAM_NAMES_LIST, gmpi::PinDatatype::String,ControllerType::None},

	{(L"Program")				,HC_PROGRAM, gmpi::PinDatatype::Int32,ControllerType::None},

	{(L"ProgramName")			,HC_PROGRAM_NAME, gmpi::PinDatatype::String,ControllerType::None},

	{(L"Voice/Trigger")			,HC_VOICE_TRIGGER, gmpi::PinDatatype::Float32,ControllerType::Trigger << 24},

	{(L"Voice/Gate")			,HC_VOICE_GATE, gmpi::PinDatatype::Float32,ControllerType::Gate << 24},

	{(L"Voice/Pitch")			,HC_VOICE_PITCH, gmpi::PinDatatype::Float32,ControllerType::Pitch << 24},

	{(L"Voice/VelocityKeyOn")	,HC_VOICE_VELOCITY_KEY_ON, gmpi::PinDatatype::Float32,ControllerType::VelocityOn << 24},

	{(L"Voice/VelocityKeyOff")	,HC_VOICE_VELOCITY_KEY_OFF, gmpi::PinDatatype::Float32,ControllerType::VelocityOff << 24},

	{(L"Voice/Aftertouch")		,HC_VOICE_AFTERTOUCH, gmpi::PinDatatype::Float32,ControllerType::PolyAftertouch << 24},

	{(L"Voice/VirtualVoiceId")	,HC_VOICE_VIRTUAL_VOICE_ID, gmpi::PinDatatype::Int32,ControllerType::VirtualVoiceId << 24},

	{(L"Voice/Active")			,HC_VOICE_ACTIVE				, gmpi::PinDatatype::Float32,ControllerType::Active << 24},

	{(L"XXXXXXXXXX")			,HC_UNUSED						, gmpi::PinDatatype::Float32, ControllerType::None},// was HC_VOICE_RESET -"Voice/Reset"

	{(L"VoiceAllocationMode")	,HC_VOICE_ALLOCATION_MODE		, gmpi::PinDatatype::Int32, ControllerType::None},

	{(L"Bender")				,HC_PITCH_BENDER				, gmpi::PinDatatype::Float32, ControllerType::Bender << 24},

	{(L"HoldPedal")				,HC_HOLD_PEDAL					, gmpi::PinDatatype::Float32, (ControllerType::CC << 24) | 64}, // CC 64 = Hold Pedal

	{(L"Channel Pressure")		,HC_CHANNEL_PRESSURE			, gmpi::PinDatatype::Float32, ControllerType::ChannelPressure << 24},

	{(L"Time/BPM")				,HC_TIME_BPM					, gmpi::PinDatatype::Float32, ControllerType::BPM << 24},

	{(L"Time/SongPosition")		,HC_TIME_QUARTER_NOTE_POSITION	, gmpi::PinDatatype::Float32, ControllerType::SongPosition << 24},

	{(L"Time/TransportPlaying")	,HC_TIME_TRANSPORT_PLAYING		, gmpi::PinDatatype::Bool, ControllerType::TransportPlaying << 24},
	{(L"Polyphony")				,HC_POLYPHONY					, gmpi::PinDatatype::Int32, ControllerType::None},
	{(L"ReserveVoices")			,HC_POLYPHONY_VOICE_RESERVE		, gmpi::PinDatatype::Int32, ControllerType::None},
	{(L"Oversampling/Rate")		,HC_OVERSAMPLING_RATE			, gmpi::PinDatatype::Enum, ControllerType::None},
	{(L"Oversampling/Filter")	, HC_OVERSAMPLING_FILTER		, gmpi::PinDatatype::Enum, ControllerType::None},
	{(L"User/Int0")				, HC_USER_SHARED_PARAMETER_INT0	, gmpi::PinDatatype::Int32, ControllerType::None},
	{(L"Time/BarStartPosition"), HC_TIME_BAR_START				, gmpi::PinDatatype::Float32, ControllerType::barStartPosition << 24},
	{(L"Time/Timesignature/Numerator"), HC_TIME_NUMERATOR		, gmpi::PinDatatype::Int32, ControllerType::timeSignatureNumerator << 24},
	{(L"Time/Timesignature/Denominator"), HC_TIME_DENOMINATOR	, gmpi::PinDatatype::Int32, ControllerType::timeSignatureDenominator << 24},

	// VST3 Note-expression (and MIDI 2.0 support)
	// multitrack studio: "VST3 instruments that support VST3 note expression receive per-note Pitch Bend, Volume, Pan, Expression, Brightness and Vibrato Depth. Poly Aftertouch is sent as well."

	// MIDI 2.0 Per-Note Expression 7
	{(L"Voice/Volume")		, HC_VOICE_VOLUME, gmpi::PinDatatype::Float32, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kVolumeTypeID << 16)},

	// MIDI 2.0 Per-Note Expression 10
	{(L"Voice/Pan")			, HC_VOICE_PAN, gmpi::PinDatatype::Float32, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kPanTypeID << 16)},

	// MIDI 2.0 Per-Note Pitch Bend Message (was "Tuning"). bi-polar. 0 = -10 octaves, 1.0 = +10 octaves. 
	{(L"Voice/Bender")		, HC_VOICE_PITCH_BEND, gmpi::PinDatatype::Float32, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kTuningTypeID << 16)},

	// MIDI 2.0 Per-Note Vibrato Depth. per-note CC 8
	{(L"Voice/Vibrato")		, HC_VOICE_VIBRATO, gmpi::PinDatatype::Float32, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kVibratoTypeID << 16)},

	// MIDI 2.0 Per-Note Expression. per-note CC 11
	{(L"Voice/Expression")	, HC_VOICE_EXPRESSION, gmpi::PinDatatype::Float32, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kExpressionTypeID << 16)},

	// MIDI 2.0 Per-Note Expression 5
	{(L"Voice/Brightness")	, HC_VOICE_BRIGHTNESS, gmpi::PinDatatype::Float32, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kBrightnessTypeID << 16)},

	{(L"Voice/UserControl0")	, HC_VOICE_USER_CONTROL0, gmpi::PinDatatype::Float32, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kCustomStart << 16)},
	{(L"Voice/UserControl1")	, HC_VOICE_USER_CONTROL1, gmpi::PinDatatype::Float32, (ControllerType::VoiceNoteExpression << 24) | ((ControllerType::kCustomStart + 1) << 16)},
	{(L"Voice/UserControl2")	, HC_VOICE_USER_CONTROL2, gmpi::PinDatatype::Float32, (ControllerType::VoiceNoteExpression << 24) | ((ControllerType::kCustomStart + 2) << 16)},
	{(L"Voice/PortamentoEnable")	, HC_VOICE_PORTAMENTO_ENABLE, gmpi::PinDatatype::Float32, (ControllerType::VoiceNoteControl << 24) | ((ControllerType::kPortamentoEnable) << 16)},


	{(L"SnapModulation")		, HC_SNAP_MODULATION__DEPRECATED, gmpi::PinDatatype::Int32, ControllerType::None},
	{( L"Portamento" )		, HC_PORTAMENTO, gmpi::PinDatatype::Float32, ( ControllerType::CC << 24 ) | 5}, // CC 5 = Portamento Time 
	{( L"Voice/GlideStartPitch" ), HC_GLIDE_START_PITCH, gmpi::PinDatatype::Float32, ControllerType::GlideStartPitch << 24},
	{( L"BenderRange" )		, HC_BENDER_RANGE, gmpi::PinDatatype::Float32, ( ControllerType::RPN << 24 ) | 0}, // RPN 0000 = Pitch bend range (course = semitones, fine = cents).

	{L"SubPatchCommands"		, HC_SUB_PATCH_COMMANDS, gmpi::PinDatatype::Int32, ControllerType::None},
	{L"Processor/OfflineRenderMode", HC_PROCESS_RENDERMODE, gmpi::PinDatatype::Enum, ControllerType::None}, // 0 = :Live", 2 = "Preview" (Offline)

	{L"User/Int1"				, HC_USER_SHARED_PARAMETER_INT1	, gmpi::PinDatatype::Int32, ControllerType::None},
	{L"User/Int2"				, HC_USER_SHARED_PARAMETER_INT2	, gmpi::PinDatatype::Int32, ControllerType::None},
	{L"User/Int3"				, HC_USER_SHARED_PARAMETER_INT3	, gmpi::PinDatatype::Int32, ControllerType::None},
	{L"User/Int4"				, HC_USER_SHARED_PARAMETER_INT4	, gmpi::PinDatatype::Int32, ControllerType::None},
	{L"PatchCables"				, HC_PATCH_CABLES				, gmpi::PinDatatype::Blob, ControllerType::None},

	{L"Processor/SilenceOptimisation", HC_SILENCE_OPTIMISATION	, gmpi::PinDatatype::Bool, ControllerType::None},

	{L"ProgramCategory"			,HC_PROGRAM_CATEGORY			, gmpi::PinDatatype::String, ControllerType::None},
	{L"ProgramCategoriesList"	,HC_PROGRAM_CATEGORIES_LIST		, gmpi::PinDatatype::String, ControllerType::None},

	{L"MpeMode"					,HC_MPE_MODE					, gmpi::PinDatatype::Int32, ControllerType::None},
	{L"Presets/ProgramModified"	,HC_PROGRAM_MODIFIED			, gmpi::PinDatatype::Bool, ControllerType::None},
	{L"Presets/CanUndo"			,HC_CAN_UNDO					, gmpi::PinDatatype::Bool, ControllerType::None},
	{L"Presets/CanRedo"			,HC_CAN_REDO					, gmpi::PinDatatype::Bool, ControllerType::None },

	// MAINTAIN ORDER TO PRESERVE OLDER WAVES EXPORTS DSP.XML consistency
};

HostControls StringToHostControl( const wstring& txt )
{
	const int size = (sizeof(lookup) / sizeof(HostControlStruct));

//	_RPT1(_CRT_WARN, "StringToHostControl %s.\n", WStringToUtf8(txt).c_str());

	HostControls returnVal = HC_NONE;

	if( txt.compare( L"" ) != 0 )
	{
		int j;

		for( j = 0 ; j < size ; j++ )
		{
			if( txt.compare( lookup[j].display ) == 0  )
			{
				returnVal = lookup[j].id;
				break;
			}
		}

		if( j == size )
		{
			// Avoiding msg box in plugin
			assert(false);
//			GetApp()->SeMessageBox( errorText ,MB_OK|MB_ICONSTOP );
		}
	}

	return returnVal;
}

gmpi::PinDatatype GetHostControlDatatype( HostControls hostControlId )
{
	const int DATAYPE_INFO_COUNT = (sizeof(lookup) / sizeof(HostControlStruct));
	if(hostControlId < DATAYPE_INFO_COUNT)
	{
		return lookup[hostControlId].datatype;
	}

	assert(false);
	return gmpi::PinDatatype::Float32;
}

const wchar_t* GetHostControlName( HostControls hostControlId )
{
	const int DATAYPE_INFO_COUNT = (sizeof(lookup) / sizeof(HostControlStruct));
	if(hostControlId < DATAYPE_INFO_COUNT)
	{
		return lookup[hostControlId].display;
	}

	assert(false);
	return L"";
}

const wchar_t* GetHostControlNameByAutomation(int automation)
{
	const int DATAYPE_INFO_COUNT = (sizeof(lookup) / sizeof(HostControlStruct));

	for (int j = 0; j < DATAYPE_INFO_COUNT; j++)
	{
		if (lookup[j].automation == automation)
		{
			return lookup[j].display;
			break;
		}
	}

	assert(false);
	return L"";
}

int GetHostControlAutomation( HostControls hostControlId )
{
	const int DATAYPE_INFO_COUNT = (sizeof(lookup) / sizeof(HostControlStruct));
	if(hostControlId < DATAYPE_INFO_COUNT)
	{
		return lookup[hostControlId].automation;
	}

	assert(false);
	return ControllerType::None;
}

// Most host controls 'belong' to the Patch Automator, however a handfull apply to the local parent container.
bool HostControlAttachesToParentContainer( HostControls hostControlId )
{
	switch( hostControlId )
	{
		case HC_VOICE_TRIGGER:
		case HC_VOICE_GATE:
		case HC_VOICE_PITCH:
		case HC_VOICE_VELOCITY_KEY_ON:
		case HC_VOICE_VELOCITY_KEY_OFF:
		case HC_VOICE_AFTERTOUCH:
		case HC_VOICE_VIRTUAL_VOICE_ID:
		case HC_VOICE_ACTIVE:
		case HC_VOICE_ALLOCATION_MODE:
		case HC_PITCH_BENDER:
		case HC_HOLD_PEDAL:
		case HC_CHANNEL_PRESSURE :
        case HC_POLYPHONY:
        case HC_POLYPHONY_VOICE_RESERVE:
        case HC_OVERSAMPLING_RATE:
        case HC_OVERSAMPLING_FILTER:
		case HC_VOICE_VOLUME:
		case HC_VOICE_PAN:
		case HC_VOICE_TUNING:
		case HC_VOICE_PITCH_BEND:
		case HC_VOICE_VIBRATO:
		case HC_VOICE_EXPRESSION:
		case HC_VOICE_BRIGHTNESS:
		case HC_VOICE_USER_CONTROL0:
		case HC_VOICE_USER_CONTROL1:
		case HC_VOICE_USER_CONTROL2:
		case HC_VOICE_PORTAMENTO_ENABLE:
		case HC_SNAP_MODULATION__DEPRECATED:
		case HC_PORTAMENTO:
		case HC_GLIDE_START_PITCH:
		case HC_BENDER_RANGE:
			return true;
		break;

		default:
            return false;
		break;
	}

	return false;
}

bool HostControlisPolyphonic(HostControls hostControlId)
{
	switch (hostControlId)
	{
	case HC_VOICE_TRIGGER:
	case HC_VOICE_GATE:
	case HC_VOICE_PITCH:
	case HC_VOICE_VELOCITY_KEY_ON:
	case HC_VOICE_VELOCITY_KEY_OFF:
	case HC_VOICE_AFTERTOUCH:
	case HC_VOICE_VIRTUAL_VOICE_ID:
	case HC_VOICE_ACTIVE:
	case HC_VOICE_PORTAMENTO_ENABLE:
	case HC_VOICE_VOLUME:
	case HC_VOICE_PAN:
//nope	case HC_VOICE_TUNING:
	case HC_VOICE_PITCH_BEND:
	case HC_VOICE_VIBRATO:
	case HC_VOICE_EXPRESSION:
	case HC_VOICE_BRIGHTNESS:
	case HC_VOICE_USER_CONTROL0:
	case HC_VOICE_USER_CONTROL1:
	case HC_VOICE_USER_CONTROL2:
	case HC_GLIDE_START_PITCH:

		return true;
		break;

	default:
		return false;
		break;
	}

	return false;
}

bool AffectsVoiceAllocation(HostControls hostControlId)
{
	switch( hostControlId )
	{
	case HC_VOICE_ALLOCATION_MODE:
	case HC_PORTAMENTO:
		return true;
		break;

	default:
		return false;
		break;
	}

	return false;
}