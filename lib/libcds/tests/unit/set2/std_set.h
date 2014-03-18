//$$CDS-header$$

#ifndef __CDSUNIT_STD_SET_VC_H
#define __CDSUNIT_STD_SET_VC_H

#include <set>
#include <cds/ref.h>

namespace set2 {
    template <typename Value, typename Less, typename Lock,
        class Alloc = typename CDS_DEFAULT_ALLOCATOR::template rebind<Value>::other
    >
    class StdSet: public std::set<Value, Less, Alloc>
    {
        Lock m_lock    ;
        typedef cds::lock::scoped_lock<Lock> AutoLock ;
        typedef std::set<Value, Less, Alloc> base_class ;
    public:
        typedef typename base_class::key_type value_type ;

        StdSet( size_t nMapSize, size_t nLoadFactor )
        {}

        template <typename Key>
        bool find( const Key& key )
        {
            value_type v( key )  ;
            AutoLock al( m_lock )    ;
            return base_class::find( v ) != base_class::end() ;
        }

        bool insert( value_type const& v )
        {
            AutoLock al( m_lock )    ;
            return base_class::insert( v ).second ;
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
            typename base_class::iterator it = base_class::find( value_type(key) )   ;
            if ( it != base_class::end() ) {
                cds::unref(func)( *it )    ;

                base_class::erase( it )     ;
                return true ;
            }
            return false;
        }

        std::ostream& dump( std::ostream& stm ) { return stm; }
    };
} // namespace set2

#endif  // #ifndef __CDSUNIT_STD_MAP_VC_H
