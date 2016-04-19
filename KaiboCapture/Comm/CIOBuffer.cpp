///////////////////////////////////////////////////////////////////////////////
//
//  File        : CIOBuffer.cpp
//  Version     : 6
//  Description : Implementation of CIOBuffer and CIOBuffer::Allocator
//
//  Author      : Jian-Guang LOU
//  Date:       : 2005-12-06
//      
//
//  Revision history: 
//
//    2. Jian Tang, 2006-02-21
//       Reimplement active list with hash_set for CIOBuffer::Allocator.
//
//    1. Jian Tang, 2006-01-20
//       Decoupling OVERLAPPED structure from class CIOBuffer.
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <stdio.h>
#include <algorithm>

using std::find;

#include "CIOBuffer.h"
#include "Exception.h"
#include "Win32_Utils.h"
#include "HeapAllocator.h"

namespace OS
{

	///////////////////////////////////////////////////////////////////////////////
	// CIOBuffer
	///////////////////////////////////////////////////////////////////////////////

	CIOBuffer::CIOBuffer(Allocator &allocator, size_t size, bool growForward, long initRef)
		: IIOBuffer(initRef),
		m_allocator(allocator),
		m_size(size),
		m_used(0),
		m_growForward(growForward)
	{
		ClearData();
	}

	long CIOBuffer::Release()
	{
		long result = CRefCountObject::Release();

		if (0 == result)
		{
			m_used = 0;
			m_growForward = true;

			m_allocator.Release(this);
		}

		return result;
	}

	void CIOBuffer::AddData(const char* const pData, size_t dataLength)
	{
		if (dataLength > m_size - m_used)
		{
			throw CGeneralException(_T("CIOBuffer::AddData()"), _T("Not enough space in buffer"));
		}

		if (m_growForward)
		{
			memcpy(m_buffer + m_used, pData, dataLength);
			m_used += dataLength;
		}
		else
		{
			// data growing backward from current GetDataPtr()
			memcpy(const_cast<BYTE*>(GetDataPtr() - dataLength), pData, dataLength);
			m_dataPos -= dataLength;
			if ((m_size - m_used) > m_dataPos)
				m_used = m_size - m_dataPos;
		}
	}

	BYTE* CIOBuffer::Reserve(size_t dataLength)
	{
		if (dataLength > m_size - m_used)
		{
			throw CGeneralException(_T("CIOBuffer::Reserve()"), _T("Not enough space in buffer"));
		}

		if (m_growForward)
		{
			BYTE* pReserved = const_cast<BYTE*>(GetBuffer()) + m_used;
			m_used += dataLength;

			return pReserved;
		}
		else
		{
			// data growing backward from current GetDataPtr()        
			m_dataPos -= dataLength;
			if ((m_size - m_used) > m_dataPos)
				m_used = m_size - m_dataPos;

			return const_cast<BYTE*>(GetDataPtr());
		}
	}

	void CIOBuffer::Use(size_t dataLength)
	{
		if (m_growForward)
			m_used += dataLength;
		else
		{
			// data growing backward from current GetDataPtr()        
			m_dataPos -= dataLength;
			if ((m_size - m_used) > m_dataPos)
				m_used = m_size - m_dataPos;
		}
	}

	void CIOBuffer::ConsumeData(size_t bytes)
	{
		size_t newPos = m_dataPos + bytes;

		if ((m_growForward && (newPos > m_used)) || (!m_growForward && (newPos > m_size)))
			throw CGeneralException(_T("CIOBuffer::ConsumeData()"), _T("Not enough data to consume"));

		m_dataPos = newPos;
	}

	void CIOBuffer::ClearData()
	{
		//m_wsabuf.buf = reinterpret_cast<char*>(m_buffer);
		//m_wsabuf.len = m_size;

		m_used = 0;

		if (m_growForward)
			m_dataPos = 0;
		else
			m_dataPos = m_size;
	}

	CIOBuffer* CIOBuffer::SplitBuffer(size_t bytesToSplit)
	{
		if (bytesToSplit + m_dataPos > (m_growForward ? m_used : m_size))
		{
			throw CGeneralException(_T("CIOBuffer::SplitBuffer()"), _T("Not enough data to split"));
		}

		CIOBuffer *pNewBuffer = m_allocator.Allocate(m_growForward, true);

		pNewBuffer->AddData(GetDataPtr(), bytesToSplit);

		ConsumeData(bytesToSplit);

		Defrag();

		return pNewBuffer;
	}

	CIOBuffer* CIOBuffer::AllocateNewBuffer(bool growForward, bool addRef, bool cloneData) const
	{
		CIOBuffer* pNew = m_allocator.Allocate(growForward, addRef);

		if (cloneData && pNew)
			pNew->AddData(this->GetDataPtr(), this->GetDataSize());

		return pNew;
	}

	COverlapped* CIOBuffer::AllocateOverlapped()
	{
		return m_allocator.AllocateOverlapped(this);;
	}

	void CIOBuffer::Defrag()
	{
		if (m_growForward && m_dataPos != 0)
		{
			memmove(m_buffer, GetDataPtr(), m_used - m_dataPos);
			m_used -= m_dataPos;
			m_dataPos = 0;
		}
		else if (!m_growForward && m_dataPos != m_size - m_used)
		{
			m_used = m_size - m_dataPos;
		}
	}

	/*
	void *CIOBuffer::operator new(size_t objectSize, size_t bufferSize)
	{
	void *pMem = new char[objectSize + bufferSize];

	return pMem;
	}

	void CIOBuffer::operator delete(void *pObject, size_t  bufferSize)
	{
	delete[] (char *)pObject;
	}
	*/

	OS::CHeapAllocator g_IOBufAllocator(1024 * 1024, FALSE);

	void *CIOBuffer::operator new(size_t objectSize, size_t bufferSize)
	{
		void *pMem = g_IOBufAllocator.alloc(objectSize + bufferSize);

		return pMem;
	}

	void CIOBuffer::operator delete(void *pObject, size_t objectSize)
	{
		CIOBuffer* pThisBuf = reinterpret_cast<CIOBuffer *>(pObject);

		g_IOBufAllocator.free(pObject, objectSize + pThisBuf->m_size);
	}



	///////////////////////////////////////////////////////////////////////////////
	// COverlapped
	///////////////////////////////////////////////////////////////////////////////

	COverlapped::COverlapped(CIOBuffer::Allocator &allocator)
		: m_allocator(allocator),
		m_ref(1),
		m_pBuf(NULL),
		m_indication(0)
	{
		ZeroMemory(static_cast<OVERLAPPED *>(this), sizeof(OVERLAPPED));
	}

	long COverlapped::AddRef()
	{
		CCriticalSection::AutoLock lock(m_crit);
		return ++m_ref;
	}

	long COverlapped::Release()
	{
		long res;
		bool err = false;

		{
			CCriticalSection::AutoLock lock(m_crit);

			if (m_ref == 0)      // Error! double Release
			{
				err = true;
				throw CGeneralException(_T("COverlapped::Release()"), _T("m_ref is already 0"));
			}
			else
				--m_ref;

			res = m_ref;
		}

		if (res == 0 && !err)
		{
			m_indication = 0;
			DetachBuf();
			m_allocator.Release(this);
		}

		return res;
	}

	void COverlapped::SetupZeroByteRead()
	{
		if (!m_pBuf->GrowForward())
			throw CGeneralException(_T("COverlapped::SetupZeroByteRead()"),
			_T("Buffer growing backward while being used for read operation"));

		m_wsabuf.buf = reinterpret_cast<char*>(const_cast<BYTE*>(m_pBuf->GetBuffer())) + m_pBuf->GetUsedSize();
		// reinterpret_cast<char*>(m_pBuf->m_buffer) + m_pBuf->m_used;
		m_wsabuf.len = 0;
	}

	void COverlapped::SetupRead()
	{
		if (!m_pBuf->GrowForward())
			throw CGeneralException(_T("COverlapped::SetupRead()"),
			_T("Buffer growing backward while being used for read operation"));

		m_wsabuf.buf = reinterpret_cast<char*>(const_cast<BYTE*>(m_pBuf->GetBuffer())) + m_pBuf->GetUsedSize();
		// reinterpret_cast<char*>(m_pBuf->m_buffer) + m_pBuf->m_used;
		m_wsabuf.len = (ULONG)(m_pBuf->GetTotalSize() - m_pBuf->GetUsedSize());
		// m_pBuf->m_size - m_pBuf->m_used; 
	}

	void COverlapped::SetupWrite()
	{
		if (m_pBuf->GrowForward())
			throw CGeneralException(_T("COverlapped::SetupWrite()"),
			_T("Buffer growing forward while being used for write operation"));

		m_wsabuf.buf = reinterpret_cast<char*>(const_cast<BYTE*>(m_pBuf->GetDataPtr()));
		m_wsabuf.len = (ULONG)m_pBuf->GetDataSize();   // m_pBuf->m_size - m_pBuf->m_dataPos;    
	}

	///////////////////////////////////////////////////////////////////////////////
	// CIOBuffer::Allocator
	///////////////////////////////////////////////////////////////////////////////

	CIOBuffer::Allocator::Allocator(size_t bufferSize, size_t maxFreeBuffers)
		: m_bufferSize(bufferSize),
		m_maxFreeBuffers(maxFreeBuffers),
		m_csBufferFree(4000),
		m_csBufferActive(4000),
		m_csOvFree(4000),
		m_csOvActive(4000)
	{
		// reserve vector space for freelist
		m_freeList.reserve(m_maxFreeBuffers);
		m_freeOvList.reserve(m_maxFreeBuffers);
	}

	CIOBuffer::Allocator::~Allocator()
	{
		try
		{
			DestroyAll();
		}
		catch (...)
		{
		}
	}

	void CIOBuffer::Allocator::Initialize(size_t bufferSize, size_t maxFreeBuffers)
	{
		m_bufferSize = bufferSize;
		m_maxFreeBuffers = maxFreeBuffers;

		m_freeList.reserve(m_maxFreeBuffers);
		m_freeOvList.reserve(m_maxFreeBuffers);
	}

	CIOBuffer *CIOBuffer::Allocator::Allocate(bool growForward, bool addRef)
	{
		CIOBuffer *pBuffer = 0;

		{
			CCriticalSection::AutoLock lock(m_csBufferFree);

			if (!m_freeList.empty())
			{
				pBuffer = m_freeList.back();
				m_freeList.pop_back();
			}
		}

		if (NULL != pBuffer)
		{
			if (addRef)
				pBuffer->AddRef();

			pBuffer->m_growForward = growForward;
			pBuffer->ClearData();
		}
		else
		{
			pBuffer = new(m_bufferSize)CIOBuffer(*this, m_bufferSize, growForward, 0);

			if (!pBuffer)
				throw CGeneralException(_T("CIOBuffer::Allocator::Allocate()"), _T("Out of memory"));

			if (addRef)
				pBuffer->AddRef();

			OnBufferCreated();
		}

		{
			CCriticalSection::AutoLock lock(m_csBufferActive);

			m_activeList.insert(pBuffer);    //   m_activeList.push_back(pBuffer);
			OnBufferAllocated();
		}

		return pBuffer;
	}

	COverlapped* CIOBuffer::Allocator::AllocateOverlapped()
	{
		COverlapped *pOv = 0;

		{
			CCriticalSection::AutoLock lock(m_csOvFree);

			if (!m_freeOvList.empty())
			{
				pOv = m_freeOvList.back();
				m_freeOvList.pop_back();
			}
		}

		if (NULL != pOv)
		{
			pOv->AddRef();
		}
		else
		{
			pOv = new COverlapped(*this);

			if (!pOv)
				throw CGeneralException(_T("COverlapped::Allocator::AllocateOverlapped()"), _T("Out of memory"));
		}

		{
			CCriticalSection::AutoLock lock(m_csOvActive);

			m_activeOvList.insert(pOv);  // m_activeOvList.push_back(pOv);
		}

		return pOv;
	}

	COverlapped* CIOBuffer::Allocator::AllocateOverlapped(IIOBuffer* pBuf)
	{
		COverlapped* pOv = AllocateOverlapped();
		if (pOv)
			pOv->AttachBuf(pBuf);

		return pOv;
	}

	void CIOBuffer::Allocator::Release(CIOBuffer *pBuffer)
	{
		if (!pBuffer)
		{
			throw CGeneralException(_T("CIOBuffer::Allocator::Release()"), _T("pBuffer is null"));
		}

		{
			CCriticalSection::AutoLock lock(m_csBufferActive);

			//   BufferList::iterator iter = std::find(m_activeList.begin(), m_activeList.end(), pBuffer);
			BufferSet::iterator iter = m_activeList.find(pBuffer);
			if (iter == m_activeList.end())
			{
				throw CGeneralException(_T("CIOBuffer::Allocator::Release()"), _T("pBuffer is not in the list of m_activeList"));
			}
			OnBufferReleased();

			m_activeList.erase(iter);
		}


		{
			CCriticalSection::AutoLock lock(m_csBufferFree);

			if (m_maxFreeBuffers == 0 ||
				m_freeList.size() < m_maxFreeBuffers)
			{
				pBuffer->ClearData();

				// add to the free list
				m_freeList.push_back(pBuffer);
			}
			else
			{
				DestroyBuffer(pBuffer);
			}
		}

	}

	void CIOBuffer::Allocator::DestroyAll()
	{
		DestroyAllBuffers();
		DestroyAllOverlaps();
	}

	void CIOBuffer::Allocator::DestroyAllBuffers()
	{
		CCriticalSection::AutoLock freeLock(m_csBufferFree);
		CCriticalSection::AutoLock activeLock(m_csBufferActive);

		for each(CIOBuffer * pBuffer in m_freeList)
		{
			DestroyBuffer(pBuffer);
		}
		m_freeList.clear();

		for each(CIOBuffer * pBuffer in m_activeList)
		{
			DestroyBuffer(pBuffer);
		}
		m_activeList.clear();

		/*
		while (!m_activeList.empty())
		{
		OnBufferReleased();

		BufferSet::iterator iter = m_activeList.begin();
		CIOBuffer* pBuffer = *iter;
		DestroyBuffer(pBuffer);
		m_activeList.erase(iter);

		//CIOBuffer* pBuffer = m_activeList.front();
		//DestroyBuffer(pBuffer);
		//m_activeList.pop_front();
		}

		while (!m_freeList.empty())
		{
		CIOBuffer* pBuffer = m_freeList.back();
		DestroyBuffer(pBuffer);
		m_freeList.pop_back();
		}
		*/
	}

	void CIOBuffer::Allocator::DestroyAllOverlaps()
	{
		CCriticalSection::AutoLock freeLock(m_csOvFree);
		CCriticalSection::AutoLock activeLock(m_csOvActive);

		for each(COverlapped * pOv in m_activeOvList)
		{
			Destroy(pOv);
		}
		m_activeOvList.clear();

		for each(COverlapped * pOv in m_freeOvList)
		{
			Destroy(pOv);
		}
		m_freeOvList.clear();

		/*
		while (!m_activeOvList.empty())
		{
		OverlapSet::iterator it = m_activeOvList.begin();
		COverlapped* pOv = *it;
		Destroy(pOv);
		m_activeOvList.erase(it);

		//COverlapped* pOv = m_activeOvList.front();
		//Destroy(pOv);
		//m_activeOvList.pop_front();
		}

		while (!m_freeOvList.empty())
		{
		COverlapped* pOv = m_freeOvList.back();
		Destroy(pOv);
		m_freeOvList.pop_back();
		}
		*/
	}

	void CIOBuffer::Allocator::DestroyBuffer(CIOBuffer *pBuffer)
	{
		delete pBuffer;
		OnBufferDestroyed();
	}

	void CIOBuffer::Allocator::Release(COverlapped *pOv)
	{
		if (!pOv)
			return;

		{
			CCriticalSection::AutoLock lock(m_csOvActive);

			// OverlapList::iterator iter = std::find(m_activeOvList.begin(), m_activeOvList.end(), pOv);
			OverlapSet::iterator iter = m_activeOvList.find(pOv);

			if (iter == m_activeOvList.end())
			{
				//   throw CGeneralException(_T("COverlapped::Allocator::Release()"), _T("pOv is not in the list of m_activeOvList"));      
				return;
			}

			m_activeOvList.erase(iter);
		}

		{
			CCriticalSection::AutoLock lock(m_csOvFree);

			if (m_maxFreeBuffers == 0 ||
				m_freeOvList.size() < m_maxFreeBuffers)
			{
				// add to the free list
				m_freeOvList.push_back(pOv);
			}
			else
			{
				Destroy(pOv);
			}
		}
	}

	void CIOBuffer::Allocator::Destroy(COverlapped *pOv)
	{
		if (pOv->hEvent)
		{
			if (!::CloseHandle(pOv->hEvent))
			{
				throw CGeneralException(
					_T("CIOBuffer::Allocator::Destroy()"),
					GetLastErrorMessage(::GetLastError()));
			}
			pOv->hEvent = NULL;
		}
		delete pOv;
	}

//NAMESPACE
}