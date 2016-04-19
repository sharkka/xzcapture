///////////////////////////////////////////////////////////////////////////////
//
//  File        : IRefCount.h
//  Version     : 1
//  Description : Definition of IRefCount
//
//  Author      : Jian-Guang LOU
//  Date:       : 2005-12-06
//
//  Copyright (C) 
//  All rights reserved.              
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __IREFCOUNT__
#define __IREFCOUNT__

namespace OS
{

	// interface for object reference count
	class IRefCount
	{

	public:

		virtual long AddRef() = 0;

		virtual long Release() = 0;

	};// END INTERFACE DEFINITION IRefCount
}
#endif // __IREFCOUNT__
