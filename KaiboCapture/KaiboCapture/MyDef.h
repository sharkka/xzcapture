// content type
#define CT_LIVE_CONTENT 0
#define CT_FILE_CONTENT 1
#define CT_GRF_CONTENT 2
#define CT_UNDEFINE_CONTENT 255

// schedule type
#define ST_REPEAT 0
#define ST_EVENT 1

// Current Status
#define CS_PLAYING		0
#define CS_STOP			1

#define INVALIDE_DATE   1.0e7

#define START_DATE      2.0e-6

#define SCHTIMERANGE    (10)

// Channel status
#define S_NOVIDEO			((HRESULT)0x00000002L)
#define S_CONTINUE          ((HRESULT)0x00000003L)
#define E_INVALIDLIVEARG	E_UNEXPECTED + 1
#define E_INVALIDFILEARG	E_UNEXPECTED + 2
