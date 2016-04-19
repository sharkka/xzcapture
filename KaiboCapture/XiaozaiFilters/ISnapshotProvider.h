// Static Model


#ifndef __ISNAPSHOTPROVIDER__
#define __ISNAPSHOTPROVIDER__

MIDL_INTERFACE("4D474BEC-86FC-4681-9DC2-D034598D7D96")
ISnapshotProvider : public IUnknown
{

public:

	virtual HRESULT STDMETHODCALLTYPE GetCurrentSnapshot(BYTE *ppData, DWORD *datalen)=0;

	virtual HRESULT STDMETHODCALLTYPE GetBMPSetting(LONG *width, LONG *height, WORD *bitcount)=0;

	virtual HRESULT STDMETHODCALLTYPE SetRefresgFrequency(DWORD frequency)=0;

};// END INTERFACE DEFINITION ISnapshotProvider

#endif // __ISNAPSHOTPROVIDER__
