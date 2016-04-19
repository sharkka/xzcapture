#pragma once

#include "Exception.h"

namespace OS	{

_tstring GetLastErrorMessage(DWORD last_error);

_tstring GetDateStamp();

void TraceLastError();

#ifndef _WIN32_WCE
    _tstring GetCurrentDirectory();

    _tstring GetUserName();

    _tstring GetComputerName();
#endif // _WIN32_WCE

_tstring GetModuleFileName(HINSTANCE hModule = 0);

_tstring GetFileVersion();

BOOL WriteProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry,
	LPBYTE pData, UINT nBytes);
BOOL GetProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry,
	BYTE** ppData, UINT* pBytes);

DWORD RegQueryDword(LPCTSTR item, DWORD default_value);
_tstring RegQueryString(LPCTSTR item, LPCTSTR default_value);
BOOL RegSetDword(LPCTSTR item, DWORD value);
BOOL RegSetString(LPCTSTR item, _tstring);

BOOL GetTempPathLow(DWORD ccBuffer, LPTSTR lpszBuffer);

BOOL	GetASFPacketInterval (const BYTE *pData, 
                              DWORD *pInterval,
                              DWORD *pPacketSize,
                              DWORD *pBitrate);
}