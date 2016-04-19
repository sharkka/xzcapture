/////////////////////////////////////////////////////////////////////////////////////////////////////
//! <pre><b>Jian-Guang LOU</b></pre>
//! <pre><b>Copyright (c) 2007 </b></pre>
//! @file ITracer.h
//! @author jlou@microsoft.com
//! @brief ITracer interface
//! @Date 2007/05/28
/////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "TString.h"

namespace OS {
class CGeneralException
{
 public :

    CGeneralException(
                      const _tstring & where,
                      const _tstring & message);

    virtual ~CGeneralException() {}

	virtual _tstring GetWhere() const;

    virtual _tstring GetExceptionMessage() const;

protected:

    _tstring m_where;

    _tstring m_message;
};

class CWin32Exception : public CGeneralException
{
 public :

    CWin32Exception(const _tstring &where, DWORD error);

    DWORD GetError() const;

 protected :

    DWORD m_error;
};

}