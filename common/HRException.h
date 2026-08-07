// HRException.h: interface for the HRException class.
//
//////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if 0
#include <exception>
#include <string>
#include <sstream>
#endif

class HRException : public std::exception
{
private :
	DWORD				hr_ ;
	mutable std::string sContext_ ;

public :
	HRException ( DWORD hr, const char * sContext ) : hr_ ( hr ), sContext_ ( sContext )
	{
	}
	virtual const char * what () const throw () // cannot say at this time that this code is exception safe....
	{
		LPSTR lpstr ;
		::FormatMessageA ( FORMAT_MESSAGE_ALLOCATE_BUFFER |
							FORMAT_MESSAGE_FROM_SYSTEM |
							FORMAT_MESSAGE_IGNORE_INSERTS,
							0,
							hr_,
							MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
							(LPSTR)&lpstr,
							0,
							0 ) ;
		std::ostringstream ostr ;
		ostr << sContext_ << ", " << " (0x" << std::setw ( 8 ) << std::setfill ( '0' ) << std::hex << hr_ << "), " ;
		if ( lpstr )
		{
			ostr << lpstr  ;
			::LocalFree ( lpstr ) ;
		}
		else
		{
			ostr << "No translation available" ;
		}
		sContext_ = ostr.str () ;
		std::string::size_type s = sContext_.find_first_of ( "\n\r", 0 ) ;
		while ( s != std::string::npos )
		{
			sContext_.erase ( s, 1 ) ;
			s = sContext_.find_first_of ( "\n\r", s ) ;
		}

		return sContext_.c_str () ;
	}
} ;

class InternalErrorException : public std::exception
{
private :
	mutable std::string sContext_ ;

public :
	InternalErrorException ( const char * sContext ) : sContext_ ( sContext )
	{
	}
	const char * what () const throw () // not really sure but only for internal logic type errors
	{
		return sContext_.c_str () ;
	}
} ;

