///////////////////////////////////////////////////////////////////////////////
//
//  File        : CRefCountObject.h
//  Version     : 1
//  Description : Definition of CRefCountObject
//
//  Author      : Jian-Guang LOU
//  Date:       : 2005-12-06
//
//  Copyright (C) 
//  All rights reserved.              
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __CREFCOUNTOBJECT__
#define __CREFCOUNTOBJECT__

#include "IRefCount.h"
#include "Thread.h"

namespace OS    {

// We cannot make IRefCount virtual base here since the child class CIOBuffer uses a zero-sized array

class CRefCountObject : public IRefCount
{
public:
    CRefCountObject(long initref = 1);
    virtual ~CRefCountObject(){};

	virtual long AddRef();
	virtual long Release();
    virtual long GetRef();

private:
    //OS::CCriticalSection m_crit;
    volatile __declspec(align(4)) long m_ref;
};

}

#endif // __CREFCOUNTOBJECT__
