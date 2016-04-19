// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the XIAOZAIFILTERS_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// XIAOZAIFILTERS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once
#include "Config.h"
#include "ISuperSettings.h"

// This class is exported from the XiaozaiFilters.dll
class XIAOZAIFILTERS_API CXiaozaiFilters {
	ISuperSettings * m_SuperSetting = NULL;
public:
	CXiaozaiFilters(void);
	~CXiaozaiFilters();
	ISuperSettings * GetSettingInterface()
	{
		return m_SuperSetting;
	};
	// TODO: add your methods here.
	int Test(HWND hwnd);
	static bool Init(int loglevel);
	static bool Uninit();
};


