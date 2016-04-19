#include "stdafx.h"

#include "ID.h"

namespace OS
{

	_tstring ToString(const ID & id)
	{
		char idstr[40];
		idstr[39] = 0;
		id.ID2STR(idstr);
		return ToString(idstr);
	}

	_tstring ToString(const GUID & guid)
	{
		return ToString(ID(guid));
	}

	_tstring ToStringWithoutBraces(const ID & id)
	{
		char idstr[40];
		idstr[39] = 0;
		id.ID2STRWithoutBraces(idstr);
		return ToString(idstr);
	}

	_tstring ToStringWithoutBraces(const GUID & guid)
	{
		return ToStringWithoutBraces(ID(guid));
	}
}