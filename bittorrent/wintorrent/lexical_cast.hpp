#pragma once
#include <cstdlib>
#include <string>
#include <sstream>
#include <windows.h>
#ifndef _DEBUG
#include <boost/lexical_cast.hpp>
#endif
#include <boost/program_options/detail/utf8_codecvt_facet.hpp>
#include <vector>

namespace umtl
{
	/////////////////////////////////////
	// T -> string or wstring

	template< typename R >
	struct __select_cast_func_ {
		template< typename T >
		inline static R cast( T t ) {
			return (R)t;
		}
	};

	template<typename R, typename T>
	inline R lexical_cast( T t )
	{
		return __select_cast_func_< R >::cast( t );
	}

	template<>
	struct __select_cast_func_< std::string > {
		template< typename T >
		inline static std::string cast( T t )  { 
#ifdef _DEBUG
			std::stringstream str;
			str << t;
			return str.str();
#else
			return boost::lexical_cast< std::string >(t);
#endif
		}
	};

	template<>
	struct __select_cast_func_< std::wstring > {
		template< typename T >
		inline static std::wstring cast( T t )  { 
			std::wstringstream str;
			str << t;// << std::ends;
			return str.str();
		}
	};

	////////////////////////////////////
	// char const * -> R

	template< typename R >
	inline R lexical_cast( char const * t )
	{
		return t ? t : R();
	};

	template<>
	inline char* lexical_cast( char const * t )
	{
		return t ? const_cast<char*>(t) : 0;
	}

	template<>
	inline int lexical_cast( char const * t )
	{
		return t ? atoi(t) : 0;
	}

	template<>
	inline unsigned int lexical_cast( char const * t )
	{
		return t ? (unsigned int)atoi(t) : 0;
	}

	template<>
	inline short int lexical_cast( char const * t )
	{
		return t ? (short int)atoi(t) : 0;
	}

	template<>
	inline unsigned short int lexical_cast( char const * t )
	{
		return t ? (unsigned short int)atoi(t) : 0;
	}

	template<>
	inline long int lexical_cast( char const * t )
	{
		return t ? (long int)atoi(t) : 0;
	}

	template<>
	inline unsigned long int lexical_cast( char const * t )
	{
		return t ? (unsigned long int)atoi(t) : 0;
	}

	template<>
	inline float lexical_cast( char const * t )
	{
		return t ? (float)atof(t) : 0;
	}

	template<>
	inline double lexical_cast( char const * t )
	{
		return t ? atof(t) : 0;
	}

	template<>
	inline __int64 lexical_cast( char const * t )
	{
		return t ? _atoi64( t ) : 0;
	}

	template<>
	inline unsigned __int64 lexical_cast( char const * t )
	{
		return t ? (unsigned __int64)_atoi64( t ) : 0;
	}

	template<>
	inline std::wstring lexical_cast( char const * t )
	{
		std::wstring r;

		if( t )
		{
			size_t len = strlen(t) + 1;
			wchar_t * buf = new wchar_t[ len ];
			//mbstowcs_s( 0, buf, len, t, len );
			len = MultiByteToWideChar( CP_ACP, 0, t, -1, buf, len );
			buf[len-1] = 0;

			r = buf;
			delete[] buf;
		}

		return r;
	}

	////////////////////////////////////
	// string -> R

	template<typename R>
	inline R lexical_cast( std::string const & t )
	{
		return lexical_cast<R>(t.c_str());
	}

	template<>
	inline std::string lexical_cast( std::string const & t )
	{
		return t;
	}

	////////////////////////////////////
	// wchar_t -> R

	template< typename R >
	inline R lexical_cast( wchar_t const * t )
	{
		std::string s = lexical_cast< std::string >(t);
		return lexical_cast< R >( s.c_str() );
	}

	template<>
	inline wchar_t * lexical_cast( wchar_t const  * t )
	{
		return t ? const_cast< wchar_t * >(t) : 0;
	}

	template<>
	inline std::string lexical_cast( wchar_t const * t )
	{
		std::string r;

		if( t )
		{
			size_t len = wcslen( t ) * 2 + 1 ;
			char * buf = new char[ len ];
			//wcstombs_s( 0, buf, len, t, len );
			WideCharToMultiByte( CP_ACP, 0, t, len, buf, len, 0, 0 );
			buf[len-1] = 0;

			r = buf;
			delete[] buf;
		}

		return r;
	}

	///////////////////////////////////////
	// wstring -> R

	template<typename R>
	inline R lexical_cast( std::wstring const & t )
	{
		return lexical_cast<R>(t.c_str());
	}

	template<>
	inline std::wstring lexical_cast( std::wstring const & t )
	{
		return t;
	}

	///////////////////////////////////////
	//

	typedef std::codecvt<wchar_t, char, std::mbstate_t> codecvt_type;

	struct narrower : std::unary_function<std::wstring, std::string>
	{
		std::locale                 loc_;
		codecvt_type const&			codecvt_;
	public:
		narrower(const std::locale& loc = std::locale()) : loc_(loc),
#if defined(_MSC_VER) && (_MSC_VER < 1300)
			type_ ( _USE(loc, ctype<codecvt_type> ) )
#else
			codecvt_ ( std::use_facet<codecvt_type>(loc_) )
#endif
		{}

		std::string operator() (const std::wstring& src) const
		{
			std::mbstate_t state = std::mbstate_t();
			std::vector<char> buf((src.size()+1) * codecvt_.max_length());
			wchar_t const* in= src.c_str();
			char* out= &buf[0];
			std::codecvt_base::result result
				= codecvt_.out(state, src.c_str(), src.c_str() + src.size(), in, &buf[0], &buf[0] + buf.size(), out);

			if (result == std::codecvt_base::error)
				return std::string();

			for( size_t i=0; i<buf.size(); ++i)
			{
				if( buf[i] == ' ' || buf[i] == '#' )
					buf[i] = '+';
			}

			return std::string(&buf[0]);
		}
	};

	///////////////////////////////////////
	//

	std::string to_utf8( std::string & text );

	///////////////////////////////////////
}

