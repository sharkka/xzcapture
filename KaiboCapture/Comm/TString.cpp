#include "stdafx.h"
#include "TString.h"

namespace OS
{

	_tstring StringW2T(const wchar_t * lpwcstrString)
	{
		_tstring strRes;

#ifdef UNICODE

		strRes = lpwcstrString;

#else

		size_t sz;

		wcstombs_s(&sz, NULL, 0, lpwcstrString,	0);

		char * pstr = new char[sz+1];

		wcstombs_s(&sz, pstr, sz+1, lpwcstrString, sz);

		strRes = pstr;

		delete [] pstr;

#endif

		return strRes;
	}

	std::wstring StringT2W(const _tstring & tstr)
	{
		std::wstring wstrRes;

#ifdef UNICODE

		wstrRes = tstr;

#else

		size_t sz;

		mbstowcs_s(&sz, NULL, 0, tstr.c_str(),	0);

		wchar_t * pwstr = new wchar_t[sz+1];

		mbstowcs_s(&sz, pwstr, sz+1, tstr.c_str(), sz);

		wstrRes = pwstr;

		delete [] pwstr;

#endif

		return wstrRes;
	}


	_tstring StringA2T(const char * lpcstrString)
	{
		_tstring strRes;

#ifdef UNICODE
		size_t sz;

		mbstowcs_s(&sz, NULL, 0, lpcstrString, 0);

		wchar_t * pwstr = new wchar_t[sz + 1];

		mbstowcs_s(&sz, pwstr, sz + 1, lpcstrString, sz);

		strRes = pwstr;

		delete[] pwstr;

#else

		strRes = lpcstrString;

#endif

		return strRes;
	}

	std::string StringT2UTF8(const _tstring & tstr)
	{
		std::string strRes;

#ifdef UNICODE
		size_t sz =	WideCharToMultiByte(CP_UTF8, 0, tstr.c_str(),-1,0,0,0,0);

		char * pstr = new char[sz + 1];

		WideCharToMultiByte(CP_UTF8, 0, tstr.c_str(),-1,pstr,sz,0,0);

		strRes = pstr;
		delete[] pstr;

#else

		strRes = tstr;

#endif	

		return strRes;
	}

	std::string StringT2A(const _tstring & tstr)
	{
		std::string strRes;

#ifdef UNICODE
		size_t sz;

		wcstombs_s(&sz, NULL, 0, tstr.c_str(), 0);

		char * pstr = new char[sz + 1];

		wcstombs_s(&sz, pstr, sz + 1, tstr.c_str(), sz);
		
		strRes = pstr;
		delete[] pstr;

#else

		strRes = tstr;

#endif	

		return strRes;

	}


	_tstring HexToString(const unsigned char *pBuffer, size_t iBytes)
	{
		_tstring result;

		for (size_t i = 0; i < iBytes; i++)
		{
			unsigned char c;

			unsigned char b = pBuffer[i] >> 4;

			if (9 >= b)
			{
				c = b + '0';
			}
			else
			{
				c = (b - 10) + 'A';
			}

			result += (TCHAR)c;

			b = pBuffer[i] & 0x0f;

			if (9 >= b)
			{
				c = b + '0';
			}
			else
			{
				c = (b - 10) + 'A';
			}

			result += (TCHAR)c;
		}

		return result;
	}

	void StringToHex(const _tstring &ts, unsigned char *pBuffer, size_t nBytes)
	{
		const std::string s = StringT2A(const_cast<TCHAR *>(ts.c_str()));

		for (size_t i = 0; i < nBytes; i++)
		{
			const size_t stringOffset = i * 2;

			unsigned char val = 0;

			const unsigned char b = s[stringOffset];

			if (isdigit(b))
			{
				val = (unsigned char)((b - '0') * 16);
			}
			else
			{
				val = (unsigned char)(((toupper(b) - 'A') + 10) * 16);
			}

			const unsigned char b1 = s[stringOffset + 1];

			if (isdigit(b1))
			{
				val += b1 - '0';
			}
			else
			{
				val += (unsigned char)((toupper(b1) - 'A') + 10);
			}

			pBuffer[i] = val;
		}
	}


	_tstring ToHex(const unsigned char c)
	{
		TCHAR hex[3];

		const int val = c;

		_stprintf_s(hex, 3, _T("%02X"), val);

		return hex;
	}

	_tstring DumpData(const unsigned char * const pData, size_t dataLength, size_t lineLength /* = 0 */)
	{
		const size_t bytesPerLine = lineLength != 0 ? (lineLength - 1) / 3 : 0;

		_tstring result;

		_tstring hexDisplay;
		_tstring display;

		size_t i = 0;

		while (i < dataLength)
		{
			const unsigned char c = pData[i++];

			hexDisplay += ToHex(c) + _T(" ");

			if (isprint(c))
			{
				display += (TCHAR)c;
			}
			else
			{
				display += _T('.');
			}

			if ((bytesPerLine && (i % bytesPerLine == 0 && i != 0)) || i == dataLength)
			{
				if (i == dataLength && (bytesPerLine && (i % bytesPerLine != 0)))
				{
					for (size_t pad = i % bytesPerLine; pad < bytesPerLine; pad++)
					{
						hexDisplay += _T("   ");
					}
				}
				result += hexDisplay + _T(" - ") + display + _T("\n");

				hexDisplay = _T("");
				display = _T("");
			}
		}

		return result;
	}

	_tstring StripLeading(const _tstring &source, const char toStrip)
	{
		const TCHAR *pSrc = source.c_str();

		while (pSrc && *pSrc == toStrip)
		{
			++pSrc;
		}

		return pSrc;
	}

	_tstring StripTrailing(const _tstring &source, const char toStrip)
	{
		size_t i = source.length();
		const TCHAR *pSrc = source.c_str() + i;

		--pSrc;

		while (i && *pSrc == toStrip)
		{
			--pSrc;
			--i;
		}

		return source.substr(0, i);
	}

	void SplitString(const _tstring & src, const TCHAR delim, std::vector<_tstring> & strArr)
	{
		strArr.clear();

		int i = 0, j;
		int k = static_cast<int>(src.length());

		const TCHAR * pSrc = src.c_str();

		while (i < k)
		{
			while (*pSrc == delim && i < k)
			{
				pSrc++;
				i++;
			}

			j = i;
			while (*pSrc != delim && i < k)
			{
				pSrc++;
				i++;
			}

			if (j < i)
			{
				strArr.push_back(src.substr(j, i - j));
			}
		}
	}

//NAMESPACE
}