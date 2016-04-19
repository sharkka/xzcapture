/******************************************************************************************************************************
* File Name   : d:\work\stage\testsrc\AVECodeTest\DSAudio\AudioVolume.h
* File Path   : d:\work\stage\testsrc\AVECodeTest\DSAudio
* File Base   : AudioVolume
* Brief       : 
* Key Words   :
* File Ext    : h
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/31 10:31:23
* MODIFY DATE : 2016/03/31 10:31:23
******************************************************************************************************************************/
#ifndef _CMicVolume_H_
#define _CMicVolume_H_

/******************************************************************************************************************************
* Class Name  : CAudioVolume
* File Path   : d:\work\stage\testsrc\AVECodeTest\DSAudio
* File Base   : AudioVolume
* Brief       : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/31 10:31:43
* Modify Date : 2016/03/31 10:31:43
******************************************************************************************************************************/
class CAudioVolume : CObject
{
public: 
    CAudioVolume();
    ~CAudioVolume();

    int iCurIndex;
    CStringArray strarr;
    CDWordArray intarr;

	bool	GetDeviceList();
    bool	GetVolume();
    bool	SetVolume(int iVol);

	static float	GetSystemVolume();
	static void		SetSystemVolume(float vol);
	static float	GetSystemMicVolume();
	static void		SetSystemMicVolume(float vol);
    
	int		iMicVol;
    int		iMixMaxVol;
    int		iMicOldVol;
};
#endif  // _CMicVolume_H_