///////////////////////////////////////////////////////////////////////////////
//
//  File        : Thread.h
//  Version     : 1
//  Description : Definition of CThread, NETWORK::CCriticalSection, CEvent, CSemaphore, etc.
//
//  Author      : Jian-Guang LOU
//  Date:       : 2005-12-06
//
//  Copyright (C) 
//  All rights reserved.              
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#pragma warning(push)
#pragma warning(disable:4512)
#include "TString.h"

typedef unsigned (__stdcall *PTHREAD_START) (void*); 

#define svsBeginThread(psa, cbStack, pfnStartAddr, \
    pvParam, fdwCreate, pdwThreadID)               \
    ((HANDLE) _beginthreadex(                      \
      (void*) (psa),                               \
      (unsigned) cbStack,                          \
      (PTHREAD_START)(pfnStartAddr),               \
      (void *) (pvParam),                          \
      (unsigned)(fdwCreate),                       \
      (unsigned *)(pdwThreadID)))

namespace OS {

class CThread;
class CCriticalSection;
class CEvent;
class CAutoRstEvent;
class CManualRstEvent;


///////////////////////////////////////////////////////////////////////////////
// CThread
///////////////////////////////////////////////////////////////////////////////

class CThread
{
 public :

    CThread();

    virtual ~CThread();

    HANDLE GetHandle() const
    { return m_hThread; }

    void Wait() const;

    bool Wait(DWORD timeoutMillis) const;

	virtual void Start();

    void Terminate(DWORD exitCode = 0);

    void PostMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void Resume()
    {
        if (m_running)
            return;
        if (m_hThread != INVALID_HANDLE_VALUE)
            ResumeThread(m_hThread);
        m_running = true;
    };
    void Suspend()
    {
        if (!m_running)
            return;
        if (m_hThread != INVALID_HANDLE_VALUE)
            SuspendThread(m_hThread);
        m_running = false;
    };
 protected :

    virtual int Run() = 0;

    static DWORD __stdcall ThreadFunction(void *pV);
    bool  m_running;
    HANDLE m_hThread;
    DWORD  m_dwThreadID;
};

///////////////////////////////////////////////////////////////////////////////
// NETWORK::CCriticalSection
///////////////////////////////////////////////////////////////////////////////

class CCriticalSection
{
public :

    class AutoLock
    {
     public:
		 explicit AutoLock(OS::CCriticalSection &crit);
        ~AutoLock();

     private :
         OS::CCriticalSection &m_crit;
    };

    CCriticalSection( DWORD dwSpinCount = 0 );

    ~CCriticalSection();

    void Enter();

    void Leave();

private :

    CRITICAL_SECTION m_crit;
};

///////////////////////////////////////////////////////////////////////////////
// CEvent
///////////////////////////////////////////////////////////////////////////////

class CEvent
{
public :

    CEvent( LPSECURITY_ATTRIBUTES lpSecurityAttributes,
           bool manualReset,
           bool initialState);

    CEvent( LPSECURITY_ATTRIBUTES lpSecurityAttributes,
           bool manualReset,
           bool initialState,
           const _tstring &name);

    virtual ~CEvent();

    HANDLE GetEvent() const;

    void Wait() const;

    bool Wait(DWORD timeoutMillis) const;

    void Reset();

    void Set();

    void Pulse();

    static HANDLE Create(LPSECURITY_ATTRIBUTES lpEventAttributes,
                         bool bManualReset,
                         bool bInitialState,
                         LPCTSTR lpName);

private :

    HANDLE m_hEvent;
};

///////////////////////////////////////////////////////////////////////////////
// CAutoRstEvent
///////////////////////////////////////////////////////////////////////////////

class CAutoRstEvent : public CEvent
{
 public :

    explicit CAutoRstEvent(
                           bool initialState = false);

    explicit CAutoRstEvent(
                           const _tstring &name,
                           bool initialState = false);
};


///////////////////////////////////////////////////////////////////////////////
// CManualRstEvent
///////////////////////////////////////////////////////////////////////////////

class CManualRstEvent : public CEvent
{
 public :

    explicit CManualRstEvent(
                             bool initialState = false);

    explicit CManualRstEvent(
                             const _tstring &name,
                             bool initialState = false);

};


/////////////////////////////////////////////////////////////
////CReadWriteSection is designed for operation of read and write operators
////Multiple threads can read an object simultanuously
////while, only one write operation is permitted at a time
///////////////////////////////////////////////////////////////
class CReadWriteSection
{
    CCriticalSection    m_cs;
    CCriticalSection    m_rcs;
    LONG                m_read_count;
    CEvent              m_event_write;
    WORD                m_valid;

#ifdef READWRITESECTION_TLS
    static DWORD        s_dwTlsIndex;
#endif

public:
	bool IsValid()
	{
		return (m_valid == 0xA55A);
	};
    CReadWriteSection()
        : m_event_write(NULL,true,true)
        , m_cs( 4000 )
        , m_rcs( 4000 )
    {
        m_read_count = 0;
        m_valid = 0xA55A;
    };

    virtual ~CReadWriteSection()
    {
        m_valid = 0x5AA5;
    };

#ifdef READWRITESECTION_TLS
    void EnterRead()
    {
        // Jie JIAN change: 2007-12-3
        // check TLS for already held RWS
        if ( !IsAlreadyReading() )
        {
            CCriticalSection::AutoLock mylock(m_cs);
            //m_event_read.Wait();

            //Protected count
            CCriticalSection::AutoLock rlock(m_rcs);
            m_event_write.Reset();
            ::InterlockedIncrement(&m_read_count);

            RecordReading();
        }
    };

    void LeaveRead()
    {
        RemoveReading();

        CCriticalSection::AutoLock rlock(m_rcs);
        if (::InterlockedDecrement(&m_read_count) == 0)
            m_event_write.Set();
    };
#else 
    void EnterRead()
    {
        CCriticalSection::AutoLock mylock(m_cs);
        //m_event_read.Wait();

        //Protected count
        CCriticalSection::AutoLock rlock(m_rcs);
        m_event_write.Reset();
        ::InterlockedIncrement(&m_read_count);
    };

    void LeaveRead()
    {
        CCriticalSection::AutoLock rlock(m_rcs);
        if (::InterlockedDecrement(&m_read_count) == 0)
            m_event_write.Set();
    };

#endif // READWRITESECTION_TLS

    void EnterWrite()
    {
        m_cs.Enter();
        m_event_write.Wait();
        //m_event_read.Reset();
    };

    void LeaveWrite()
    {
        //m_event_read.Set();
        m_cs.Leave();
    };

#ifdef READWRITESECTION_TLS
protected:
    /********************************************************************
     * Jie JIAN @ 2007-12-3:                                            *
     *      Functions used to tackle reader lock re-entrance            *
     ********************************************************************/
    struct RWS_ENTERRECORD
    {
    public:
        void        * pRws;
        DWORD         dwEnterCount;
    };

    bool IsAlreadyReading();

    void RecordReading();

    void RemoveReading();

    typedef std::vector<RWS_ENTERRECORD> RwsRecords;
    typedef RwsRecords::iterator         RwsRecordIter;
#endif


public:
    class AutoWriteLock
    {
     public:
         explicit AutoWriteLock(CReadWriteSection &crit): m_crit(crit)
        {
            if (m_crit.m_valid == 0xA55A)
            {
                m_crit.EnterWrite();
            }
        };
        ~AutoWriteLock()
        {
            if (m_crit.m_valid == 0xA55A)
                m_crit.LeaveWrite();
        };

     private :
        CReadWriteSection &m_crit;
    };

    class AutoReadLock
    {
     public:
         explicit AutoReadLock(CReadWriteSection &crit): m_crit(crit)
        {
            if (m_crit.m_valid == 0xA55A)
            {
                m_crit.EnterRead();
            }
        };
        ~AutoReadLock()
        {
            if (m_crit.m_valid == 0xA55A)
                m_crit.LeaveRead();
        };

     private :
        CReadWriteSection &m_crit;
    };
};

/////////////////////////////////////////////////////////////////////////////
// CSemaphore
///////////////////////////////////////////////////////////////////////////////

class CSemaphore
{
public:
	CSemaphore(long lInitialCount = 1, long lMaxCount = 1);

    ~CSemaphore() 
    {
        ::CloseHandle(m_hSemaphore);
    }

    void Add() { return Add(1,NULL); } 

    void Add(long lCount, long* lpPrevCount = NULL);

	void Wait();    

    bool Wait(DWORD timeoutMillis);

    HANDLE GetHandle(){return m_hSemaphore;};

private:
    HANDLE m_hSemaphore;
};

}

#pragma warning(pop)
