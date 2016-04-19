#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <strstream>
#include <sstream>
#include <stdio.h>
#include <tchar.h>

//! TString types
#ifdef _UNICODE

typedef std::wstring             _tstring;
typedef std::wfstream           _tfstream;
typedef std::wostringstream     _tostringstream;
#else

typedef std::string     _tstring;
typedef std::fstream    _tfstream;
typedef std::ostringstream    _tostringstream;

#endif


namespace OS
{


	_tstring StringA2T(const char * lpcstrString);

	std::string StringT2A(const _tstring & tstr);

	_tstring StringW2T(const wchar_t * lpwcstrString);

	std::wstring StringT2W(const _tstring & tstr);

	std::string StringT2UTF8(const _tstring & tstr);

	_tstring HexToString(const unsigned char *pBuffer, size_t iBytes);

	void StringToHex(const _tstring &ts, unsigned char *pBuffer, size_t nBytes);

	_tstring StripTrailing(const _tstring &source, const char toStrip);

	_tstring StripLeading(const _tstring &source, const char toStrip);

	_tstring DumpData(const unsigned char * const pData, size_t dataLength, size_t lineLength = 0);

	_tstring ToHex(const unsigned char c);

	void SplitString(const _tstring & src, const TCHAR delim, std::vector<_tstring> & strArr);

	template <class T>
	_tstring ToString(T num)
	{
		_tstring strNum = _T("");

		{
			std::strstream buf;

			buf << num << std::ends;

			strNum = StringA2T(buf.str());

			buf.freeze(false);
		}
		return strNum;
	};

}
