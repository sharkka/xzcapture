///////////////////////////////////////////////////////////////////////////////
//
//  File        : CIOBuffer.h
//  Version     : 1
//  Description : Definition of CIOBuffer and CIOBuffer::Allocator
//
//  Author      : Jian-Guang
//  Date:       : 2005-12-06
//              
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __CIOBUFFER__
#define __CIOBUFFER__

#pragma warning(disable : 4786)  // identifier was truncated to '255' characters 
                                 // in the debug information

#pragma warning(disable: 4200)   // nonstandard extension used : zero-sized array in struct/union

#include <winsock2.h>
#include <list>
#include <hash_set>

#include "CRefCountObject.h"
#include "IIOBuffer.h"
#include "Thread.h" 
#include "RawUserData.h"

namespace OS
{


	class CIOBuffer;
	class COverlapped;

	///////////////////////////////////////////////////////////////////////////////
	// CIOBuffer
	///////////////////////////////////////////////////////////////////////////////

#pragma warning( disable: 4512 ) // assignment operatore not gen

	class CIOBuffer :
		public IIOBuffer
	{
	public:
		class Allocator;
		friend class Allocator;
		friend class COverlapped;

		// interface IRefCount
		long Release();

		// interface IIOBuffer
		size_t GetUsedSize() const     { return m_used; }
		size_t GetTotalSize() const    { return m_size; }
		const BYTE* GetBuffer() const  { return &m_buffer[0]; }
		const BYTE* GetDataPtr() const { return &m_buffer[0] + m_dataPos; }
		size_t GetDataSize() const     { return (m_growForward ? m_used : m_size) - m_dataPos; }
		void AddData(const char* const pData, size_t dataLength);
		void AddData(const BYTE* const pData, size_t dataLength)
		{
			AddData(reinterpret_cast<const char*>(pData), dataLength);
		}
		void AddData(char data)        { AddData(&data, 1); }
		void AddData(BYTE data)        { AddData(&data, 1); }
		void Use(size_t bytes);
		BYTE* Reserve(size_t bytes);
		void ConsumeData(size_t bytes);
		void ClearData();
		void Defrag();

		bool GrowForward() const { return m_growForward; }

		COverlapped* AllocateOverlapped();  // the return object has reference 1.

		CIOBuffer* SplitBuffer(size_t bytesToSplit);
		CIOBuffer* AllocateNewBuffer(bool growForward = true, bool addRef = false, bool cloneData = false) const;

	private:
		Allocator &m_allocator;

		bool m_growForward;
		size_t m_dataPos;

		const size_t m_size;
		size_t m_used;
		BYTE m_buffer[0];    // start of the actual buffer, 
		// must remain the last data member in the class.

	private:
		CIOBuffer(Allocator &allocator, size_t size, bool growForward = true, long initRef = 1);

		static void *operator new(size_t objSize, size_t bufferSize);
		static void operator delete(void *pObj, size_t bufferSize);
	};

	///////////////////////////////////////////////////////////////////////////////
	// COverlapped
	///////////////////////////////////////////////////////////////////////////////

	class COverlapped :
		public OVERLAPPED,
		public CRawUserData
	{
	public:
		friend class CIOBuffer;
		friend class CIOBuffer::Allocator;

		long AddRef();
		long Release();

		// interface for IO
		void SetupZeroByteRead();
		void SetupRead();
		void SetupWrite();
		WSABUF *GetWSABUF() const       { return const_cast<WSABUF*>(&m_wsabuf); }
		size_t GetIndication() const          { return m_indication; }
		void SetIndication(size_t indication) { m_indication = indication; }

		SOCKADDR_IN* GetIoParamAddr()   { return &m_addr; }
		int* GetIoParamAddrLen()        { return &m_addrLen; }
		DWORD* GetIoParamNumBytes()     { return &m_dwNumBytes; }
		DWORD* GetIoParamFlags()        { return &m_dwFlags; }

		IIOBuffer* GetIOBuffer() { return m_pBuf; }

	private:
		COverlapped(CIOBuffer::Allocator& allocator);
		void AttachBuf(IIOBuffer* pBuf)
		{
			if (pBuf)
				pBuf->AddRef();
			m_pBuf = pBuf;
		}
		void DetachBuf()
		{
			if (m_pBuf)
			{
				m_pBuf->Release();
				m_pBuf = NULL;
			}
		}

	private:
		CIOBuffer::Allocator& m_allocator;
		IIOBuffer* m_pBuf;

		WSABUF m_wsabuf;

		// accompanying parameters for overlapped IO
		SOCKADDR_IN m_addr;
		int m_addrLen;
		DWORD m_dwNumBytes;
		DWORD m_dwFlags;

		size_t m_indication;

		OS::CCriticalSection m_crit;
		long m_ref;
	};

#pragma warning( default: 4512 )

	///////////////////////////////////////////////////////////////////////////////
	// CIOBuffer::Allocator
	///////////////////////////////////////////////////////////////////////////////

	class CIOBuffer::Allocator
	{
	public:

		friend class CIOBuffer;
		friend class COverlapped;

		explicit Allocator(
			size_t bufferSize,
			size_t maxFreeBuffers);

		virtual ~Allocator();

		CIOBuffer* Allocate(bool growForward = true, bool AddRef = false);

		size_t GetBufferSize() const{ return m_bufferSize; }

	protected:

		void Initialize(size_t bufferSize, size_t maxFreeBuffers);

		void DestroyAll();
		void DestroyAllBuffers();
		void DestroyAllOverlaps();

		COverlapped* AllocateOverlapped(IIOBuffer* pBuf);

	private:
		COverlapped* AllocateOverlapped();
		void Release(COverlapped *pOv);
		void Destroy(COverlapped *pOv);

		void Release(CIOBuffer *pBuffer);
		void DestroyBuffer(CIOBuffer *pBuffer);

		virtual void OnBufferCreated() {}
		virtual void OnBufferAllocated() {}
		virtual void OnBufferReleased() {}
		virtual void OnBufferDestroyed() {}

	private:
		size_t m_bufferSize;
		size_t m_maxFreeBuffers;

		// Jie JIAN changed on 2008-01-02
		// 1. use vector in place of list to improve performance
		// 2. separate the CriticalSection to reduce contention

		OS::CCriticalSection m_csBufferFree;
		OS::CCriticalSection m_csBufferActive;

		// typedef std::list<CIOBuffer*> BufferList;
		typedef std::vector<CIOBuffer*>       BufferList;
		typedef stdext::hash_set<CIOBuffer*>  BufferSet;
		BufferList m_freeList;
		BufferSet  m_activeList;

		OS::CCriticalSection m_csOvFree;
		OS::CCriticalSection m_csOvActive;

		// typedef std::list<COverlapped*> OverlapList;
		typedef std::vector<COverlapped*>       OverlapList;
		typedef stdext::hash_set<COverlapped*>  OverlapSet;
		OverlapList m_freeOvList;
		OverlapSet  m_activeOvList;
	};

}
#endif // __CIOBUFFER__
