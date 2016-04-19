///////////////////////////////////////////////////////////////////////////////
//
//  File        : CRefCountObject.cpp
//  Version     : 1
//  Description : Implementation of CRefCountObject
//
//  Author      : Jian-Guang LOU
//  Date:       : 2005-12-06
//
//  Copyright (C) 
//  All rights reserved.              
//
//  Revision history: 
//
//
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "CRefCountObject.h"
#include "Exception.h"

namespace OS {

CRefCountObject::CRefCountObject(long initref)
    : m_ref(initref)
{
}

long CRefCountObject::AddRef()
{
    //CCriticalSection::AutoLock lock(m_crit);
    long ref = ::InterlockedIncrement(&m_ref);
    return ref;
}

long CRefCountObject::Release()
{
    //CCriticalSection::AutoLock lock(m_crit);
    long ref = ::InterlockedDecrement(&m_ref);
    if (ref == -1)      // Error! double Release
    {
        throw CGeneralException(_T("CRefCountObject::Release()"), _T("m_ref is already 0"));
    }
    return ref;
}

long CRefCountObject::GetRef()
{
    //CCriticalSection::AutoLock lock(m_crit);
    return m_ref;
}

}