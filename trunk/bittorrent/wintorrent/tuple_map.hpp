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
// �Ѱ��� Ʃ���� ���� �� �ִ� �ִ� ���Ұ����� 11 �� ������ �����մϴ�

#		ifndef TUPLE_ELEM_MAX
#			define TUPLE_ELEM_MAX	11
#		endif

//--------------------------------------------------
//

#		define dec_tuple_empty(z,n,data) typename Elem##n = data

//--------------------------------------------------
//
// �⺻���� ����

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
// �� �κ� ���� Ư��ȭ�� ���� �ݺ��Ǵ� ����

#	define n BOOST_PP_ITERATION()

#	define spec_tuple_set( z, n, data ) data

#	define dec_tuple_map( z, n, data ) \
	typedef std::map< tuple_set_impl::KEY_PTR< Elem##n >, tuple * > Map##n; \
	typedef typename Map##n##::iterator iterator_##n; \
	typedef typename Map##n##::const_iterator const_iterator_##n;

#	define dec_elem_param( z, n, data ) \
	Elem##n e##n

//--------------------------------------------------
// Ư��ȭ Ŭ���� ����

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
	// Ʃ���� ���� ������ ������ŭ �� ����

	BOOST_PP_REPEAT( n, dec_tuple_map, ~ );
	typedef std::tr1::tuple< BOOST_PP_ENUM_PARAMS( n, Map ) > Maps;

	// ù��° ���� ���� ������ ���
	typedef typename std::tr1::tuple_element< 0, Maps >::type MainMap;

	// ���ĵ� �ε����� ������ �����ϰ� �ϴ� ���� ����
	typedef std::vector< tuple * > IndexList;
	typedef std::tr1::array< IndexList, n > IndexListArray;
	// ���� �Ǿ��ִ� ���¿��� insert, erase �� ȣ��Ǹ� Ŭ����ǰ� at �Լ��� ȣ��� ���� ���õ˴ϴ�.
	mutable IndexListArray indexListArray_; 

	//--------------------------------------------------
	// �����ϸ� 0, �����ϸ� tuple ptr

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
	// ������ �����ϸ� true, �ƴϸ� false �� ����

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
	// stl map �� ������ �������̽� ����

	bool empty() const { return std::tr1::get<0>(maps_).empty(); }

	size_t size() const	{ return std::tr1::get<0>( maps_ ).size(); }

	template< int ElemIdx >
	typename std::tr1::tuple_element< ElemIdx, Maps >::type::const_iterator
		begin() { return std::tr1::get<ElemIdx>( maps_ ).begin(); }

	template< int ElemIdx >
	typename std::tr1::tuple_element< ElemIdx, Maps >::type::const_iterator
		end() { return std::tr1::get<ElemIdx>( maps_ ).end(); }

	//--------------------------------------------------
	// �ʿ� ���ĵ� �ε����� ����

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