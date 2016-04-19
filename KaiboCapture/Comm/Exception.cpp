#include "stdafx.h"
#include "TString.h"
#include "Exception.h"
#include "Win32_Utils.h"

namespace OS{

///////////////////////////////////////////////////////////////////////////////
// CGeneralException
///////////////////////////////////////////////////////////////////////////////

CGeneralException::CGeneralException(
                                     const _tstring &where,
                                     const _tstring &message)
    :  m_where(where),
    m_message(message)
{

}

_tstring CGeneralException::GetWhere() const
{
    return m_where;
}

_tstring CGeneralException::GetExceptionMessage() const
{
    return m_message;
}

CWin32Exception::CWin32Exception(
                                 const _tstring &where,
                                 DWORD error)
    :  CGeneralException(where, _T("[") + ToString(error) + _T("] ") + GetLastErrorMessage(error)),
    m_error(error)
{
}

DWORD CWin32Exception::GetError() const
{
    return m_error;
}

}