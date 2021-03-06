#pragma once

// {A29626E9-5326-4059-919C-A0F277D92C17}
DEFINE_GUID(CLSID_XiaozaiFilter,
	0xa29626e9, 0x5326, 0x4059, 0x91, 0x9c, 0xa0, 0xf2, 0x77, 0xd9, 0x2c, 0x17);


// {CACBBB2A-B19D-4CD0-9E01-F7C9CA639CFC}
DEFINE_GUID(IID_IXiaozaiUploaderSettings,
0xcacbbb2a, 0xb19d, 0x4cd0, 0x9e, 0x1, 0xf7, 0xc9, 0xca, 0x63, 0x9c, 0xfc);

// {19197516-52C3-45E2-AA37-FFAE6E8A4EDF}
DEFINE_GUID(IID_IRecordProvider,
	0x19197516, 0x52c3, 0x45e2, 0xaa, 0x37, 0xff, 0xae, 0x6e, 0x8a, 0x4e, 0xdf);

// {4D474BEC-86FC-4681-9DC2-D034598D7D96}
DEFINE_GUID(IID_ISnapshotProvider,
	0x4d474bec, 0x86fc, 0x4681, 0x9d, 0xc2, 0xd0, 0x34, 0x59, 0x8d, 0x7d, 0x96);

// {3F317BE7-2CAF-4A8F-8D7D-3406609B2ABA}
DEFINE_GUID(IID_ISuperSettings,
	0x3f317be7, 0x2caf, 0x4a8f, 0x8d, 0x7d, 0x34, 0x6, 0x60, 0x9b, 0x2a, 0xba);


///
///  WMD Video/Audio Type definitions
///

#include "wmsdkidl.h"

DEFINE_GUID(WMMEDIASUBTYPE_MPG4, 
0x3447504D, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9B, 0x71);

//44495658-0000-0010-8000-00AA00389B71 
DEFINE_GUID(WMMEDIASUBTYPE_XVID, 
0x44495658, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9B, 0x71);

//00002000-0000-0010-8000-00AA00389B71
DEFINE_GUID(WMMEDIASUBTYPE_XAID, 
0x00002000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9B, 0x71);

//30345652-0000-0010-8000-00AA00389B71  Real Video 4.0
DEFINE_GUID(WMMEDIASUBTYPE_RV40, 
0x30345652, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9B, 0x71);

//4B4F4F43-0000-0010-8000-00AA00389B71  Real Audio 4.0
DEFINE_GUID(WMMEDIASUBTYPE_COOK, 
0x4B4F4F43, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9B, 0x71);

//DX50
DEFINE_GUID(WMMEDIASUBTYPE_DIVX_MPEG4, 
0x30355844, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9B, 0x71);

//h.263
DEFINE_GUID(WMMEDIASUBTYPE_H263, 
0x33363248, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9B, 0x71);

//DIVX
DEFINE_GUID(WMMEDIASUBTYPE_DIVX, 
0x58564944, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9B, 0x71);
