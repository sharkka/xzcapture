#include "stdafx.h"
#include "HeapAllocator.h"

namespace OS {

void * CHeapAllocator::alloc(size_t nbytes)
{
    if ((m_hHeap != NULL)&&(nbytes))
    {
        void * pVoid = HeapAlloc(m_hHeap,0,nbytes);
        if (pVoid && m_bLogSize)
        {
#pragma warning(suppress: 4267)
            __declspec(align(4)) LONG mysize = nbytes;
            ::InterlockedExchangeAdd(&m_lAllocatedSize,mysize);
        }
        return pVoid;
    }
    return NULL;
}

void   CHeapAllocator::free(void * addr, size_t nbytes)
{
    if ((m_hHeap != NULL)&&(addr))
    {
        if (HeapFree(m_hHeap,0,addr) && m_bLogSize)
        {
#pragma warning(suppress: 4267)
            __declspec(align(4)) LONG mysize = 0L - nbytes;
            ::InterlockedExchangeAdd(&m_lAllocatedSize,mysize);
        }
    }
}
    
BOOL CHeapAllocator::Initialize(SIZE_T dwInitSize)
{
    Unitialize();
    m_hHeap = HeapCreate(0,dwInitSize,0);


	// Jie JIAN changed @ 2007-12-28
	// use LFH if possible
	typedef BOOL (WINAPI *PFN_HEAPSETINFORMATION)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
	
	HMODULE hKernel32 = LoadLibraryW( L"kernel32.dll" );
	if ( NULL != hKernel32 )
	{
		PFN_HEAPSETINFORMATION pfnHeapSetInformation = 
			reinterpret_cast<PFN_HEAPSETINFORMATION>(
			GetProcAddress(hKernel32, "HeapSetInformation")
			);

		if ( NULL != pfnHeapSetInformation )
		{
			ULONG ulHeapInformation = 2;
			(*pfnHeapSetInformation)( m_hHeap,
									  HeapCompatibilityInformation,
									  &ulHeapInformation,
									  sizeof(ULONG) );
		}

		FreeLibrary(hKernel32);
	}

    return TRUE;
}

BOOL CHeapAllocator::Unitialize()
{
    if (m_hHeap != NULL)
        HeapDestroy(m_hHeap);
    m_hHeap = NULL;
    return TRUE;
}

}
