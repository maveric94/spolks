#include "Packet.h"

Packet::Packet()
{
	mReadPos = 0;
}
Packet::~Packet()
{

}

void Packet::Clear()
{
	mData.clear();
	mReadPos = 0;
}

size_t Packet::Size()
{
	return mData.size();
}

void Packet::append(const void* data, size_t length)
{
	if (data)
	{
		size_t currentSize = mData.size();
		mData.resize(currentSize + length);
		std::memcpy(&mData[currentSize], data, length);
	}
}
Packet& Packet::operator >> (Int8& data)
{
	data = (Int8)mData[mReadPos];
	mReadPos += sizeof(data);
	return *this;
}
Packet& Packet::operator >> (UInt8& data)
{
	data = (UInt8)mData[mReadPos];
	mReadPos += sizeof(data);
	return *this;
}
Packet& Packet::operator >> (Int32& data)
{
	UInt8* bytes = (UInt8*)&mData[mReadPos];
	data = (Int32)(bytes[0] << 24) |
		(Int32)(bytes[1] << 16) |
		(Int32)(bytes[2] << 8) |
		(Int32)(bytes[3]);

	mReadPos += sizeof(data);
			
	return *this;
}

Packet& Packet::operator >> (UInt32& data)
{
	UInt8* bytes = (UInt8*)&mData[mReadPos];
	data = (UInt32)(bytes[0] << 24) |
		(UInt32)(bytes[1] << 16) |
		(UInt32)(bytes[2] << 8) |
		(UInt32)(bytes[3]);

	mReadPos += sizeof(data);

	return *this;
}

Packet& Packet::operator >> (Int64& data)
{
	UInt8* bytes = (UInt8*)&mData[mReadPos];
	data = (Int64)(bytes[0] << 56) |
		(Int64)(bytes[1] << 48) |
		(Int64)(bytes[2] << 40) |
		(Int64)(bytes[3] << 32) |
		(Int64)(bytes[4] << 24) |
		(Int64)(bytes[5] << 16) |
		(Int64)(bytes[6] << 8) |
		(Int64)(bytes[7]);

	mReadPos += sizeof(data);

	return *this;
}
Packet& Packet::operator >> (UInt64& data)
{
	UInt8* bytes = (UInt8*)&mData[mReadPos];
	data = (UInt64)(bytes[0] << 56) |
		(UInt64)(bytes[1] << 48) |
		(UInt64)(bytes[2] << 40) |
		(UInt64)(bytes[3] << 32) |
		(UInt64)(bytes[4] << 24) |
		(UInt64)(bytes[5] << 16) |
		(UInt64)(bytes[6] << 8) |
		(UInt64)(bytes[7]);

	mReadPos += sizeof(data);

	return *this;
}
Packet& Packet::operator >> (char* data)
{
	UInt32 length;
	*this >> length;

	if (length > 0)
	{
		std::memcpy(data, &mData[mReadPos], length);
		data[length] = '\0';
		mReadPos += length;
	}
	return *this;
}
Packet& Packet::operator >> (wchar_t* data)
{
	UInt32 length = 0;
	*this >> length;

	if (length > 0)
	{
		for (UInt32 i = 0; i < length; ++i)
		{
			UInt32 character = 0;
			*this >> character;
			data[i] = static_cast<wchar_t>(character);
		}
		data[length] = L'\0';
	}

	return *this;
}

Packet& Packet::operator >>(std::string& data)
{
	// First extract string length
	UInt32 length = 0;
	*this >> length;

	data.clear();
	if (length > 0)
	{
		// Then extract characters
		data.assign(&mData[mReadPos], length);

		// Update reading position
		mReadPos += length;
	}

	return *this;
}



Packet& Packet::operator << (Int8 data)
{
	append(&data, sizeof(data));
	return *this;
}
Packet& Packet::operator << (UInt8 data)
{
	append(&data, sizeof(data));
	return *this;
}
Packet& Packet::operator << (Int32 data)
{
	UInt8 bytes[] =
	{
		(UInt8)((data >> 24) & 0xFF),
		(UInt8)((data >> 16) & 0xFF),
		(UInt8)((data >> 8) & 0xFF),
		(UInt8)(data & 0xFF)
	};
	append(bytes, sizeof(data));
	return *this;
}
Packet& Packet::operator << (UInt32 data)
{
	UInt8 bytes[] =
	{
		(UInt8)((data >> 24) & 0xFF),
		(UInt8)((data >> 16) & 0xFF),
		(UInt8)((data >> 8) & 0xFF),
		(UInt8)(data & 0xFF)
	};
	append(bytes, sizeof(data));
	return *this;
}
Packet& Packet::operator << (Int64 data)
{
	UInt8 bytes[] =
	{
		(UInt8)((data >> 56) & 0xFF),
		(UInt8)((data >> 48) & 0xFF),
		(UInt8)((data >> 40) & 0xFF),
		(UInt8)((data >> 32) & 0xFF),
		(UInt8)((data >> 24) & 0xFF),
		(UInt8)((data >> 16) & 0xFF),
		(UInt8)((data >> 8) & 0xFF),
		(UInt8)(data & 0xFF)
	};
	append(bytes, sizeof(data));
	return *this;
}
Packet& Packet::operator << (UInt64 data)
{
	UInt8 bytes[] =
	{
		(UInt8)((data >> 56) & 0xFF),
		(UInt8)((data >> 48) & 0xFF),
		(UInt8)((data >> 40) & 0xFF),
		(UInt8)((data >> 32) & 0xFF),
		(UInt8)((data >> 24) & 0xFF),
		(UInt8)((data >> 16) & 0xFF),
		(UInt8)((data >> 8) & 0xFF),
		(UInt8)(data & 0xFF)
	};
	append(bytes, sizeof(data));
	return *this;
}
Packet& Packet::operator << (char* data)
{
	UInt32 length = strlen(data);
	*this << length;
	append(data, sizeof(char)* length);

	return *this;
}
Packet& Packet::operator << (wchar_t* data)
{
	UInt32 length = std::wcslen(data);
	*this << length;

	for (const wchar_t* c = data; *c != L'\0'; ++c)
		*this << (UInt32)(*c);

	return *this;
}

Packet& Packet::operator << (std::string& data)
{
	// First insert string length
	UInt32 length = static_cast<UInt32>(data.size());
	*this << length;

	// Then insert characters
	if (length > 0)
		append(data.c_str(), length * sizeof(std::string::value_type));

	return *this;
}

void Packet::AddRawData(void* data, UInt32 length)
{
	*this << length;
	append(data, sizeof(char) * length);
}

UInt32 Packet::ExtractRawData(void* data)
{
	UInt32 length;
	*this >> length;
	if (length > 0)
	{
		std::memcpy(data, &mData[mReadPos], length);
		mReadPos += length;
	}
	return length;
}