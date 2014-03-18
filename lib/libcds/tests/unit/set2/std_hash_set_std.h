//$$CDS-header$$

#ifndef __CDSUNIT_STD_HASH_SET_STD_H
#define __CDSUNIT_STD_HASH_SET_STD_H

#include <unordered_set>
#include <cds/ref.h>

namespace set2 {

    template <typename Value, typename Hash, typename Less, typename EqualTo, typename Lock,
        class Alloc = typename CDS_DEFAULT_ALLOCATOR::template rebind<Value>::other
    >
    class StdHashSet
        : public std::unordered_set<
            Value
            , Hash
            , EqualTo
            , Alloc
        >
    {
    public:
        Lock m_lock    ;
        typedef cds::lock::scoped_lock<Lock> AutoLock ;
        typedef std::unordered_set<
            Value
            , Hash
            , EqualTo
            , Alloc
        >   base_class ;
    public:
        typedef typename base_class::value_type value_type ;

        StdHashSet( size_t nSetSize, size_t nLoadFactor )
        {}

        template <typename Key>
        bool find( const Key& key )
        {
            AutoLock al( m_lock )    ;
            return base_class::find( value_type(key) ) != base_class::end() ;
        }

        template <typename Key>
        bool insert( Key const& key )
        {
            AutoLock al( m_lock )    ;
            std::pair<typename base_class::iterator, bool> pRet = base_class::insert( value_type( key )) ;
            return pRet.second ;
        }

        template <typename Key, typename Func>
        bool insert( Key const& key, Func func )
        {
            AutoLock al( m_lock )    ;
            std::pair<typename base_class::iterator, bool> pRet = base_class::insert( value_type( key )) ;
            if ( pRet.second ) {
                cds::unref(func)( *pRet.first ) ;
                return true ;
            }
            return false;
        }

        template <typename T, typename Func>
        std::pair<bool, bool> ensure( const T& key, Func func )
        {
            AutoLock al( m_lock )   ;
            std::pair<typename base_class::iterator, bool> pRet = base_class::insert( value_type( key )) ;
            if ( pRet.second ) {
                cds::unref(func)( true, *pRet.first, key ) ;
                return std::make_pair( true, true ) ;
            }
            else {
                cds::unref(func)( false, *pRet.first, key ) ;
                return std::make_pair( true, false ) ;
            }
        }

        template <typename Key>
        bool erase( const Key& key )
        {
            AutoLock al( m_lock )   ;
            return base_class::erase( value_type(key) ) != 0 ;
        }

        template <typename T, typename Func>
        bool erase( const T& key, Func func )
        {
            AutoLock al( m_lock )   ;
            typename base_class::iterator it = base_class::find( value_type(key) ) ;
            if ( it != base_class::end() ) {
                cds::unref(func)( *it )   ;
                return base_class::erase( it ) != base_class::end() ;
            }
            return false ;
        }

        std::ostream& dump( std::ostream& stm ) { return stm; }
    };
}   // namespace set2

#endif  // #ifndef __CDSUNIT_STD_HASH_SET_STD_H
