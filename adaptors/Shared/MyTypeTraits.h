#pragma once
#include <string>
#include <sstream>
//#include "conversion.h"
//#include "mp_sdk_common.h"
#include "Common.h"

//typedef std::vector<uint8_t> MpBlob;

// helpers to convert any parameter value to raw bytes
template<class U> struct VariableLengthStorageTraits
{
	enum { result = false };
};

template<>
struct VariableLengthStorageTraits<std::wstring>
{
	enum { result = true };
};

template<>
struct VariableLengthStorageTraits<gmpi::Blob>
{
	enum { result = true };
};


// helpers to SE datatype.
template<class U> struct SynthEditDatatypeTrait
{
	static const int result = -1;
};

template<>
struct SynthEditDatatypeTrait<std::wstring>
{
	static const int result = 1;
};

template<>
struct SynthEditDatatypeTrait<gmpi::Blob>
{
	static const int result = 10;
};

template<>
struct SynthEditDatatypeTrait<int>
{
	static const int result = 9;
};

template<>
struct SynthEditDatatypeTrait<float>
{
	static const int result = 6;
};


template <typename T>
class MyTypeTraits
{
public:
	// convert string to this type.
	static void parse(const wchar_t* stringValue, T& returnValue);

	// convert UTF8 string to this type.
	static void parse(const char* stringValue, T& returnValue);

	// convert this type to UTF8 string.
	static std::string toXML( const T& value );

#if defined( SE_SUPPORT_MFC )
	static void MfcArchiveWrite( class CArchive& ar, T& value )
	{
		ar << value;
	};
	static void MfcArchiveRead( class CArchive& ar, T& value )
	{
		ar >> value;
	};
#endif

	enum { IsVariableLengthType = VariableLengthStorageTraits<T>::result };
	static const int FixedTypeSize = sizeof(T);
	static const int DataType = SynthEditDatatypeTrait<T>::result;
};


// Wide strings.
template<>
void MyTypeTraits<std::wstring>::parse(const wchar_t* stringValue, std::wstring& returnValue);
template<>
void MyTypeTraits<gmpi::Blob>::parse(const wchar_t* stringValue, gmpi::Blob& returnValue);
template<>
void MyTypeTraits<float>::parse(const wchar_t* stringValue, float& returnValue);
template<>
void MyTypeTraits<int>::parse(const wchar_t* stringValue, int& returnValue);
template<>
void MyTypeTraits<short>::parse(const wchar_t* stringValue, short& returnValue);
template<>
void MyTypeTraits<bool>::parse(const wchar_t* stringValue, bool& returnValue);
template<>
void MyTypeTraits<double>::parse(const wchar_t* stringValue, double& returnValue);

// UTF8 strings.
template<>
void MyTypeTraits<std::wstring>::parse(const char* stringValue, std::wstring& returnValue);
template<>
void MyTypeTraits<gmpi::Blob>::parse(const char* stringValue, gmpi::Blob& returnValue);
template<>
void MyTypeTraits<float>::parse(const char* stringValue, float& returnValue);
template<>
void MyTypeTraits<int>::parse(const char* stringValue, int& returnValue);
template<>
void MyTypeTraits<short>::parse(const char* stringValue, short& returnValue);
template<>
void MyTypeTraits<bool>::parse(const char* stringValue, bool& returnValue);
template<>
void MyTypeTraits<double>::parse(const char* stringValue, double& returnValue);

template<>
std::string MyTypeTraits<gmpi::Blob>::toXML( const gmpi::Blob& value );
template<>
std::string MyTypeTraits<std::wstring>::toXML( const std::wstring& value );
template<>
std::string MyTypeTraits<float>::toXML( const float& value );
template<>
std::string MyTypeTraits<int>::toXML(const int& value);
template<>
std::string MyTypeTraits<int64_t>::toXML(const int64_t& value);
template<>
std::string MyTypeTraits<short>::toXML( const short& value );
template<>
std::string MyTypeTraits<bool>::toXML( const bool& value );
template<>
std::string MyTypeTraits<double>::toXML( const double& value );


#if defined( SE_SUPPORT_MFC )

template<>
void MyTypeTraits<gmpi::Blob>::MfcArchiveWrite( class CArchive& ar, gmpi::Blob& value )
{
	ar << (int) value.getSize();
	ar.Write( value.getData(), value.getSize() );
}

template<>
void MyTypeTraits<gmpi::Blob>::MfcArchiveRead( class CArchive& ar, gmpi::Blob& value )
{
	int size;
	ar >> size;
	unsigned char* buff = new unsigned char[size];
	ar.Read( buff, size );
	value.setValueRaw( size, buff );
	delete [] buff;
}

#endif
