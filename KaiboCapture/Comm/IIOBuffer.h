///////////////////////////////////////////////////////////////////////////////
//
//  File        : IIOBuffer.h
//  Version     : 1
//  Description : Definition of IIOBuffer
//
//  Author      : Jian-Guang LOU
//  Date:       : 2005-12-06
//
//  Copyright (C)
//  All rights reserved.              
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __IIOBUFFER__
#define __IIOBUFFER__

#include "CRefCountObject.h"


//typedef unsigned char       BYTE;       // already defined in "windef.h"
namespace OS
{
	// interface for buffer operations
	class IIOBuffer : public OS::CRefCountObject
	{
	public:

		virtual bool GrowForward() const = 0;

		virtual size_t GetUsedSize() const = 0;

		virtual size_t GetTotalSize() const = 0;

		virtual size_t GetDataSize() const = 0;

		virtual const BYTE* GetBuffer() const = 0;

		virtual const BYTE* GetDataPtr() const = 0;

		virtual void AddData(const char* const pData, size_t dataLength) = 0;
		virtual void AddData(const BYTE* const pData, size_t dataLength) = 0;
		virtual void AddData(char data) = 0;
		virtual void AddData(BYTE data) = 0;

		virtual BYTE* Reserve(size_t bytes) = 0;

		virtual void Use(size_t bytes) = 0;

		virtual void ConsumeData(size_t bytes) = 0;

		virtual void ClearData() = 0;

		virtual void Defrag() = 0;

	public:
		IIOBuffer(long initref = 1) : CRefCountObject(initref) {}
	};
}
#endif // __IIOBUFFER__
