#ifndef PACKET_H
#define PACKET_H

#include <vector>
#include <string>
#include <cstring>
#include <cwchar>
#include "TypeDefs.h"
//#include "UDPSocket.h"
//#include "TCPSocket.h"

class Packet
{
	friend class TCPSocket;
	friend class UDPSocket;
public:
	Packet();
	~Packet();

	void Clear();
	size_t Size();

	Packet& operator >> (Int8& data);
	Packet& operator >> (UInt8& data);
	Packet& operator >> (Int32& data);
	Packet& operator >> (UInt32& data);
	Packet& operator >> (Int64& data);
	Packet& operator >> (UInt64& data);
	Packet& operator >> (char* data);
	Packet& operator >> (wchar_t* data);
	Packet& operator >> (std::string& data);

	Packet& operator << (Int8 data);
	Packet& operator << (UInt8 data);
	Packet& operator << (Int32 data);
	Packet& operator << (UInt32 data);
	Packet& operator << (Int64 data);
	Packet& operator << (UInt64 data);
	Packet& operator << (char* data);
	Packet& operator << (wchar_t* data);
	Packet& operator << (std::string& data);

	void AddRawData(void* data, UInt32 length);
	UInt32 ExtractRawData(void* data);

private:
	void append(const void* data, size_t length);
	std::vector<char> mData;
	std::size_t mReadPos;
};


#endif