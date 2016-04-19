#ifndef __COMM_GUID__
#define __COMM_GUID__

#include <vector>
#include <list>
#include <set>

#include <Rpc.h>
#include <Oleauto.h>

#include "TString.h"


#ifndef _WIN32_WCE
    #pragma comment(lib, "Rpcrt4.lib")
#endif

namespace OS
{

	//	ID
	class ID : public UUID
	{
	public:
		ID()
		{
			ZeroMemory(static_cast<UUID *>(this), sizeof(UUID));
		}

		ID(const BSTR bstr)
		{
			FromBSTR(bstr);
		}

		ID(const UUID & id)
		{
			CopyMemory(static_cast<UUID *>(this), &id, sizeof(UUID));
		}

		ID(const ID & id)
		{
			CopyMemory(static_cast<UUID *>(this), &id, sizeof(UUID));
		}

		~ID()
		{}

		size_t GetSerializeLen()
		{
			return sizeof(UUID);
		}

		BYTE * Serialize(BYTE *pRaw)
		{
			CopyMemory(pRaw, static_cast<UUID *>(this), sizeof(UUID));
			return pRaw + sizeof(UUID);
		}

		const BYTE * Deserialize(const BYTE *pRaw)
		{
			CopyMemory(static_cast<UUID *>(this), pRaw, sizeof(UUID));
			return pRaw + sizeof(UUID);
		}

		const BYTE * Deserialize_s(const BYTE *pRaw, int & sz)
		{
			if (pRaw == NULL || sz < sizeof (UUID))
				return NULL;

			CopyMemory(static_cast<UUID *>(this), pRaw, sizeof(UUID));
			pRaw += sizeof(UUID);

			return pRaw;
		}

		void Initialize(void)
		{
#ifndef SVS_GTNETS

#ifndef _WIN32_WCE
#pragma warning(suppress: 6031)
			UuidCreate(this);	// not supported by wince
#else
			srand(GetTickCount());
			Data1 = (DWORD) (ULONG_MAX * (1.0 * rand() / RAND_MAX));
			Data2 = (WORD) (USHRT_MAX * (1.0 * rand() / RAND_MAX));
			Data3 = (WORD) (USHRT_MAX * (1.0 * rand() / RAND_MAX));
			for (int i=0; i<8; i++)
				Data4[i] = (BYTE) (UCHAR_MAX * (1.0 * rand() / RAND_MAX));
#endif // _WIN32_WCE

#else
			Data1 = (DWORD) (ULONG_MAX * (1.0 * rand() / RAND_MAX));
			Data2 = (WORD) (USHRT_MAX * (1.0 * rand() / RAND_MAX));
			Data3 = (WORD) (USHRT_MAX * (1.0 * rand() / RAND_MAX));
			for (int i=0; i<8; i++)
				Data4[i] = (BYTE) (UCHAR_MAX * (1.0 * rand() / RAND_MAX));
#endif
		}

		__forceinline bool operator==(const ID& right) const
		{
			return (
				(*reinterpret_cast<const unsigned __int64*>(&Data1)
				== *reinterpret_cast<const unsigned __int64*>(&(right.Data1)))
				&& (*reinterpret_cast<const unsigned __int64*>(&Data4)
				== *reinterpret_cast<const unsigned __int64*>(&(right.Data4)))
				);
		}

		__forceinline bool operator != (const ID& right) const
		{
			return (
				(*reinterpret_cast<const unsigned __int64*>(&Data1)
				!= *reinterpret_cast<const unsigned __int64*>(&(right.Data1)))
				|| (*reinterpret_cast<const unsigned __int64*>(&Data4)
				!= *reinterpret_cast<const unsigned __int64*>(&(right.Data4)))
				);
		}

		__forceinline bool operator==(const UUID& right) const
		{
			return (
				(*reinterpret_cast<const unsigned __int64*>(&Data1)
				== *reinterpret_cast<const unsigned __int64*>(&(right.Data1)))
				&& (*reinterpret_cast<const unsigned __int64*>(&Data4)
				== *reinterpret_cast<const unsigned __int64*>(&(right.Data4)))
				);
		}

		__forceinline bool operator!=(const UUID& right) const
		{
			return (
				(*reinterpret_cast<const unsigned __int64*>(&Data1)
				!= *reinterpret_cast<const unsigned __int64*>(&(right.Data1)))
				|| (*reinterpret_cast<const unsigned __int64*>(&Data4)
				!= *reinterpret_cast<const unsigned __int64*>(&(right.Data4)))
				);
		}

		__forceinline bool operator<(const ID& right) const
		{
			if (*reinterpret_cast<const unsigned __int64*>(&Data4)
				< *reinterpret_cast<const unsigned __int64*>(&(right.Data4)))
				return true;
			else if (*reinterpret_cast<const unsigned __int64*>(&Data4)
				> *reinterpret_cast<const unsigned __int64*>(&(right.Data4)))
				return false;
			else {
				return (*reinterpret_cast<const unsigned __int64*>(&Data1)
					< *reinterpret_cast<const unsigned __int64*>(&(right.Data1)));
			}
		}

		//	hash_map
		//	http://exe.adam.ne.jp/j/componentid.html

		__forceinline operator size_t() const
		{
			const BYTE* data = reinterpret_cast<const BYTE*>(this);
			short c0 = 0;
			short c1 = 0;
			for (DWORD i = 0; i < sizeof(UUID); ++i) {
				c0 += static_cast<short>(data[i]);
				c1 += c0;
			}

			short x = -c1 % 255;
			if (x < 0)
				x += 255;

			short y = (c1 - c0) % 255;
			if (y < 0)
				y += 255;

			return y * 256 + x;
		}

		BSTR ID2BSTR() const
		{
			OLECHAR temp[39];
			StringFromGUID2(*this, temp, 39);
			return SysAllocString(temp);
		}

		BOOL ID2WSTR(wchar_t *str) const
		{
			if (str == NULL)
				return FALSE;
#ifdef _WIN32_WCE
			swprintf(str, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
#else
			swprintf_s(str, 39, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
#endif
				Data1, Data2, Data3,
				Data4[0], Data4[1], Data4[2], Data4[3],
				Data4[4], Data4[5], Data4[6], Data4[7]);
			return TRUE;
		}

		BOOL ID2WSTRWithoutBraces(wchar_t *str) const
		{
			if (str == NULL)
				return FALSE;
#ifdef _WIN32_WCE
			swprintf(str, L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
#else
			swprintf_s(str, 37, L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
#endif
				Data1, Data2, Data3,
				Data4[0], Data4[1], Data4[2], Data4[3],
				Data4[4], Data4[5], Data4[6], Data4[7]);
			return TRUE;
		}

		BOOL ID2STR(char* str) const
		{
			if (str == NULL)
				return FALSE;
#ifdef _WIN32_WCE
			sprintf(str, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
#else
			sprintf_s(str, 39, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
#endif
				Data1, Data2, Data3,
				Data4[0], Data4[1], Data4[2], Data4[3],
				Data4[4], Data4[5], Data4[6], Data4[7]);
			return TRUE;
		}

		BOOL ID2STRWithoutBraces(char *str) const
		{
			if (str == NULL)
				return FALSE;
#ifdef _WIN32_WCE
			sprintf(str, "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
#else
			sprintf_s(str, 37, "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
#endif
				Data1, Data2, Data3,
				Data4[0], Data4[1], Data4[2], Data4[3],
				Data4[4], Data4[5], Data4[6], Data4[7]);
			return TRUE;
		}

		BOOL ID2STR(BSTR str) const
		{
			if (str == NULL)
				return FALSE;
			StringFromGUID2(*this, str, 39);
			return TRUE;
		}


		// Web service bstr converter
		ID & FromWSBSTR(const BSTR bstr)
		{

			DWORD guid[8] = { 0 };

			//	avoid buffer overflow
#ifdef _WIN32_WCE
			swscanf(bstr, L"%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x",
#else
			swscanf_s(bstr, L"%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x",
#endif
				&Data1, &Data2, &Data3,
				&guid[0], &guid[1], &guid[2], &guid[3],
				&guid[4], &guid[5], &guid[6], &guid[7]);

			for (int i = 0; i < 8; ++i)
				Data4[i] = (BYTE)guid[i];

			return *this;
		}

		ID & FromBSTR(const BSTR bstr)
		{
			if (wcschr(bstr, L'{') == NULL)
			{
				return FromWSBSTR(bstr);
			}
			else
			{

				DWORD guid[8] = { 0 };

				//	avoid buffer overflow
#ifdef _WIN32_WCE
				swscanf(bstr, L"{%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x}",
#else
				swscanf_s(bstr, L"{%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x}",
#endif
					&Data1, &Data2, &Data3,
					&guid[0], &guid[1], &guid[2], &guid[3],
					&guid[4], &guid[5], &guid[6], &guid[7]);

				for (int i = 0; i < 8; ++i)
					Data4[i] = (BYTE)guid[i];

				return *this;
			}
		}

		// read from formatted c string 
		ID & FromCstr(const char* string,   // please assert validation and format of string
			const char *format = "{0x%08x, 0x%04x, 0x%04x, {0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x}}")
		{

			DWORD guid[8] = { 0 };

			if (format == NULL)
				format = "{%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x}";
#ifdef _WIN32_WCE
			sscanf(string, format,
#else
			sscanf_s(string, format,
#endif
				&Data1, &Data2, &Data3,
				&guid[0], &guid[1], &guid[2], &guid[3],
				&guid[4], &guid[5], &guid[6], &guid[7]);

			for (int i = 0; i < 8; ++i)
				Data4[i] = (BYTE)guid[i];

			return *this;
		}

		//friend IIOBuffer& operator << (IIOBuffer& mybuf, ID& myid);

		//friend ID& operator >> (ID& myid, IIOBuffer& mybuf);

	};

	static const UUID ID_NULL = { 0x0, 0x0, 0x0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
	static const UUID ID_NOT_SPECIFIED = { 0x0, 0x0, 0x0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

	static const UUID UDPSERVERID = { 0xF1225B0E, 0x4DC7, 0x4e78
		, { 0x87, 0xE3, 0xDF, 0x2D, 0xCB, 0xC9, 0x82, 0x22 } };

	typedef std::vector<ID> IDLIST;
	typedef std::vector<ID>	IDArray;
	typedef std::set<ID> IDSET;

	_tstring ToString(const ID & id);
	_tstring ToString(const GUID & guid);
	_tstring ToStringWithoutBraces(const ID & id);
	_tstring ToStringWithoutBraces(const GUID & guid);

}
#endif // __GUID__
