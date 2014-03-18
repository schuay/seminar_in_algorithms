//$$CDS-header$$

#ifndef __CDS_CONTAINER_STRIPED_MAP_STD_HASH_MAP_MSVC_ADAPTER_H
#define __CDS_CONTAINER_STRIPED_MAP_STD_HASH_MAP_MSVC_ADAPTER_H

#ifndef __CDS_CONTAINER_STRIPED_MAP_STD_HASH_MAP_ADAPTER_H
#   error <cds/container/striped_map/std_hash_map.h> must be included instead of <cds/container/striped_map/std_hash_map_vc.h> header
#endif

#include <cds/container/striped_set/adapter.h>
#include <hash_map>

//@cond
namespace cds { namespace container {
    namespace striped_set {

        // Copy policy for map
        template <typename Key, typename T, typename Traits, typename Alloc>
        struct copy_item_policy< stdext::hash_map< Key, T, Traits, Alloc > >
        {
            typedef stdext::hash_map< Key, T, Traits, Alloc > map_type ;
            typedef typename map_type::value_type pair_type ;
            typedef typename map_type::iterator iterator ;

            void operator()( map_type& map, iterator itWhat )
            {
                std::pair< typename map_type::iterator, bool> res = map.insert( *itWhat );
                assert( res.second )    ; // succesful insert
            }
        };

        // Swap policy for map
        template <typename Key, typename T, typename Traits, typename Alloc>
        struct swap_item_policy< stdext::hash_map< Key, T, Traits, Alloc > >
        {
            typedef stdext::hash_map< Key, T, Traits, Alloc > map_type ;
            typedef typename map_type::value_type pair_type ;
            typedef typename map_type::iterator iterator ;

            void operator()( map_type& map, iterator itWhat )
            {
                pair_type newVal( itWhat->first, typename pair_type::second_type() ) ;
                std::pair< typename map_type::iterator, bool> res = map.insert( newVal );
                assert( res.second )    ; // succesful insert
                std::swap( res.first->second, itWhat->second )    ;
            }
        };

#ifdef CDS_MOVE_SEMANTICS_SUPPORT
        // Move policy for map
        template <typename Key, typename T, typename Traits, typename Alloc>
        struct move_item_policy< stdext::hash_map< Key, T, Traits, Alloc > >
        {
            typedef stdext::hash_map< Key, T, Traits, Alloc > map_type ;
            typedef typename map_type::value_type pair_type ;
            typedef typename map_type::iterator iterator ;

            void operator()( map_type& map, iterator itWhat )
            {
                map.insert( std::move( *itWhat ) )    ;
            }
        };
#endif
    }   // namespace striped_set
}} // namespace cds::container

namespace cds { namespace intrusive { namespace striped_set {

    /// stdext::hash_map adapter for hash map bucket
    template <typename Key, typename T, class Traits, class Alloc, CDS_SPEC_OPTIONS>
    class adapt< stdext::hash_map< Key, T, Traits, Alloc>, CDS_OPTIONS >
    {
    public:
        typedef stdext::hash_map< Key, T, Traits, Alloc>  container_type  ;   ///< underlying container type

    private:
        /// Adapted container type
        class adapted_container: public cds::container::striped_set::adapted_container
        {
        public:
            typedef typename container_type::value_type value_type  ;   ///< value type stored in the container
            typedef typename container_type::key_type   key_type    ;
            typedef typename container_type::mapped_type    mapped_type ;
            typedef typename container_type::iterator      iterator ;   ///< container iterator
            typedef typename container_type::const_iterator const_iterator ;    ///< container const iterator

            static bool const has_find_with = false     ;
            static bool const has_erase_with = false    ;

        private:
            //@cond
            typedef typename cds::opt::select<
                typename cds::opt::value<
                    typename cds::opt::find_option<
                        cds::opt::copy_policy< cds::container::striped_set::move_item >
                        , CDS_OPTIONS
                    >::type
                >::copy_policy
                , cds::container::striped_set::copy_item, cds::container::striped_set::copy_item_policy<container_type>
                , cds::container::striped_set::swap_item, cds::container::striped_set::swap_item_policy<container_type>
#ifdef CDS_MOVE_SEMANTICS_SUPPORT
                , cds::container::striped_set::move_item, cds::container::striped_set::move_item_policy<container_type>
#endif
            >::type copy_item   ;
            //@endcond

        private:
            //@cond
            container_type  m_Map  ;
            //@endcond

        public:
            template <typename Q, typename Func>
            bool insert( const Q& key, Func f )
            {
                std::pair<iterator, bool> res = m_Map.insert( value_type( key, mapped_type() ) ) ;
                if ( res.second )
                    ::cds::unref(f)( *res.first )  ;
                return res.second   ;
            }

            template <typename Q, typename Func>
            std::pair<bool, bool> ensure( const Q& val, Func func )
            {
                std::pair<iterator, bool> res = m_Map.insert( value_type( val, mapped_type() )) ;
                ::cds::unref(func)( res.second, *res.first )     ;
                return std::make_pair( true, res.second )   ;
            }

            template <typename Q, typename Func>
            bool erase( const Q& key, Func f )
            {
                iterator it = m_Map.find( key_type(key) ) ;
                if ( it == m_Map.end() )
                    return false ;
                ::cds::unref(f)( *it )  ;
                m_Map.erase( it )       ;
                return true ;
            }

            template <typename Q, typename Func>
            bool find( Q& val, Func f )
            {
                iterator it = m_Map.find( key_type(val) ) ;
                if ( it == m_Map.end() )
                    return false ;
                ::cds::unref(f)( *it, val ) ;
                return true ;
            }

            void clear()
            {
                m_Map.clear()    ;
            }

            iterator begin()                { return m_Map.begin(); }
            const_iterator begin() const    { return m_Map.begin(); }
            iterator end()                  { return m_Map.end(); }
            const_iterator end() const      { return m_Map.end(); }

            void move_item( adapted_container& /*from*/, iterator itWhat )
            {
                assert( m_Map.find( itWhat->first ) == m_Map.end() ) ;
                copy_item()( m_Map, itWhat )   ;
            }

            size_t size() const
            {
                return m_Map.size() ;
            }
        };

    public:
        typedef adapted_container type ; ///< Result of \p adapt metafunction

    };
}}} // namespace cds::intrusive::striped_set


//@endcond

#endif  // #ifndef __CDS_CONTAINER_STRIPED_MAP_STD_HASH_MAP_MSVC_ADAPTER_H
