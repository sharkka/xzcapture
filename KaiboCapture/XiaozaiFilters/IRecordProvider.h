// Static Model


#ifndef __IRECORDPROVIDER__
#define __IRECORDPROVIDER__

MIDL_INTERFACE("19197516-52C3-45E2-AA37-FFAE6E8A4EDF")
IRecordProvider : public IUnknown
{

public:

	virtual HRESULT STDMETHODCALLTYPE SetRecordPath(const BSTR outputPath)=0;

};// END INTERFACE DEFINITION IRecordProvider

#endif // __IRECORDPROVIDER__
