///////////////////////////////////////////////////////////////////////////////
//
//  File        : RawUserData.h
//  Version     : 1
//  Description : Definition of CRawUserData
//
//  Author      : Jian-Guang LOU
//  Date:       : 2005-12-06
//
//  Copyright (C) 
//  All rights reserved.              
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __RAW_USER_DATA__
#define __RAW_USER_DATA__
#pragma warning(disable:4311 4312)
///////////////////////////////////////////////////////////////////////////////
// CRawUserData
///////////////////////////////////////////////////////////////////////////////
class CRawUserData 
{
public:
    CRawUserData()
        : m_pUserData(0) {}
    ~CRawUserData() 
        { m_pUserData = 0; }

    void *GetUserPtr() const
    {         
        return InterlockedExchangePointer(&(const_cast<void*>(m_pUserData)), m_pUserData);
    }

    void SetUserPtr(void *pData)
    {
        InterlockedExchangePointer(&m_pUserData, pData);
    }

    unsigned long GetUserData() const
    {
        return reinterpret_cast<unsigned long>(GetUserPtr());
    }

    void SetUserData(unsigned long data)
    {
        SetUserPtr(reinterpret_cast<void*>(data));
    }

private :
    __declspec(align(8)) void *m_pUserData;
};

#endif // __RAW_USER_DATA__

