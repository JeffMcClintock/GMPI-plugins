//#include "pch.h"

#include "my_input_stream.h"
//#include "mp_sdk_common.h"
//#include "mfc_emulation.h"


my_input_stream& my_input_stream::operator>>(gmpi::Blob& val)
{
    int size;
    Read(&size,sizeof(size));
	val.resize(size);
//    char* dat = new char[size];
    Read(val.data(), size);
//    val.setValueRaw(size,dat);
//    delete [] dat;
    return *this;
}

my_output_stream::my_output_stream()
{
}

my_output_stream::~my_output_stream()
{
}

my_output_stream& my_output_stream::operator<<( const std::string& val )
{
    int32_t size = (int32_t) val.size();
    Write(&size,sizeof(size));
    Write( (void*) val.data(), size * sizeof(val[0]));
    return *this;
}

my_output_stream& my_output_stream::operator<<( const std::wstring& val )
{
    int32_t size = (int32_t) val.size();
    Write(&size,sizeof(size));
    Write( (void*) val.data(), size * sizeof(val[0]));
    return *this;
}

my_output_stream& my_output_stream::operator<<(const gmpi::Blob& val)
{
    int size = val.size();
    Write(&size,sizeof(size));
    Write(val.data(),size);
    return *this;
};

