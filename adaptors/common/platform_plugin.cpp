#include "GmpiResourceManager.h"

#if !defined(IS_SYNTHEDIT_SEM) && !defined(GMPI_VST3_WRAPPER)
#include "platform.h"
#include "InterfaceObject.h"

InterfaceObject* new_InterfaceObjectA(void* addr, const wchar_t* p_name, EDirection p_direction, EPlugDataType p_datatype, const wchar_t* def_val, const wchar_t* defid, int flags, const wchar_t* p_comment, float** p_sample_ptr)
{
	return new InterfaceObject(addr, p_name, p_direction, p_datatype, def_val, defid, flags, p_comment, p_sample_ptr);
}
InterfaceObject* new_InterfaceObjectB(int p_id, struct pin_description& p_plugs_info)
{
	return new InterfaceObject(p_id, p_plugs_info);
}
InterfaceObject* new_InterfaceObjectC(int p_id, struct pin_description2& p_plugs_info)
{
	return new InterfaceObject(p_id, p_plugs_info);
}
#endif

// Meyer's singleton. see also GmpiResourceManager_editor.cpp
GmpiResourceManager* GmpiResourceManager::Instance()
{
	static GmpiResourceManager obj;
	return &obj;
}

void SafeMessagebox(
	void* hWnd,
	const wchar_t* lpText,
	const wchar_t* lpCaption = L"",
	int uType = 0
)
{
#ifdef _WIN32
	_RPTW1(0, L"%s\n", lpText);
#endif
}
