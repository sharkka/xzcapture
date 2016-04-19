#pragma once
#include <queue>
#include <map>
extern "C"{
#include "libavformat\avformat.h"
}
#include "Thread.h"

class CStreamSampleQueue
{
	typedef std::queue<AVFrame*> TSampleQueue;
	TSampleQueue m_queue;
	OS::CCriticalSection  m_CritSec;
	int m_nMaxLen;
public:
	CStreamSampleQueue(int nMaxLen = 90) 
	{
		m_nMaxLen = nMaxLen;
	};

	virtual ~CStreamSampleQueue()
	{
		AVFrame* theframe;
		while (!m_queue.empty())
		{
			//Release again to free the buffers
			theframe = m_queue.front();
			av_frame_unref(theframe);
			av_frame_free(&theframe);
			m_queue.pop();
		}
	};

	// add by Chundong WANG @ 2006-7-17
	void clear()
	{
		OS::CCriticalSection::AutoLock mylock(m_CritSec);
		AVFrame* theframe;
		while (!m_queue.empty())
		{
			//Release again to free the buffers
			theframe = m_queue.front();
			av_frame_unref(theframe);
			av_frame_free(&theframe);
			m_queue.pop();
		}
	}

	AVFrame* & back()
	{
		OS::CCriticalSection::AutoLock mylock(m_CritSec);
		return m_queue.back();
	};

	bool empty()
	{
		OS::CCriticalSection::AutoLock mylock(m_CritSec);
		return m_queue.empty();
	};

	AVFrame* & front()
	{
		OS::CCriticalSection::AutoLock mylock(m_CritSec);
		return m_queue.front();
	};

	void pop()
	{
		OS::CCriticalSection::AutoLock mylock(m_CritSec);
		AVFrame* pbuf = m_queue.front();
		av_frame_unref(pbuf);
		av_frame_free(&pbuf);
		return m_queue.pop();
	};

	AVFrame* pop_value()
	{
		OS::CCriticalSection::AutoLock mylock(m_CritSec);
		if (m_queue.empty())
			return NULL;
		AVFrame* pbuf = m_queue.front();

		m_queue.pop();
		return pbuf;
	};

	int push(AVFrame* & _val);

	int size()
	{
		OS::CCriticalSection::AutoLock mylock(m_CritSec);
		return (int)m_queue.size();
	};
};

class CStreamSamplePool
{
protected:
	typedef std::map<WORD, CStreamSampleQueue*> TStreamQueMap;
	typedef TStreamQueMap::iterator TStreamIterator;


	std::map<WORD, CStreamSampleQueue*> m_queuemap;
	OS::CCriticalSection m_lock;			// lock the GetStreamSample and InsertStreamSample
	//we initialize the pool first, then 
public:
	CStreamSamplePool()
	{
	};
	virtual ~CStreamSamplePool()
	{
		if (m_queuemap.empty())
			return;
		CStreamSampleQueue* pque;
		TStreamIterator myiter;
		for (myiter = m_queuemap.begin(); myiter != m_queuemap.end(); myiter++)
		{
			pque = myiter->second;
			delete pque;
		}
		m_queuemap.clear();
	};

	HRESULT ClearAllSamples()
	{
		OS::CCriticalSection::AutoLock lock(m_lock);

		TStreamIterator myiter = m_queuemap.begin();
		while (myiter != m_queuemap.end())
		{
			CStreamSampleQueue* pque;
			pque = myiter->second;
			pque->clear();
			myiter++;
		}

		return S_OK;
	}

	HRESULT GetStreamSample(WORD streamnum, AVFrame** sample)
	{
		OS::CCriticalSection::AutoLock lock(m_lock);

		if (sample == NULL)
			return E_POINTER;
		if (IsBadWritePtr(sample, sizeof(PVOID)))
			return E_INVALIDARG;

		TStreamIterator myiter = m_queuemap.find(streamnum);
		if (myiter == m_queuemap.end())
			return E_FAIL;

		CStreamSampleQueue* pque;
		pque = myiter->second;
		*sample = pque->pop_value();
		//if (*sample == NULL)
		//	DbgLog((LOG_TRACE, 1, TEXT("GetStreamSample...%d stream empty!"), streamnum));
		return S_OK;
	};

	HRESULT InsertStreamSample(WORD streamnum, AVFrame* sample)
	{
		OS::CCriticalSection::AutoLock lock(m_lock);

		if (sample == NULL)
			return E_POINTER;

		TStreamIterator myiter = m_queuemap.find(streamnum);
		if (myiter == m_queuemap.end())
			return E_FAIL;

		CStreamSampleQueue* pque;
		pque = myiter->second;
		pque->push(sample);
		//DbgLog((LOG_TRACE, 1, TEXT("InsertStreamSample...%d stream 0x%x"), streamnum, (int)(sample)));
		return S_OK;
	};

	BOOL IsAllBufEmpty()
	{
		OS::CCriticalSection::AutoLock lock(m_lock);
		BOOL res = TRUE;
		CStreamSampleQueue* pque;
		TStreamIterator myiter;
		for (myiter = m_queuemap.begin(); myiter != m_queuemap.end(); myiter++)
		{
			pque = myiter->second;
			if (pque == NULL)
				continue;
			res = res && pque->empty();
		}
		return res;
	};

	HRESULT AddQueue(WORD streamnum)
	{
		TStreamIterator myiter = m_queuemap.find(streamnum);
		if (myiter != m_queuemap.end())
		{
			return E_INVALIDARG;
		}
		else
		{
			CStreamSampleQueue* pque = new CStreamSampleQueue;
			if (pque == NULL)
				return E_OUTOFMEMORY;
			m_queuemap[streamnum] = pque;
			return S_OK;
		}
	};
};