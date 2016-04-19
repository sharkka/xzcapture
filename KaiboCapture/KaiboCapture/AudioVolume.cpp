/******************************************************************************************************************************
* File Name   : d:\work\stage\testsrc\AVECodeTest\DSAudio\AudioVolume.cpp
* File Path   : d:\work\stage\testsrc\AVECodeTest\DSAudio
* File Base   : AudioVolume
* Brief       : 
* Key Words   :
* File Ext    : cpp
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/31 10:32:04
* MODIFY DATE : 2016/03/31 10:32:04
******************************************************************************************************************************/
#include "StdAfx.h"
#include "AudioVolume.h"
#include <mmsystem.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <ComHelper.h>
#include <iostream>
#include <functiondiscoverykeys.h>

#pragma comment(lib, "winmm.lib")



CAudioVolume::CAudioVolume()
{
    iCurIndex = 1;
    iMicVol = 0;
    iMixMaxVol = 65535;
}

CAudioVolume::~CAudioVolume()
{

}

bool CAudioVolume::GetDeviceList()
{
	int iMixerCount = 0;
	MIXERCAPS mcDAT;
	HMIXER hmDAT;
	MIXERLINE mlDAT;
	iMixerCount = ::mixerGetNumDevs();
	if (iMixerCount <=0) {
		printf("%s failed.\n", __FUNCTION__);
		return false;
	}
	
	for (int i = 0; i < iMixerCount; i ++) {
		memset(&mcDAT, 0x00, sizeof(MIXERCAPS));
		if (::mixerGetDevCaps(i, &mcDAT, sizeof(MIXERCAPS)) == MMSYSERR_NOERROR) {
			if (mcDAT.cDestinations != 0) {
				if (::mixerOpen(&hmDAT, i, 0, 0, MIXER_OBJECTF_MIXER) == MMSYSERR_NOERROR) {
					memset(&mlDAT, 0x00, sizeof(MIXERLINE));
					mlDAT.cbStruct = sizeof(MIXERLINE);
					mlDAT.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

					if (::mixerGetLineInfo((HMIXEROBJ)hmDAT, &mlDAT,
						MIXER_GETLINEINFOF_COMPONENTTYPE | MIXER_OBJECTF_HMIXER ) == MMSYSERR_NOERROR )	{
							strarr.Add(mcDAT.szPname);
							wprintf(_T("%s\n"), mcDAT.szPname);
							intarr.Add(i);
					}
					mixerClose(hmDAT);
				}
			}
		}
	}

	return true;
}

bool CAudioVolume::GetVolume()
{
    MIXERCAPS mcDAT;
    HMIXER hmDAT;
    MIXERLINE mlDAT;
    MIXERLINECONTROLS mlCTRL;
    MIXERCONTROL mCTRL;
    MIXERCONTROLDETAILS mcdDAT;
    MIXERCONTROLDETAILS_UNSIGNED mcduDAT;
    bool bRet = false;
        
    iMicVol = 0;
    iMixMaxVol = 65535;

    memset(&mcDAT, 0x00, sizeof(MIXERCAPS));
    if (::mixerGetDevCaps(iCurIndex, &mcDAT, sizeof(MIXERCAPS)) == MMSYSERR_NOERROR)  
    {
       if (::mixerOpen(&hmDAT, iCurIndex, 0, 0, MIXER_OBJECTF_MIXER) == MMSYSERR_NOERROR)    
       {
            memset(&mlDAT, 0x00, sizeof(MIXERLINE));
            mlDAT.cbStruct = sizeof(MIXERLINE);
            mlDAT.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

            if (::mixerGetLineInfo((HMIXEROBJ)hmDAT, &mlDAT,
               MIXER_GETLINEINFOF_COMPONENTTYPE | MIXER_OBJECTF_HMIXER ) == MMSYSERR_NOERROR ) 
            {
                memset(&mlCTRL, 0x00, sizeof(MIXERLINECONTROLS));
                memset(&mCTRL, 0x00, sizeof(MIXERCONTROL));
                mlCTRL.cbStruct = sizeof(MIXERLINECONTROLS);
                mlCTRL.dwLineID = mlDAT.dwLineID;
                mlCTRL.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
                mlCTRL.cControls = 1 ;
                mlCTRL.pamxctrl = &mCTRL;
                mlCTRL.cbmxctrl = sizeof(MIXERCONTROL);
                if (::mixerGetLineControls((HMIXEROBJ)hmDAT, &mlCTRL,
                   MIXER_GETLINECONTROLSF_ONEBYTYPE | MIXER_OBJECTF_HMIXER) == MMSYSERR_NOERROR)   
                {
                    memset(&mcdDAT, 0x00, sizeof(MIXERCONTROLDETAILS));
                    memset(&mcduDAT, 0x00, sizeof(MIXERCONTROLDETAILS_UNSIGNED));
                    mcdDAT.cbStruct = sizeof(MIXERCONTROLDETAILS);
                    mcdDAT.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
                    mcdDAT.paDetails = &mcduDAT;
                    mcdDAT.dwControlID = mCTRL.dwControlID;
                    mcdDAT.cChannels = (mCTRL.fdwControl | MIXERCONTROL_CONTROLF_UNIFORM ? 1 : mlDAT.cChannels);
                    if ((mCTRL.fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE))   
                    {
                        mcdDAT.cMultipleItems = mCTRL.cMultipleItems;
                    }
                    if (mixerGetControlDetails((HMIXEROBJ)hmDAT, &mcdDAT, MIXER_SETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)    
                    {

                        iMicVol = mcduDAT.dwValue;
						printf("volume: %d\n", iMicVol * 100 / iMixMaxVol + 1);
                        iMixMaxVol = mCTRL.Bounds.dwMaximum;

                    }
                }
            }
            ::mixerClose(hmDAT);
            bRet = true;
        }
    }
    return bRet;
}
bool CAudioVolume::SetVolume(int iVol)
{
	MIXERCAPS mcDAT;
	HMIXER hmDAT;
	MIXERLINE mlDAT;
	MIXERLINECONTROLS mlCTRL;
	MIXERCONTROL mCTRL;
	MIXERCONTROLDETAILS mcdDAT;
	MIXERCONTROLDETAILS_UNSIGNED mcduDAT;
	bool bRet = false;

	iMicVol = 0;
	iMixMaxVol = 65535;

	memset(&mcDAT, 0x00, sizeof(MIXERCAPS));
	if (::mixerGetDevCaps(iCurIndex, &mcDAT, sizeof(MIXERCAPS)) == MMSYSERR_NOERROR) {
		if (::mixerOpen(&hmDAT, iCurIndex, 0, 0, MIXER_OBJECTF_MIXER) == MMSYSERR_NOERROR) {
			memset(&mlDAT, 0x00, sizeof(MIXERLINE));
			mlDAT.cbStruct = sizeof(MIXERLINE);
			mlDAT.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

			if (::mixerGetLineInfo((HMIXEROBJ)hmDAT, &mlDAT,
				MIXER_GETLINEINFOF_COMPONENTTYPE | MIXER_OBJECTF_HMIXER ) == MMSYSERR_NOERROR ) {
				memset(&mlCTRL, 0x00, sizeof(MIXERLINECONTROLS));
				memset(&mCTRL, 0x00, sizeof(MIXERCONTROL));
				mlCTRL.cbStruct = sizeof(MIXERLINECONTROLS);
				mlCTRL.dwLineID = mlDAT.dwLineID;
				mlCTRL.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
				mlCTRL.cControls = 1 ;
				mlCTRL.pamxctrl = &mCTRL;
				mlCTRL.cbmxctrl = sizeof(MIXERCONTROL);
				if (::mixerGetLineControls((HMIXEROBJ)hmDAT, &mlCTRL,
					MIXER_GETLINECONTROLSF_ONEBYTYPE | MIXER_OBJECTF_HMIXER) == MMSYSERR_NOERROR) {
					memset(&mcdDAT, 0x00, sizeof(MIXERCONTROLDETAILS));
					memset(&mcduDAT, 0x00, sizeof(MIXERCONTROLDETAILS_UNSIGNED));
					mcdDAT.cbStruct = sizeof(MIXERCONTROLDETAILS);
					mcdDAT.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
					mcduDAT.dwValue = iVol;
					mcdDAT.paDetails = &mcduDAT;
					mcdDAT.dwControlID = mCTRL.dwControlID;
					mcdDAT.cChannels = (mCTRL.fdwControl | MIXERCONTROL_CONTROLF_UNIFORM ? 1 : mlDAT.cChannels);
					if ((mCTRL.fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE)) {
						mcdDAT.cMultipleItems = mCTRL.cMultipleItems;
					}
					if (mixerSetControlDetails((HMIXEROBJ)hmDAT, &mcdDAT, 
						MIXER_SETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER ) != MMSYSERR_NOERROR) {
						bRet = false;
					}
				}
			}
			::mixerClose(hmDAT);
			bRet = true;
		}
	}
	return bRet;
}

/******************************************************************************************************************************
* Function    : GetSystemVolume
* File Path   : d:\work\stage\testsrc\AVECodeTest\DSAudio
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/31 9:59:23
* Modify Date : 2016/03/31 9:59:23
******************************************************************************************************************************/
float CAudioVolume::GetSystemVolume()
{
	float					vol			= 0.0F;
	HRESULT					hr			= S_OK;
	IAudioEndpointVolume*	volume		= NULL;
	IMMDeviceEnumerator*	enumerator	= NULL;
	IMMDeviceCollection*	collection	= NULL;
	IPropertyStore*			props		= NULL;
	LPWSTR					wsz_id		= NULL;
	IMMDevice*				endpoint	= NULL;

	CoInitialize(NULL);
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&enumerator);

	GOTO_EXIT_IF_FAILED(hr)
		hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &endpoint);
	GOTO_EXIT_IF_FAILED(hr)
		hr = endpoint->GetId(&wsz_id);
	GOTO_EXIT_IF_FAILED(hr)
		hr = endpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **)(&volume));
	GOTO_EXIT_IF_FAILED(hr)
		hr = volume->GetMasterVolumeLevelScalar(&vol);
	GOTO_EXIT_IF_FAILED(hr)

	return vol;
Exit:
	CoTaskMemFree(wsz_id);
	SAFE_RELEASE(enumerator)
	SAFE_RELEASE(collection)
	SAFE_RELEASE(endpoint)
	SAFE_RELEASE(props)

	printf("Get system volume failed.\n");

	return 0.0;
}
/******************************************************************************************************************************
* Function    : SetSystemVolume
* File Path   : d:\work\stage\testsrc\AVECodeTest\DSAudio
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/31 9:59:30
* Modify Date : 2016/03/31 9:59:30
******************************************************************************************************************************/
void CAudioVolume::SetSystemVolume(float vol)
{
	HRESULT					hr			= S_OK;
	IAudioEndpointVolume*	volume		= NULL;
	IMMDeviceEnumerator*	enumerator	= NULL;
	IMMDeviceCollection*	collection	= NULL;
	IPropertyStore*			props		= NULL;
	LPWSTR					wsz_id		= NULL;
	IMMDevice*				endpoint	= NULL;

	CoInitialize(NULL);
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&enumerator);

	GOTO_EXIT_IF_FAILED(hr)
		hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &endpoint);
	GOTO_EXIT_IF_FAILED(hr)
		hr = endpoint->GetId(&wsz_id);
	GOTO_EXIT_IF_FAILED(hr)
		hr = endpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **)(&volume));
	GOTO_EXIT_IF_FAILED(hr)
		hr = volume->SetMasterVolumeLevelScalar(vol, NULL);
	GOTO_EXIT_IF_FAILED(hr)

		return;
Exit:
	CoTaskMemFree(wsz_id);
	SAFE_RELEASE(enumerator)
	SAFE_RELEASE(collection)
	SAFE_RELEASE(endpoint)
	SAFE_RELEASE(props)

	printf("Set system volume failed.\n");

	return;
}
/******************************************************************************************************************************
* Function    : GetSystemMicVolume
* File Path   : d:\work\stage\testsrc\AVECodeTest\DSAudio
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/31 10:10:30
* Modify Date : 2016/03/31 10:10:30
******************************************************************************************************************************/
float CAudioVolume::GetSystemMicVolume()
{
	float					vol			= 0.0F;
	HRESULT					hr			= S_OK;
	IAudioEndpointVolume*	volume		= NULL;
	IMMDeviceEnumerator*	enumerator	= NULL;
	IMMDeviceCollection*	collection	= NULL;
	IPropertyStore*			props		= NULL;
	LPWSTR					wsz_id		= NULL;
	IMMDevice*				endpoint	= NULL;

	CoInitialize(NULL);
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&enumerator);

	GOTO_EXIT_IF_FAILED(hr)
		hr = enumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, &endpoint);
	GOTO_EXIT_IF_FAILED(hr)
		hr = endpoint->GetId(&wsz_id);
	GOTO_EXIT_IF_FAILED(hr)
		hr = endpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **)(&volume));
	GOTO_EXIT_IF_FAILED(hr)
		hr = volume->GetMasterVolumeLevelScalar(&vol);
	GOTO_EXIT_IF_FAILED(hr)

		return vol;
Exit:
	CoTaskMemFree(wsz_id);
	SAFE_RELEASE(enumerator)
	SAFE_RELEASE(collection)
	SAFE_RELEASE(endpoint)
	SAFE_RELEASE(props)

	printf("Get system micphone volume failed.\n");

	return 0.0;
}
/******************************************************************************************************************************
* Function    : SetSystemMicVolume
* File Path   : d:\work\stage\testsrc\AVECodeTest\DSAudio
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/31 10:10:37
* Modify Date : 2016/03/31 10:10:37
******************************************************************************************************************************/
void CAudioVolume::SetSystemMicVolume(float vol)
{
	HRESULT					hr			= S_OK;
	IAudioEndpointVolume*	volume		= NULL;
	IMMDeviceEnumerator*	enumerator	= NULL;
	IMMDeviceCollection*	collection	= NULL;
	IPropertyStore*			props		= NULL;
	LPWSTR					wsz_id		= NULL;
	IMMDevice*				endpoint	= NULL;

	CoInitialize(NULL);
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&enumerator);

	GOTO_EXIT_IF_FAILED(hr)
		hr = enumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, &endpoint);
	GOTO_EXIT_IF_FAILED(hr)
		hr = endpoint->GetId(&wsz_id);
	GOTO_EXIT_IF_FAILED(hr)
		hr = endpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **)(&volume));
	GOTO_EXIT_IF_FAILED(hr)
		hr = volume->SetMasterVolumeLevelScalar(vol, NULL);
	GOTO_EXIT_IF_FAILED(hr)

		return;
Exit:
	CoTaskMemFree(wsz_id);
	SAFE_RELEASE(enumerator)
		SAFE_RELEASE(collection)
		SAFE_RELEASE(endpoint)
		SAFE_RELEASE(props)

		printf("Set system micphone volume failed.\n");

	return;
}

