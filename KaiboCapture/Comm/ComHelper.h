////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  Definition of some COM helper macros and functions
//
////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

//----------------------------------------------
// Utilities
//----------------------------------------------

/* You must override the (pure virtual) QueryInterface to return
   interface pointers (using GetInterface) to the interfaces your derived
   class supports (the default implementation only supports IUnknown) */

#define DECLARE_SVS_IUNKNOWN(MyClassName)                       \
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);       \
                                                                \
    STDMETHODIMP_(ULONG) AddRef() {                             \
        LONG lRef = InterlockedIncrement( &m_cRef );            \
        return ULONG(m_cRef);                                   \
    };                                                          \
    STDMETHODIMP_(ULONG) Release() {                            \
        LONG lRef = InterlockedDecrement( &m_cRef );            \
    if (lRef == 0) {                                            \
        m_cRef++;                                               \
        delete this;                                            \
        return ULONG(0);                                        \
     }                                                          \
     return ULONG(m_cRef);                                      \
    };                                                          \
    static STDMETHODIMP CreateInstance(REFIID riid, void **ppv){       \
        IUnknown * tmpclass = reinterpret_cast<IUnknown *>(new MyClassName);                  \
        if (tmpclass == NULL)                                   \
            return E_OUTOFMEMORY;                               \
        HRESULT hr = tmpclass->QueryInterface(riid, ppv);       \
        if (FAILED(hr)){                                        \
            delete tmpclass;                                    \
        }                                                       \
        return hr;                                              \
    };                                                          \
protected:                                                      \
    volatile LONG m_cRef;                                       


#ifndef INONDELEGATINGUNKNOWN_DEFINED
class INonDelegatingUnknown
{
    virtual HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface
                ( 
                /* [in] */  REFIID riid, 
                /* [out] */ void **ppvObject ) = 0;

    virtual ULONG STDMETHODCALLTYPE NonDelegatingAddRef() = 0;

    virtual ULONG STDMETHODCALLTYPE NonDelegatingRelease() = 0;    
};
#define INONDELEGATINGUNKNOWN_DEFINED
#endif

#ifndef SAFE_RELEASE
    #define SAFE_RELEASE( x )           \
        if ( NULL != x )                \
        {                               \
            x->Release( );              \
            x = NULL;                   \
        }
#endif // SAFE_RELEASE

#ifndef SAFE_CLOSEHANDLE
#define SAFE_CLOSEHANDLE(h) {	if (h != NULL)		{	CloseHandle(h); h = NULL;	}		}
#endif 

#ifndef SAFE_DELETE
#define SAFE_DELETE( x )            \
if (x)                          \
{                                \
	delete x;                    \
	x = NULL;                    \
}
#endif 

#ifndef SAFE_ARRAYDELETE
#define SAFE_ARRAYDELETE( x )       \
if (x)                          \
{                                \
	delete[] x;                 \
	x = NULL;                    \
}
#endif

#ifndef SAFE_CLOSEHANDLE
#define SAFE_CLOSEHANDLE(h) {	if (h != NULL)		{	CloseHandle(h); h = NULL;	}		}
#endif 

#ifndef GOTO_EXIT_IF_FAILED
    #define GOTO_EXIT_IF_FAILED(hr) { if FAILED(hr) goto Exit; }
#endif  //GOTO_EXIT_IF_FAILED

#ifndef RETURN_IF_FAILED
    #define RETURN_IF_FAILED(hr)    { if (FAILED(hr)) return hr;    }
#endif  //RETURN HRESULT IF FAILED

#ifndef BREAK_IF_FAILED
    #define BREAK_IF_FAILED(hr)    { if (FAILED(hr)) break;    }
#endif  //RETURN HRESULT IF FAILED

#ifndef CONTINUE_IF_FAILED
    #define CONTINUE_IF_FAILED(hr)    { if (FAILED(hr)) continue;    }
#endif  //RETURN HRESULT IF FAILED

