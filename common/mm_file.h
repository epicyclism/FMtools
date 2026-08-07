//
// mm_file.h
//
// Windows/Linux minimal memory mapped read only file wrapper.
//
//
// Copyright (c) 2004-2020 Paul Ranson. paul@epicyclism.com
//
#pragma once

// for std::unique_ptr
#include <memory>

#if defined (_WIN32)
#define WIN32_LEAN_AND_MEAN
#include "targetver.h"
#include <windows.h>


// give std::unique_ptr a hand
inline void* from_HANDLE(HANDLE h)
{
	return static_cast<void*>(h);
}

inline HANDLE to_HANDLE(void* v)
{
	return static_cast<HANDLE>(v);
}

// necessary?
inline void unmap(LPCVOID p)
{
	::UnmapViewOfFile(p);
}

inline void close_handle(void* h)
{
	::CloseHandle(to_HANDLE(h));
}

template <typename T> class mem_map_file
{
private :
	std::unique_ptr < void, decltype(&unmap)> pV_ ;
    LARGE_INTEGER nL_ ;

public :
	mem_map_file() : pV_{ nullptr, &unmap}
    {
#if defined(_M_X64 )
		nL_.QuadPart = 0;
#else
		nL_.LowPart = 0;
		nL_.HighPart = 0;
#endif
	}
	mem_map_file( LPCWSTR sName ) : pV_{ nullptr, &unmap}
    {
        open ( sName ) ;
    }
	mem_map_file( LPCSTR sName ) : pV_{ nullptr, &unmap}
    {
        open ( sName ) ;
    }
	// filename goes straight through to the API, so std::string_view a bit trickier
	//
    bool open ( LPCWSTR sName )
    {
		std::unique_ptr < void, decltype(&close_handle)> hF
					( from_HANDLE(::CreateFileW ( sName,
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL )),
							&close_handle) ;
	    if ( hF.get() == INVALID_HANDLE_VALUE )
        {
		    return false ;
        }
	    ::GetFileSizeEx ( hF.get(), &nL_ ) ;

		std::unique_ptr < void, decltype(&close_handle)> hFM 
					( from_HANDLE(::CreateFileMapping ( hF.get(),
							    0,
							    PAGE_READONLY,
							    nL_.HighPart,
							    nL_.LowPart,
							    0 )),
								&close_handle) ;
	    if ( !hFM )
	    {
		    return false ;
	    }

	    pV_.reset( ::MapViewOfFile ( to_HANDLE(hFM.get()),
							    FILE_MAP_READ,
							    0,
							    0,
							    nL_.LowPart )) ;

        return !!pV_; // note 'true' or 'false' for successful mapping rather than the value of the ptr!
    }
    
    bool open ( LPCSTR sName )
    {
		std::unique_ptr < void, decltype(&close_handle)> hF
						(from_HANDLE(::CreateFileA(sName,
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL )),
							close_handle) ;
	    if ( hF.get() == INVALID_HANDLE_VALUE )
        {
		    return false ;
        }
	    ::GetFileSizeEx ( hF.get(), &nL_ ) ;

		std::unique_ptr < void, decltype(&close_handle)> hFM
						(from_HANDLE(::CreateFileMapping(static_cast<HANDLE>(hF.get()),
								0,
							    PAGE_READONLY,
							    nL_.HighPart,
							    nL_.LowPart,
							    0 )),
								&close_handle) ;
	    if ( !hFM )
	    {
		    return false ;
	    }

	    pV_.reset( (::MapViewOfFile (static_cast<HANDLE>(hFM.get()),
							    FILE_MAP_READ,
							    0,
							    0,
							    nL_.LowPart ))) ;

        return !!pV_ ; // note 'true' or 'false' for successful mapping rather than the value of the ptr!
    }
    size_t bytelength () const
    {
#if defined(_M_X64 )
        return nL_.QuadPart ;
#else
		return nL_.LowPart;
#endif
    }
    size_t length () const
    {
#if defined(_M_X64 )
        return nL_.QuadPart / sizeof(T);
#else
		return nL_.LowPart / sizeof(T);
#endif
    }
    
	T const* ptr () const
	{
		return reinterpret_cast< T const *>( pV_.get()) ;
	}

	template <typename P> P const * ptrT ( size_t off ) const
	{
		return reinterpret_cast< P const *>( reinterpret_cast<unsigned char const*>( pV_.get()) + off ) ;
	}

	operator const T* () const
	{
        return reinterpret_cast<const T*>( pV_.get()) ;
    }

    const T* begin() const
	{
        return reinterpret_cast<const T*>( pV_.get()) ;
    }

    const T* end() const
    {
#if defined(_M_X64 )
        return reinterpret_cast<const T*>(pV_.get()) + (nL_.QuadPart/sizeof(T));
#else
        return reinterpret_cast<const T*>(pV_.get()) + (nL_.LowPart/sizeof(T));
#endif
    }

    void close ()
    {
        pV_.release () ;
#if defined(_M_X64 )
        nL_.QuadPart = 0 ;
#else
		nL_.LowPart = 0;
		nL_.HighPart = 0;
#endif
    }

    operator bool () const
    {
        return pV_ != nullptr ;
    }

	bool operator ! () const
    {
        return pV_ == 0 ;
    }
} ;
#else
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

template < typename T > class mem_map_file
{
private:
	off_t sz_;
	void const* pv_;

public:
	mem_map_file() : sz_(0), pv_((void const*)-1)
	{
	}
	mem_map_file(const char* sName)
	{
		open(sName);
	}
	~mem_map_file()
	{
		close();
	}
	bool open(const char* sName)
	{
		int fd = ::open(sName, O_RDONLY);
		if (fd < 0)
			return false;

		struct stat st;
		if (::fstat(fd, &st) < 0)
		{
			::close(fd);
			return false;
		}
		sz_ = st.st_size;

		pv_ = ::mmap(0, sz_, PROT_READ, MAP_PRIVATE, fd, 0);
		::close(fd);

		return pv_; // note 'true' or 'false' for successful mapping rather than the value of the ptr!
	}

	size_t bytelength() const
	{
		return sz_;
	}

	size_t length() const
	{
		return sz_ / sizeof(T);
	}

	T const* ptr() const
	{
		return reinterpret_cast< T const *>(pv_);
	}

	template <typename P> P const * ptrT(size_t off) const
	{
		return reinterpret_cast< P const *>(reinterpret_cast<unsigned char const*>(pv_) + off);
	}

	operator const T* () const
	{
		return reinterpret_cast<const T*>(pv_);
	}

	void close()
	{
		if (pv_ != (void const*)-1)
		{
			::munmap(const_cast<void*>(pv_), sz_);
			pv_ = (void const*)-1;
			sz_ = 0;
		}
	}

	operator bool() const
	{
		return pv_ != (void const*)-1;
	}
	const T* begin() const
	{
		return reinterpret_cast<const T*>(pv_);
	}

	const T* end() const
	{
		return reinterpret_cast<const T*>(pv_) + (sz_ / sizeof(T));
	}
};
#endif