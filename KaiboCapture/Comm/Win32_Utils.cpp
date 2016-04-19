#include "stdafx.h"

#include <cassert>
#include <memory>

using std::auto_ptr;

#include "Win32_Utils.h"
#include "ID.h"
#include "mfobjects.h"
#include <VersionHelpers.h>

namespace OS {

_tstring GetLastErrorMessage(DWORD last_error)
{
    static TCHAR errmsg[512];

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                       0,
                       last_error,
                       0,
                       errmsg,
                       511,
                       NULL);

    return errmsg;
}

void TraceLastError()
{
	OutputDebugString(GetLastErrorMessage(GetLastError()).c_str());
}


_tstring GetDateStamp()
{
    SYSTEMTIME systime;
    GetSystemTime(&systime);

    static TCHAR buffer[7];

    _stprintf_s(buffer, sizeof(buffer)/sizeof(TCHAR),  _T("%02d%02d%02d"), systime.wDay, systime.wMonth, (1900 + systime.wYear) % 100);

    return buffer;
}

#ifndef _WIN32_WCE
    #pragma comment(lib, "Version.lib")
#endif

_tstring GetFileVersion()
{
    _tstring version;

    const _tstring moduleFileName = GetModuleFileName(NULL);

    LPTSTR pModuleFileName = const_cast<LPTSTR>(moduleFileName.c_str());

    DWORD zero = 0;

    DWORD verSize = ::GetFileVersionInfoSize(pModuleFileName, &zero);

    if (verSize != 0)
    {
        auto_ptr<BYTE> spBuffer(new BYTE[verSize]);

        if (::GetFileVersionInfo(pModuleFileName, 0, verSize, spBuffer.get()))
        {
            LPTSTR pVersion = 0;
            UINT verLen = 0;

            if (::VerQueryValue(spBuffer.get(),
                                const_cast<LPTSTR>(_T("\\StringFileInfo\\080904b0\\ProductVersion")),
                                (void**)&pVersion,
                                &verLen))
            {
                version = pVersion;
            }
        }
    }

    return version;
}

// returns key for HKEY_CURRENT_USER\"Software"\RegistryKey\ProfileName
// creating it if it doesn't exist
// responsibility of the caller to call RegCloseKey() on the returned HKEY
HKEY GetAppRegistryKey()
{
	LPCTSTR pszRegistryKey = _T("CCTV");
	LPCTSTR pszProfileName = _T("SVS");

	HKEY hAppKey = NULL;
	HKEY hSoftKey = NULL;
	HKEY hCompanyKey = NULL;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("software"), 0, KEY_WRITE|KEY_READ,
		&hSoftKey) == ERROR_SUCCESS)
	{
		DWORD dw;
		if (RegCreateKeyEx(hSoftKey, pszRegistryKey, 0, REG_NONE,
			REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
			&hCompanyKey, &dw) == ERROR_SUCCESS)
		{
			RegCreateKeyEx(hCompanyKey, pszProfileName, 0, REG_NONE,
				REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
				&hAppKey, &dw);
		}
	}
	if (hSoftKey != NULL)
		RegCloseKey(hSoftKey);
	if (hCompanyKey != NULL)
		RegCloseKey(hCompanyKey);

	return hAppKey;
}

// returns key for:
//      HKEY_CURRENT_USER\"Software"\RegistryKey\AppName\lpszSection
// creating it if it doesn't exist.
// responsibility of the caller to call RegCloseKey() on the returned HKEY
HKEY GetSectionKey(LPCTSTR lpszSection)
{
	assert(lpszSection != NULL);

	HKEY hSectionKey = NULL;
	HKEY hAppKey = GetAppRegistryKey();
	if (hAppKey == NULL)
		return NULL;

	DWORD dw;
	RegCreateKeyEx(hAppKey, lpszSection, 0, REG_NONE,
		REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hSectionKey, &dw);
	RegCloseKey(hAppKey);
	return hSectionKey;
}

HKEY GetCommonSettingRegistryKey(HKEY root, BOOL isWritable)
{
	HKEY hAppKey = NULL;
	if (!isWritable)
	{
		RegOpenKeyEx(
			root,
			_T("Software\\CCTV\\P2P Streaming"),
			0,
			KEY_READ,
			&hAppKey);
	}
	else
	{
		RegCreateKeyEx(
			root,
			_T("Software\\CCTV\\P2P Streaming"),
			0,
			NULL,
			0,
			KEY_READ | KEY_WRITE,
			NULL,
			&hAppKey,
			NULL);
	}
	return hAppKey;
}

BOOL _RegReadDword(HKEY root, LPCTSTR item, DWORD* value)
{
	HKEY regKey = GetCommonSettingRegistryKey(root, FALSE);
	if (regKey == NULL)
		return FALSE;
	DWORD len = sizeof(DWORD);
	DWORD flag = (ERROR_SUCCESS == RegQueryValueEx(regKey, item, NULL, NULL, (BYTE*)value, &len));
	RegCloseKey(regKey);
	return flag;
}

DWORD RegQueryDword(LPCTSTR item, DWORD default_value)
{
	DWORD value = 0;
	if (_RegReadDword(HKEY_CURRENT_USER, item, &value) ||
		_RegReadDword(HKEY_LOCAL_MACHINE, item, &value))
		return value;
	return default_value;
}

BOOL _RegReadString(HKEY root, LPCTSTR item, _tstring* value)
{
	HKEY regKey = GetCommonSettingRegistryKey(root, FALSE);
	if (regKey == NULL)
		return FALSE;
	TCHAR ret[2048];	// maximum allowed length of registry value
	DWORD len = sizeof(ret);
	BOOL flag = (ERROR_SUCCESS == RegQueryValueEx(regKey, item, NULL, NULL, (BYTE*)ret, &len));
	RegCloseKey(regKey);
	if (flag) *value = ret;
	return flag;
}

_tstring RegQueryString(LPCTSTR item, LPCTSTR default_value)
{
	_tstring value;
	if (_RegReadString(HKEY_CURRENT_USER, item, &value) ||
		_RegReadString(HKEY_LOCAL_MACHINE, item, &value))
		return value;
	return default_value;
}

BOOL RegSetDword(LPCTSTR item, DWORD value)
{
	HKEY regKey = GetCommonSettingRegistryKey(HKEY_CURRENT_USER, TRUE);
	if (regKey == NULL)
		return FALSE;
	BOOL flag = (ERROR_SUCCESS == RegSetValueEx(regKey, item, 0, REG_DWORD, (BYTE*) &value, sizeof(value)));
	RegCloseKey(regKey);
	return flag;
}

BOOL RegSetString(LPCTSTR item, _tstring value)
{
	HKEY regKey = GetCommonSettingRegistryKey(HKEY_CURRENT_USER, TRUE);
	if (regKey == NULL)
		return FALSE;

	BOOL flag = (ERROR_SUCCESS == RegSetValueEx(regKey, item, 0, REG_SZ, (BYTE*) value.c_str(), ((DWORD)value.size() + 1) * sizeof(value[0])));

	RegCloseKey(regKey);
	return flag;

}

BOOL GetProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry,
	BYTE** ppData, UINT* pBytes)
{
	assert(lpszSection != NULL);
	assert(lpszEntry != NULL);
	assert(ppData != NULL);
	assert(pBytes != NULL);
	*ppData = NULL;
	*pBytes = 0;
	HKEY hSecKey = GetSectionKey(lpszSection);
	if (hSecKey == NULL)
	{
		return FALSE;
	}


	DWORD dwType=0;
	DWORD dwCount=0; 
	LONG lResult = RegQueryValueEx(hSecKey, (LPTSTR)lpszEntry, NULL, &dwType, NULL, &dwCount);
	*pBytes = dwCount;
	if (lResult == ERROR_SUCCESS)
	{
		assert(dwType == REG_BINARY);
		*ppData = new BYTE[*pBytes];
		lResult = RegQueryValueEx(hSecKey, (LPTSTR)lpszEntry, NULL, &dwType,
			*ppData, &dwCount);
	}
	RegCloseKey(hSecKey);
	if (lResult == ERROR_SUCCESS)
	{
		assert(dwType == REG_BINARY);
		return TRUE;
	}
	else
	{
		delete [] *ppData;
		*ppData = NULL;
	}
	return FALSE;
}

BOOL WriteProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry,
	LPBYTE pData, UINT nBytes)
{
	assert(lpszSection != NULL);
	LONG lResult;
	HKEY hSecKey = GetSectionKey(lpszSection);
	if (hSecKey == NULL)
		return FALSE;
	lResult = RegSetValueEx(hSecKey, lpszEntry, NULL, REG_BINARY,
		pData, nBytes);
	RegCloseKey(hSecKey);
	return lResult == ERROR_SUCCESS;
}

#ifndef _WIN32_WCE

_tstring GetCurrentDirectory()
{
    DWORD size = ::GetCurrentDirectory(0, 0);

    auto_ptr<TCHAR> spBuf(new TCHAR[size]);

    if (0 == ::GetCurrentDirectory(size, spBuf.get()))
    {
        throw CGeneralException(_T("GetCurrentDirectory()"), _T("Failed to get current directory"));
    }

    return _tstring(spBuf.get());
}

_tstring GetComputerName()
{
    static bool gotName = false;

    static _tstring name = _T("UNAVAILABLE");

    if (!gotName)
    {
        TCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1] ;
        DWORD computerNameLen = MAX_COMPUTERNAME_LENGTH ;

        if (::GetComputerName(computerName, &computerNameLen))
        {
            name = computerName;
        }

        gotName = true;
    }

    return name;
}

_tstring GetUserName()
{
    static bool gotName = false;

    static _tstring name = _T("UNAVAILABLE");

    if (!gotName)
    {
        TCHAR userName[256 + 1] ;
        DWORD userNameLen = 256;

        if (::GetUserName(userName, &userNameLen))
        {
            name = userName;
        }

        gotName = true;
    }

    return name;
}

#endif // _WIN32_WCE

_tstring GetModuleFileName(
                           HINSTANCE hModule /* = 0 */)
{
    static bool gotName = false;

    static _tstring name = _T("UNAVAILABLE");

    if (!gotName)
    {
        TCHAR moduleFileName[MAX_PATH + 1] ;
        DWORD moduleFileNameLen = MAX_PATH ;

        if (::GetModuleFileName(hModule, moduleFileName, moduleFileNameLen))
        {
            name = moduleFileName;
        }

        gotName = true;
    }

    return name;
}

LPCTSTR LoadResourceString(HANDLE hModule, UINT resourceId)
{
	static TCHAR message[1000];
	if (LoadString((HINSTANCE)hModule, resourceId, message,
				   sizeof(message) / sizeof(message[0])) <= 0)
	{
		// fail to get resource string
		message[0] = 0;
	}
	return message;
}

BOOL GetTempPathLow(DWORD ccBuffer, LPTSTR lpszBuffer)
{

	if (IsWindowsVistaOrGreater())
	{  // vista or 2k8
		if (ExpandEnvironmentStrings(L"%USERPROFILE%\\AppData\\Local\\Temp\\Low\\", lpszBuffer, ccBuffer))
		{  // try Local Low directory
			TCHAR tempName[MAX_PATH];
			if (GetTempFileName(lpszBuffer, L"gtpl", 0, tempName))
			{
				HANDLE handle = CreateFile(tempName, GENERIC_WRITE,
										   FILE_SHARE_READ, NULL,
										   CREATE_ALWAYS,
										   FILE_ATTRIBUTE_NORMAL, NULL);
				if (handle != NULL)
				{
					CloseHandle(handle);
					DeleteFile(tempName);
					return TRUE;
				}
			}
		}
	}

	return GetTempPath(ccBuffer, lpszBuffer);
}

/////////////////////////////////////////////////////////////////////
//
// this function is to get the ASF packet interval by parsing ASF header
// pData: pointer to ASF header
// pInterval: pointer to the value indicating ASF packet interval (in ms)
//
/////////////////////////////////////////////////////////////////////
BOOL	GetASFPacketInterval (const BYTE *pData, 
                              DWORD *pInterval,
                              DWORD *pPacketSize,
                              DWORD *pBitrate)
{
	DWORD	ObjectNum;
	DWORD	ObjectCounter = 0;
	ID		ObjectID;
	BSTR	ObjectIDWStr = _T("");
	QWORD	ObjectSize;
	QWORD	Offset = 0;
	DWORD	BitRate;
	DWORD	PacketSize;
	BSTR	ASF_File_Properties_Object = _T("{8CABDCA1-A947-11CF-8EE4-00C00C205365}");
	BSTR	ASF_Header_Object = _T("{75B22630-668E-11CF-A6D9-00AA0062CE6C}");

	//parse Header Object, if no Header object found, return false
	ObjectID = *((ID *)(pData + Offset));
	ObjectIDWStr = ObjectID.ID2BSTR();
	if (wcscmp(ObjectIDWStr, ASF_Header_Object))
	{
		*pInterval = 0;

        SysFreeString(ObjectIDWStr);
		return FALSE;
	}	
	ObjectNum = *((DWORD *)(pData + 24));
	Offset += 30;	//Header object's size is fixed to 30 byte

	//parse following objects in Header object, until file properties object is parsed
	while(wcscmp(ObjectIDWStr,ASF_File_Properties_Object))
	{
        SysFreeString(ObjectIDWStr);

		if (ObjectCounter == ObjectNum)
			break;
		ObjectID = *((ID *)(pData + Offset));
		ObjectIDWStr = ObjectID.ID2BSTR();
		ObjectSize = *((QWORD *)(pData + Offset + 16));
		Offset += ObjectSize;		
		ObjectCounter ++;
	}
    SysFreeString(ObjectIDWStr);

	//no file properties object found in header, return false
	if (ObjectCounter == ObjectNum)
	{
		*pInterval = 0;
		return FALSE;
	}

	BitRate = *((DWORD *)(pData + Offset - 4));
	PacketSize = *((DWORD *)(pData + Offset - 8));
	*pInterval = PacketSize * 8000 / BitRate;
    *pPacketSize = PacketSize;
    *pBitrate = BitRate;
	return TRUE;
}


}