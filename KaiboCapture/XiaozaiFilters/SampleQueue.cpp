#include "stdafx.h"
#include "SampleQueue.h"

int CStreamSampleQueue::push(AVFrame* & _val)
{
	OS::CCriticalSection::AutoLock mylock(m_CritSec);

#ifdef  MEDIABUF_OVERFLOW_CHECK_MEM_SIZE
		//Check if it exceeds the maximal memory size
		if (240 * 1024 * 1024 <= GetMediaBufAllocator()->getusedsize())
		{
			//Release the oldest buffers
			DbgLog((LOG_TRACE, 1, TEXT("CStreamSampleQueue: push: maximum queue length exceeded")));
			if (m_queue.size())
			{
				m_queue.front()->Release();
				pop();
			}
		}
#else
		//Check if it exceeds the maximal queue length
		if (m_nMaxLen <= (int)m_queue.size())
		{
			//Release the oldest buffers
			//DbgLog((LOG_TRACE, 1, TEXT("CStreamSampleQueue: push: maximum queue length exceeded")));			
			pop();

		}
#endif
//	DbgLog((LOG_MEMORY, 3, TEXT("buffer pool len %d "),	m_queue.size()));
	 m_queue.push(_val);
	 return 0;
}
