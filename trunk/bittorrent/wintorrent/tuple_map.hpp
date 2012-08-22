#ifndef BOOST_PP_IS_ITERATING

#	ifndef TUPLE_SET_HPP_INCLUDE
#		define TUPLE_SET_HPP_INCLUDE

#		include <boost/preprocessor/repetition.hpp>
#		include <boost/preprocessor/arithmetic/sub.hpp>
#		include <boost/preprocessor/punctuation/comma_if.hpp>
#		include <boost/preprocessor/iteration/iterate.hpp>
#		include <map>
#		include <vector>
#		include <array>
#		include <tuple>
#		include <algorithm>
#		include "lexical_cast.hpp"

//--------------------------------------------------
//

namespace tuple_set_impl
{
	template< typename T >
	struct KEY_PTR
	{
		typedef T type;

		T * p;
		KEY_PTR() : p(0)			{}
		KEY_PTR( T * p ) : p(p)		{}
		KEY_PTR( T & p ) : p(&p)	{}
		KEY_PTR( T && p ) : p(&p)	{}

		bool operator<( KEY_PTR const & other ) const
		{
			if( !p ) 
				return false;

			if( !other.p ) 
				return true;

			return (*p) < (*other.p);
		}

		bool operator==( KEY_PTR const & other ) const {
			return (p && other.p) ? (*p == *other.p) : (false);
		}
	};
}

//--------------------------------------------------
//
// 한개의 튜플이 가질 수 있는 최대 원소갯수를 11 개 정도로 지정합니다

#		ifndef TUPLE_ELEM_MAX
#			define TUPLE_ELEM_MAX	11
#		endif

//--------------------------------------------------
//

#		define dec_tuple_empty(z,n,data) typename Elem##n = data

//--------------------------------------------------
//
// 기본형식 정의

template< BOOST_PP_ENUM( TUPLE_ELEM_MAX, dec_tuple_empty, std::tr1::_Nil ) >
class tuple_map {};

//--------------------------------------------------
//

#		undef dec_tuple_empty

#		define BOOST_PP_ITERATION_LIMITS (1, TUPLE_ELEM_MAX - 1)
#		define BOOST_PP_FILENAME_1 "tuple_map.hpp"
#		include BOOST_PP_ITERATE()

#	endif // TUPLE_SET_HPP_INCLUDE

//--------------------------------------------------
//

#else // BOOST_PP_IS_ITERATING

//--------------------------------------------------
// 이 부분 부터 특수화를 위해 반복되는 구간

#	define n BOOST_PP_ITERATION()

#	define spec_tuple_set( z, n, data ) data

#	define dec_tuple_map( z, n, data ) \
	typedef std::map< tuple_set_impl::KEY_PTR< Elem##n >, tuple * > Map##n; \
	typedef typename Map##n##::iterator iterator_##n; \
	typedef typename Map##n##::const_iterator const_iterator_##n;

#	define dec_elem_param( z, n, data ) \
	Elem##n e##n

//--------------------------------------------------
// 특수화 클래스 정의

template< BOOST_PP_ENUM_PARAMS( n, typename Elem ) >
class tuple_map< 
	BOOST_PP_ENUM_PARAMS( n, Elem )
	BOOST_PP_COMMA_IF( n )
	BOOST_PP_ENUM( BOOST_PP_SUB( TUPLE_ELEM_MAX, n ), spec_tuple_set, std::tr1::_Nil ) 
>
{
public:
	//--------------------------------------------------
	//

	tuple_map() {}

	tuple_map( tuple_map && other ) : maps_( std::move(other.maps_) ) {}

	~tuple_map() { clear(); }

	typedef std::tr1::tuple< BOOST_PP_ENUM_PARAMS(n, Elem) > tuple;
	typedef tuple const * value_type;

	//--------------------------------------------------
	// 튜플이 갖는 원소의 갯수만큼 맵 선언

	BOOST_PP_REPEAT( n, dec_tuple_map, ~ );
	typedef std::tr1::tuple< BOOST_PP_ENUM_PARAMS( n, Map ) > Maps;

	// 첫번째 맵을 메인 맵으로 사용
	typedef typename std::tr1::tuple_element< 0, Maps >::type MainMap;

	// 정렬된 인덱스로 참조를 가능하게 하는 백터 선언
	typedef std::vector< tuple * > IndexList;
	typedef std::tr1::array< IndexList, n > IndexListArray;
	// 선언만 되어있는 상태에서 insert, erase 가 호출되면 클리어되고 at 함수가 호출될 때만 세팅됩니다.
	mutable IndexListArray indexListArray_; 

	//--------------------------------------------------
	// 성공하면 0, 실패하면 tuple ptr

	tuple const * insert( BOOST_PP_ENUM( n, dec_elem_param, ~ ) )
	{

#	define tuple_elem_find_spec( z, n, data ) \
	tuple const * t##n = find<n>( e##n ); \
	if( t##n ) return t##n;

		BOOST_PP_REPEAT( n, tuple_elem_find_spec, ~ );

#	undef tuple_elem_find_spec

		tuple * t = new tuple( BOOST_PP_ENUM_PARAMS( n, e ) );

#	define tuple_insert_spec( z, n, data ) \
	insert_impl<n>( t );

		BOOST_PP_REPEAT( n, tuple_insert_spec, ~ );

#	undef tuple_insert_spec

		for( int i=0; i<n; ++i)
			indexListArray_[i].clear();

		return 0;
	}

	//--------------------------------------------------
	//

	template< int ElemIdx, typename Arg >
	typename std::tr1::tuple_element< ElemIdx, Maps >::type::const_iterator
		find_iter( Arg arg ) const
	{
		typedef typename std::tr1::tuple_element< ElemIdx, Maps >::type Map;

		Map const & map = std::tr1::get< ElemIdx >( maps_ );

		Map::key_type::type key = umtl::lexical_cast< Map::key_type::type >(arg);

		return map.find( typename Map::key_type(key) );
	}

	//--------------------------------------------------
	//

	template< int ElemIdx, typename Key >
	tuple const * find( Key key ) const
	{
		typedef typename std::tr1::tuple_element< ElemIdx, Maps >::type Map;

		Map const & map = std::tr1::get< ElemIdx >( maps_ );

		Map::const_iterator iter = find_iter<ElemIdx>(key);

		return iter == map.end() ? 0 : iter->second;
	}

	//--------------------------------------------------
	// 삭제를 성공하면 true, 아니면 false 를 리턴

	template< int ElemIdx >
	bool erase( typename std::tr1::tuple_element< ElemIdx, Maps >::type::key_type::type key )
	{
		tuple const * t = find< ElemIdx >( key );

		if( t )
		{
#define tuple_erase_spec( z, n, data ) erase_impl<n>( const_cast< tuple& >(*t) );

			BOOST_PP_REPEAT( n, tuple_erase_spec, ~ );

#undef tuple_erase_spec

			delete t;

			indexListArray_[ElemIdx].clear();

			return true;
		}

		return false;
	}

	//--------------------------------------------------
	//

	template< int ElemIdx >
	typename std::tr1::tuple_element< ElemIdx, Maps >::type::const_iterator
		erase( typename std::tr1::tuple_element< ElemIdx, Maps >::type::const_iterator iter )
	{
		if( iter == end<ElemIdx>() ) 
			return iter;

		std::tr1::tuple_element< ElemIdx, Maps >::type::const_iterator next_iter = iter;

		++next_iter;

		if( iter->second )
			erase< ElemIdx >( std::tr1::get< ElemIdx >( *const_cast< tuple* >( iter->second ) ) );

		return next_iter;
	}

	//--------------------------------------------------
	//

	void clear()
	{
#define tuple_clear_spec( z, n, data ) clear_impl<n>();

		BOOST_PP_REPEAT( n, tuple_clear_spec, ~ );

#undef tuple_clear_spec

		for( int i=0; i<n; ++i)
			indexListArray_[i].clear();
	}

	//--------------------------------------------------
	// stl map 과 동일한 인터페이스 지원

	bool empty() const { return std::tr1::get<0>(maps_).empty(); }

	size_t size() const	{ return std::tr1::get<0>( maps_ ).size(); }

	template< int ElemIdx >
	typename std::tr1::tuple_element< ElemIdx, Maps >::type::const_iterator
		begin() { return std::tr1::get<ElemIdx>( maps_ ).begin(); }

	template< int ElemIdx >
	typename std::tr1::tuple_element< ElemIdx, Maps >::type::const_iterator
		end() { return std::tr1::get<ElemIdx>( maps_ ).end(); }

	//--------------------------------------------------
	// 맵에 정렬된 인덱스로 참조

	template< int ElemIdx >
	tuple const * at( size_t const idx ) const
	{
		return idx < size() ? indexing<ElemIdx>()[idx] : 0 ;
	}

	template< int ElemIdx >
	IndexList & indexing() const
	{
		IndexList & indexList = indexListArray_[ElemIdx];

		if( indexList.empty() )
		{
			if( indexList.capacity() < size() )
				indexList.reserve( size() );

			MainMap const & map = std::tr1::get<ElemIdx>(maps_);

			for( MainMap::const_iterator iter = map.begin(); iter != map.end(); ++iter )
				indexList.push_back( iter->second );
		}

		return indexList;
	}

	//--------------------------------------------------
	//

private:

	Maps maps_;

	template< int MapIdx >
	void insert_impl( tuple * t )
	{
		typedef typename std::tr1::tuple_element< MapIdx, Maps >::type Map;
		typedef typename Map::key_type KeyType;

		Map & map = std::tr1::get< MapIdx >( maps_ );

		map.insert( std::make_pair( KeyType( std::tr1::get< MapIdx >( *t ) ) , t ) );
	}

	template< int MapIdx >
	void erase_impl( tuple & t )
	{
		typedef typename std::tr1::tuple_element< MapIdx, Maps >::type Map;
		typedef typename Map::key_type KeyType;

		Map & map = std::tr1::get< MapIdx >( maps_ );

		Map::const_iterator iter = map.find( KeyType( std::tr1::get<MapIdx>(t) ) );

		if( iter != map.end() )
			map.erase( iter );
	}

	template< int MapIdx >
	void clear_impl()
	{
		typedef typename std::tr1::tuple_element< MapIdx, Maps >::type Map;

		Map & map = std::tr1::get< MapIdx >( maps_ );

		if( MapIdx == 0 )
		{
			std::for_each( map.begin(), map.end(),
				[]( Map::value_type & elem )
			{
				if( elem.second )
					delete elem.second;
			}
			);
		}

		map.clear();
	}

	tuple_map( tuple_map const & other );
	void operator=( tuple_map const & other );
};

#	undef dec_tuple_map
#	undef dec_elem_param
#	undef spec_tuple_set
#	undef n

#endif // BOOST_PP_IS_ITERATING