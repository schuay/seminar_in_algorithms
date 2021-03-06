//$$CDS-header$$

#ifndef __CDS_CONTAINER_STRIPED_SET_STD_VECTOR_ADAPTER_H
#define __CDS_CONTAINER_STRIPED_SET_STD_VECTOR_ADAPTER_H

#include <cds/container/striped_set/adapter.h>     // lower_bound
#include <cds/ref.h>
#include <vector>
#include <algorithm>    // std::lower_bound
#include <utility>      // std::pair

//@cond
namespace cds { namespace container {
    namespace striped_set {

        // Copy policy for std::vector
        template <typename T, typename Alloc>
        struct copy_item_policy< std::vector< T, Alloc > >
        {
            typedef std::vector< T, Alloc > vector_type ;
            typedef typename vector_type::iterator iterator ;

            void operator()( vector_type& vec, iterator itInsert, iterator itWhat )
            {
                vec.insert( itInsert, *itWhat );
            }
        };

        // Swap policy for std::vector
        template <typename T, typename Alloc>
        struct swap_item_policy< std::vector< T, Alloc > >
        {
            typedef std::vector< T, Alloc > vector_type ;
            typedef typename vector_type::iterator iterator ;

            void operator()( vector_type& vec, iterator itInsert, iterator itWhat )
            {
                typename vector_type::value_type newVal    ;
                itInsert = vec.insert( itInsert, newVal );
                std::swap( *itInsert, *itWhat )   ;
            }
        };

#ifdef CDS_MOVE_SEMANTICS_SUPPORT
        // Move policy for std::vector
        template <typename T, typename Alloc>
        struct move_item_policy< std::vector< T, Alloc > >
        {
            typedef std::vector< T, Alloc > vector_type ;
            typedef typename vector_type::iterator iterator ;

            void operator()( vector_type& vec, iterator itInsert, iterator itWhat )
            {
                vec.insert( itInsert, std::move( *itWhat ))    ;
            }
        };
#endif

    }   // namespace striped_set
}} // namespace cds::container

namespace cds { namespace intrusive { namespace striped_set {

    /// std::vector adapter for hash set bucket
    template <typename T, class Alloc, CDS_SPEC_OPTIONS>
    class adapt< std::vector<T, Alloc>, CDS_OPTIONS >
    {
    public:
        typedef std::vector<T, Alloc>     container_type          ;   ///< underlying container type

    private:
        /// Adapted container type
        class adapted_container: public cds::container::striped_set::adapted_sequential_container
        {
        public:
            typedef typename container_type::value_type value_type  ;   ///< value type stored in the container
            typedef typename container_type::iterator      iterator ;   ///< container iterator
            typedef typename container_type::const_iterator const_iterator ;    ///< container const iterator

            static bool const has_find_with = true     ;
            static bool const has_erase_with = true    ;

        private:
            //@cond
            typedef typename cds::opt::details::make_comparator_from_option_list< value_type, CDS_OPTIONS >::type key_comparator  ;

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

            struct find_predicate
            {
                bool operator()( value_type const& i1, value_type const& i2) const
                {
                    return key_comparator()( i1, i2 ) < 0 ;
                }

                template <typename Q>
                bool operator()( Q const& i1, value_type const& i2) const
                {
                    return key_comparator()( i1, i2 ) < 0 ;
                }

                template <typename Q>
                bool operator()( value_type const& i1, Q const& i2) const
                {
                    return key_comparator()( i1, i2 ) < 0 ;
                }
            };
            //@endcond

        private:
            //@cond
            container_type  m_Vector  ;
            //@endcond

        public:

            template <typename Q, typename Func>
            bool insert( const Q& val, Func f )
            {
                iterator it = std::lower_bound( m_Vector.begin(), m_Vector.end(), val, find_predicate() ) ;
                if ( it == m_Vector.end() || key_comparator()( val, *it ) != 0 ) {
                    value_type newItem( val )   ;
                    it = m_Vector.insert( it, newItem )   ;
                    cds::unref( f )( *it )      ;
                    return true ;
                }
                return false    ;
            }

#           ifdef CDS_EMPLACE_SUPPORT
            template <typename... Args>
            bool emplace( Args&&... args )
            {
#if CDS_COMPILER == CDS_COMPILER_MSVC && CDS_COMPILER_VERSION == CDS_COMPILER_MSVC12
                // MS VC++ 2013 internal compiler error
                // Use assignment workaround, see http://connect.microsoft.com/VisualStudio/feedback/details/804941/visual-studio-2013-rc-c-internal-compiler-error-with-std-forward
                value_type val = value_type(std::forward<Args>(args)...);
#else
                value_type val( std::forward<Args>(args)... )   ;
#endif
                iterator it = std::lower_bound( m_Vector.begin(), m_Vector.end(), val, find_predicate() ) ;
                if ( it == m_Vector.end() || key_comparator()( val, *it ) != 0 ) {
                    it = m_Vector.emplace( it, std::move( val ) )   ;
                    return true     ;
                }
                return false ;
            }
#           endif

            template <typename Q, typename Func>
            std::pair<bool, bool> ensure( const Q& val, Func func )
            {
                iterator it = std::lower_bound( m_Vector.begin(), m_Vector.end(), val, find_predicate() ) ;
                if ( it == m_Vector.end() || key_comparator()( val, *it ) != 0 ) {
                    // insert new
                    value_type newItem( val )               ;
                    it = m_Vector.insert( it, newItem )     ;
                    cds::unref( func )( true, *it, val )    ;
                    return std::make_pair( true, true )     ;
                }
                else {
                    // already exists
                    cds::unref( func )( false, *it, val )   ;
                    return std::make_pair( true, false )    ;
                }
            }

            template <typename Q, typename Func>
            bool erase( const Q& key, Func f )
            {
                iterator it = std::lower_bound( m_Vector.begin(), m_Vector.end(), key, find_predicate() ) ;
                if ( it == m_Vector.end() || key_comparator()( key, *it ) != 0 )
                    return false ;

                // key exists
                cds::unref( f )( *it )  ;
                m_Vector.erase( it )    ;
                return true     ;
            }

            template <typename Q, typename Less, typename Func>
            bool erase( const Q& key, Less pred, Func f )
            {
                iterator it = std::lower_bound( m_Vector.begin(), m_Vector.end(), key, pred ) ;
                if ( it == m_Vector.end() || pred( key, *it ) || pred( *it, key ) )
                    return false ;

                // key exists
                cds::unref( f )( *it )  ;
                m_Vector.erase( it )    ;
                return true     ;
            }

            template <typename Q, typename Func>
            bool find( Q& val, Func f )
            {
                iterator it = std::lower_bound( m_Vector.begin(), m_Vector.end(), val, find_predicate() ) ;
                if ( it == m_Vector.end() || key_comparator()( val, *it ) != 0 )
                    return false ;

                // key exists
                cds::unref( f )( *it, val )  ;
                return true     ;
            }

            template <typename Q, typename Less, typename Func>
            bool find( Q& val, Less pred, Func f )
            {
                iterator it = std::lower_bound( m_Vector.begin(), m_Vector.end(), val, pred ) ;
                if ( it == m_Vector.end() || pred( val, *it ) || pred( *it, val ) )
                    return false ;

                // key exists
                cds::unref( f )( *it, val )  ;
                return true     ;
            }


            void clear()
            {
                m_Vector.clear()    ;
            }

            iterator begin()                { return m_Vector.begin(); }
            const_iterator begin() const    { return m_Vector.begin(); }
            iterator end()                  { return m_Vector.end(); }
            const_iterator end() const      { return m_Vector.end(); }

            void move_item( adapted_container& /*from*/, iterator itWhat )
            {
                iterator it = std::lower_bound( m_Vector.begin(), m_Vector.end(), *itWhat, find_predicate() ) ;
                assert( it == m_Vector.end() || key_comparator()( *itWhat, *it ) != 0 ) ;

                copy_item()( m_Vector, it, itWhat )   ;
            }

            size_t size() const
            {
                return m_Vector.size() ;
            }
        };

    public:
        typedef adapted_container type ; ///< Result of \p adapt metafunction

    };
}}} // namespace cds::intrusive::striped_set

//@endcond

#endif // #ifndef __CDS_CONTAINER_STRIPED_SET_STD_VECTOR_ADAPTER_H
