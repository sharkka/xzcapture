#include "stdafx.h"

#include <process.h>

#include "Thread.h"
#include "Exception.h"

namespace OS{

CThread::CThread()
    :  m_hThread(INVALID_HANDLE_VALUE), m_dwThreadID(0)
{
    m_running = false;
}

CThread::~CThread()
{
    if (m_hThread != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
    }
}

void CThread::Start()
{
    if (m_running)
        return;
    if (m_hThread == INVALID_HANDLE_VALUE)
    {
        m_hThread = svsBeginThread(NULL, 0, &ThreadFunction, (void*)this, 0, &m_dwThreadID);

        if (m_hThread == INVALID_HANDLE_VALUE)
        {
            throw CWin32Exception(_T("CThread::Start() - CreateThread"), GetLastError());
        }
        m_running = true;
    }
    else
    {
        throw CGeneralException(_T("CThread::Start()"), _T("Thread already running - you can only call Start() once!"));
    }
}

void CThread::Wait() const
{
    if (!Wait(INFINITE))
    {
        throw CGeneralException(_T("CThread::Wait()"), _T("Unexpected timeout on infinite wait"));
    }
}

bool CThread::Wait(DWORD timeoutMillis) const
{
    // TODO base class? Waitable?
    bool ok;

    DWORD result = ::WaitForSingleObject(m_hThread, timeoutMillis);

    if (result == WAIT_TIMEOUT)
    {
        ok = false;
    }
    else if (result == WAIT_OBJECT_0)
    {
        ok = true;
    }
    else
    {
        throw CWin32Exception(_T("CThread::Wait() - WaitForSingleObject"), ::GetLastError());
    }

    return ok;
}

DWORD __stdcall CThread::ThreadFunction(void *pV)
{
    DWORD result = 0;

    CThread* pThis = (CThread*)pV;

    if (pThis)
    {
        try
        {
            result = pThis->Run();
        }
        catch(...)
        {
        }
    }
    pThis->m_running = false;
    return result;
}

void CThread::Terminate(DWORD exitCode /* = 0 */)
{
    if (!::TerminateThread(m_hThread, exitCode))
    {
        // TODO we could throw an exception here...
    }
	m_hThread = INVALID_HANDLE_VALUE;
    m_running = false;
}

void CThread::PostMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	/*if (!::PostThreadMessage(m_dwThreadID, msg, wParam, lParam)) {
		throw CWin32Exception(_T("CThread::PostMessage()"), ::GetLastError());
	}*/
	//== CODE ADD BY ANYZ =================================================================//
	// Date: 2016/04/06 13:36:12
	if (0 == m_dwThreadID || INVALID_HANDLE_VALUE == m_hThread) {
		m_running = false;
		//Terminate(0);
	} else if (!::PostThreadMessage(m_dwThreadID, msg, wParam, lParam)) {
		printf("exception raised, %d\n", ::GetLastError());
		m_running = false;
		Terminate(0);
		//throw CWin32Exception(_T("CThread::PostMessage()"), ::GetLastError());
	}
	
	//== CODE END =========================================================================//
}

///////////////////////////////////////////////////////////////////////////////
// NETWORK::CCriticalSection
///////////////////////////////////////////////////////////////////////////////

CCriticalSection::CCriticalSection( DWORD dwSpinCount /* = 0 */)
{
    ::InitializeCriticalSectionAndSpinCount(&m_crit, dwSpinCount);
}

CCriticalSection::~CCriticalSection()
{
#ifdef _DEBUG    
    if (::TryEnterCriticalSection(&m_crit))
    {
        ::LeaveCriticalSection(&m_crit);
        ::DeleteCriticalSection(&m_crit);
    }
    else
        DebugBreak();
#else
    ::DeleteCriticalSection(&m_crit);
#endif
}

void CCriticalSection::Enter()
{
    ::EnterCriticalSection(&m_crit);
}

void CCriticalSection::Leave()
{
    ::LeaveCriticalSection(&m_crit);
}

///////////////////////////////////////////////////////////////////////////////
// NETWORK::CCriticalSection::AutoLock
///////////////////////////////////////////////////////////////////////////////

CCriticalSection::AutoLock::AutoLock(CCriticalSection &crit)
    : m_crit(crit)
{
    m_crit.Enter();
}

CCriticalSection::AutoLock::~AutoLock()
{
    m_crit.Leave();
}

///////////////////////////////////////////////////////////////////////////////
// CSemaphore
///////////////////////////////////////////////////////////////////////////////

bool CSemaphore::Wait(DWORD timeoutMillis) 
{
    bool ok;

    DWORD result = ::WaitForSingleObject(m_hSemaphore, timeoutMillis);

    if (result == WAIT_TIMEOUT)
        ok = false;
    else if (result == WAIT_OBJECT_0)
        ok = true;
    else
        throw CWin32Exception(_T("CSemaphore::Wait() - WaitForSingleObject"), ::GetLastError());

    return ok;
}

CSemaphore::CSemaphore(long lInitialCount, long lMaxCount)
        : m_hSemaphore(::CreateSemaphore(NULL, lInitialCount, lMaxCount, NULL))
{
    if (m_hSemaphore == NULL)
        throw CWin32Exception(_T("CSemaphore::CSemaphore()"), ::GetLastError());
}

void CSemaphore::Add(long lCount, long* lpPrevCount)
{
    if ( !::ReleaseSemaphore(m_hSemaphore, lCount, lpPrevCount) )
        throw CWin32Exception(_T("CSemaphore::Add()"), ::GetLastError());
}

void CSemaphore::Wait()
{
    if (!Wait(INFINITE))
        throw CGeneralException(_T("CSemaphore::Wait()"), _T("Unexpected timeout on infinite wait"));
}
///////////////////////////////////////////////////////////////////////////////
// CEvent
///////////////////////////////////////////////////////////////////////////////

CEvent::CEvent(
               LPSECURITY_ATTRIBUTES lpEventAttributes,
               bool bManualReset,
               bool bInitialState)
    :  m_hEvent(Create(lpEventAttributes, bManualReset, bInitialState, 0))
{

}

CEvent::CEvent(
               LPSECURITY_ATTRIBUTES lpEventAttributes,
               bool bManualReset,
               bool bInitialState,
               const _tstring &name)
    :  m_hEvent(Create(lpEventAttributes, bManualReset, bInitialState, name.c_str()))
{

}

CEvent::~CEvent()
{
    ::CloseHandle(m_hEvent);
}

HANDLE CEvent::GetEvent() const
{
    return m_hEvent;
}

void CEvent::Wait() const
{
    if (!Wait(INFINITE))
    {
        throw CGeneralException(_T("CEvent::Wait()"), _T("Unexpected timeout on infinite wait"));
    }
}

bool CEvent::Wait(DWORD timeoutMillis) const
{
    bool ok;

    DWORD result = ::WaitForSingleObject(m_hEvent, timeoutMillis);

    if (result == WAIT_TIMEOUT)
    {
        ok = false;
    }
    else if (result == WAIT_OBJECT_0)
    {
        ok = true;
    }
    else
    {
        throw CWin32Exception(_T("CEvent::Wait() - WaitForSingleObject"), ::GetLastError());
    }

    return ok;
}

void CEvent::Reset()
{
    if (!::ResetEvent(m_hEvent))
    {
        throw CWin32Exception(_T("CEvent::Reset()"), ::GetLastError());
    }
}

void CEvent::Set()
{
    if (!::SetEvent(m_hEvent))
    {
        throw CWin32Exception(_T("CEvent::Set()"), ::GetLastError());
    }
}

void CEvent::Pulse()
{
    if (!::PulseEvent(m_hEvent))
    {
        throw CWin32Exception(_T("CEvent::Pulse()"), ::GetLastError());
    }
}

HANDLE CEvent::Create(LPSECURITY_ATTRIBUTES lpEventAttributes,
                      bool bManualReset,
                      bool bInitialState,
                      LPCTSTR lpName)
{
    HANDLE hEvent = ::CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);

    if (hEvent == NULL)
    {
        throw CWin32Exception(_T("CEvent::Create()"), ::GetLastError());
    }

    return hEvent;
}

///////////////////////////////////////////////////////////////////////////////
// CAutoRstEvent
///////////////////////////////////////////////////////////////////////////////

CAutoRstEvent::CAutoRstEvent(bool initialState /* = false */)
    :  CEvent(0, false, initialState)
{

}

CAutoRstEvent::CAutoRstEvent(const _tstring &name,
                             bool initialState /* = false */)
    :  CEvent(0, false, initialState, name)
{

}

///////////////////////////////////////////////////////////////////////////////
// CManualRstEvent
///////////////////////////////////////////////////////////////////////////////

CManualRstEvent::CManualRstEvent(bool initialState /* = false */)
    :  CEvent(0, true, initialState)
{

}

CManualRstEvent::CManualRstEvent(const _tstring &name,
                                 bool initialState /* = false */)
    :  CEvent(0, true, initialState, name)
{

}

#ifdef READWRITESECTION_TLS
///////////////////////////////////////////////////////////////////////////////
// CReadWriteSection
///////////////////////////////////////////////////////////////////////////////
DWORD CReadWriteSection::s_dwTlsIndex = TlsAlloc();

//------------------------------------------------------------------------------
// IsAlreadyReading:
//      Tell if current thread has already taken the Readlock of this 
//      CReadWriteSection
//------------------------------------------------------------------------------
bool CReadWriteSection::IsAlreadyReading()
{
    _ASSERTE( s_dwTlsIndex != TLS_OUT_OF_INDEXES );

    RwsRecords * pVecRwsRecords = reinterpret_cast<RwsRecords *>(TlsGetValue( s_dwTlsIndex ));
    _ASSERTE( NULL != pVecRwsRecords );

    for ( RwsRecordIter iter = pVecRwsRecords->begin();
          iter != pVecRwsRecords->end();
          ++ iter )
    {
        // tell if CT(current thread) is already reading in this RWS(ReadWriteSection),
        // if so, just increment the count
        RWS_ENTERRECORD & rRwsRecord = *iter;
        if ( rRwsRecord.pRws == this )
        {
            rRwsRecord.dwEnterCount++;
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
// RecordReading:
//      Record the ReadWriteSections' Readlock current thread has held
//------------------------------------------------------------------------------
void CReadWriteSection::RecordReading()
{
    _ASSERTE( s_dwTlsIndex != TLS_OUT_OF_INDEXES );

    RwsRecords * pVecRwsRecords = reinterpret_cast<RwsRecords *>(TlsGetValue( s_dwTlsIndex ));
    if ( NULL == pVecRwsRecords )
    {
        pVecRwsRecords = new RwsRecords();
        TlsSetValue( s_dwTlsIndex, pVecRwsRecords );
    }

    RWS_ENTERRECORD re = { this, 1 };
    pVecRwsRecords->push_back( re );
}

//------------------------------------------------------------------------------
// RemoveReading:
//      Remove the Readlock record for current thread
//------------------------------------------------------------------------------
void CReadWriteSection::RemoveReading()
{
    _ASSERTE( s_dwTlsIndex != TLS_OUT_OF_INDEXES );

    RwsRecords * pVecRwsRecords = reinterpret_cast<RwsRecords *>(TlsGetValue( s_dwTlsIndex ));
    _ASSERTE( NULL != pVecRwsRecords );

    for ( RwsRecordIter iter = pVecRwsRecords->begin();
          iter != pVecRwsRecords->end();
          ++ iter )
    {
        RWS_ENTERRECORD & rRwsRecord = *iter;
        if ( rRwsRecord.pRws == this )
        {
            // check the entercount
            if ( --rRwsRecord.dwEnterCount == 0 )
            {
                pVecRwsRecords->erase( iter );
            }
            break;
        }
    }

    //
    // Performance sacrifice, just to prevent memory leak
    if ( 0 == pVecRwsRecords->size() )
    {
        delete pVecRwsRecords;
        TlsSetValue( s_dwTlsIndex, NULL );
    }
}
#endif // READWRITESECTION_TLS

}