#pragma once

#include <xmemory>

namespace OS {
class __declspec(novtable) IAllocator
{
public:
    virtual void * alloc(size_t nbytes) = 0;

    virtual void   free(void * addr, size_t nbytes) = 0;

    virtual LONG   getusedsize() = 0;

};

class CHeapAllocator: public IAllocator
{
public:
    explicit CHeapAllocator(SIZE_T dwInitSize = 1024*1024, BOOL bLogSize = TRUE):m_hHeap(NULL),m_lAllocatedSize(0),
		m_bLogSize(bLogSize)
    {
        Initialize(dwInitSize);
    };

    virtual ~CHeapAllocator()
    {
        Unitialize();
    };

    void * alloc(size_t nbytes);

    void   free(void * addr, size_t nbytes);

    virtual LONG   getusedsize() {return m_lAllocatedSize;};
    
protected:
    BOOL Initialize(SIZE_T dwInitSize);

    BOOL Unitialize();

private:
    volatile __declspec(align(4)) LONG m_lAllocatedSize;
    HANDLE m_hHeap;
	BOOL   m_bLogSize;
};

// TEMPLATE FUNCTION _GetHeap
inline IAllocator* GetSVSHeapAllocator()
{
     static  CHeapAllocator  g_SVSHeapAllocator(1024*1024,FALSE);
     return &g_SVSHeapAllocator;
}

// TEMPLATE FUNCTION _Allocate
template<class _Ty> inline
	_Ty  *_SVSAllocate(SIZE_T _Count, _Ty  *)
	{	// check for integer overflow
    IAllocator* pHeap = GetSVSHeapAllocator();
    if (pHeap == NULL)
       _THROW_NCEE(std::bad_alloc, NULL);
	if (_Count <= 0)
		_Count = 0;
	else if (((_SIZT)(-1) / _Count) < sizeof (_Ty))
		_THROW_NCEE(std::bad_alloc, NULL);

		// allocate storage for _Count elements of type _Ty
	return ((_Ty  *)pHeap->alloc(_Count * sizeof (_Ty)));
	}

// TEMPLATE FUNCTION _SVSDeAllocate
template<class _Ty> inline
	void _SVSDeAllocate(_Ty  * _Ptr)
{
    IAllocator* pHeap = GetSVSHeapAllocator();
    if (pHeap)
        pHeap->free(_Ptr, sizeof(_Ty));
    else
        _THROW_NCEE(std::bad_alloc, NULL);
}

		// TEMPLATE FUNCTION _Construct
template<class _T1,
	class _T2> inline
	void _SVSConstruct(_T1  *_Ptr, const _T2& _Val)
	{	// construct object at _Ptr with value _Val
	void  *_Vptr = _Ptr;
	::new (_Vptr) _T1(_Val);
	}

		// TEMPLATE FUNCTION _Destroy
template<class _Ty> inline
	void _SVSDestroy(_Ty  *_Ptr)
	{	// destroy object at _Ptr
	_DESTRUCTOR(_Ty, _Ptr);
	}

template<> inline
	void _SVSDestroy(char  *)
	{	// destroy a char (do nothing)
	}

template<> inline
	void _SVSDestroy(wchar_t  *)
	{	// destroy a wchar_t (do nothing)
	}

// TEMPLATE CLASS allocator using our own heap allocator
template<class _Ty>
	class SVSAllocator
        : public std::_Allocator_base<_Ty>
	{	// generic allocator for objects of class _Ty
public:
    typedef std::_Allocator_base<_Ty> _Mybase;
	typedef typename _Mybase::value_type value_type;
	typedef value_type  *pointer;
	typedef value_type & reference;
	typedef const value_type  *const_pointer;
	typedef const value_type & const_reference;

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	template<class _Other>
		struct rebind
		{	// convert an allocator<_Ty> to an allocator <_Other>
		typedef SVSAllocator<_Other> other;
		};

	pointer address(reference _Val) const
		{	// return address of mutable _Val
		return (&_Val);
		}

	const_pointer address(const_reference _Val) const
		{	// return address of nonmutable _Val
		return (&_Val);
		}

	SVSAllocator() _THROW0()
		{	// construct default allocator (do nothing)
		}

	SVSAllocator(const SVSAllocator<_Ty>&) _THROW0()
		{	// construct by copying (do nothing)
		}

	template<class _Other>
		SVSAllocator(const SVSAllocator<_Other>&) _THROW0()
		{	// construct from a related allocator (do nothing)
		}

	template<class _Other>
		SVSAllocator<_Ty>& operator=(const SVSAllocator<_Other>&)
		{	// assign from a related allocator (do nothing)
		return (*this);
		}

	void deallocate(pointer _Ptr, size_type)
		{	// deallocate object at _Ptr, ignore size
            _SVSDeAllocate(_Ptr);
		}

	pointer allocate(size_type _Count)
		{	// allocate array of _Count elements
		return (_SVSAllocate(_Count, (pointer)0));
		}

	pointer allocate(size_type _Count, const void  *)
		{	// allocate array of _Count elements, ignore hint
		return (allocate(_Count));
		}

	void construct(pointer _Ptr, const _Ty& _Val)
		{	// construct object at _Ptr with value _Val
		_SVSConstruct(_Ptr, _Val);
		}

	void destroy(pointer _Ptr)
		{	// destroy object at _Ptr
		_SVSDestroy(_Ptr);
		}

	size_t max_size() const _THROW0()
		{	// estimate maximum array size
		size_t _Count = (_SIZT)(-1) / sizeof (_Ty);
		return (0 < _Count ? _Count : 1);
		}
	};

		// allocator TEMPLATE OPERATORS
template<class _Ty,
	class _Other> inline
	bool operator==(const SVSAllocator<_Ty>&, const SVSAllocator<_Other>&) _THROW0()
	{	// test for allocator equality (always true)
	return (true);
	}

template<class _Ty,
	class _Other> inline
	bool operator!=(const SVSAllocator<_Ty>&, const SVSAllocator<_Other>&) _THROW0()
	{	// test for allocator inequality (always false)
	return (false);
	}

		// CLASS allocator<void>
template<> class SVSAllocator<void>
	{	// generic allocator for type void
public:
	typedef void _Ty;
	typedef _Ty  *pointer;
	typedef const _Ty  *const_pointer;
	typedef _Ty value_type;

	template<class _Other>
		struct rebind
		{	// convert an allocator<void> to an allocator <_Other>
		typedef SVSAllocator<_Other> other;
		};

	SVSAllocator() _THROW0()
		{	// construct default allocator (do nothing)
		}

	SVSAllocator(const SVSAllocator<_Ty>&) _THROW0()
		{	// construct by copying (do nothing)
		}

	template<class _Other>
		SVSAllocator(const SVSAllocator<_Other>&) _THROW0()
		{	// construct from related allocator (do nothing)
		}

	template<class _Other>
		SVSAllocator<_Ty>& operator=(const SVSAllocator<_Other>&)
		{	// assign from a related allocator (do nothing)
		return (*this);
		}
	};

}
