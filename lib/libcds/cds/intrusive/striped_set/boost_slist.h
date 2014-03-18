//$$CDS-header$$

#ifndef __CDS_INTRUSIVE_STRIPED_SET_BOOST_SLIST_ADAPTER_H
#define __CDS_INTRUSIVE_STRIPED_SET_BOOST_SLIST_ADAPTER_H

#include <boost/intrusive/slist.hpp>
#include <cds/intrusive/striped_set/adapter.h>

//@cond
namespace cds { namespace intrusive { namespace striped_set {

    template <typename T, CDS_BOOST_INTRUSIVE_DECL_OPTIONS5, CDS_SPEC_OPTIONS>
    class adapt< boost::intrusive::slist< T, CDS_BOOST_INTRUSIVE_OPTIONS5 >, CDS_OPTIONS >
    {
    public:
        typedef boost::intrusive::slist< T, CDS_BOOST_INTRUSIVE_OPTIONS5 >  container_type  ;   ///< underlying intrusive container type

    private:
        /// Adapted intrusive container
        class adapted_container: public cds::intrusive::striped_set::adapted_sequential_container
        {
        public:
            typedef typename container_type::value_type     value_type  ;   ///< value type stored in the container
            typedef typename container_type::iterator       iterator ;   ///< container iterator
            typedef typename container_type::const_iterator const_iterator ;    ///< container const iterator

            typedef typename cds::opt::details::make_comparator_from_option_list< value_type, CDS_OPTIONS >::type key_comparator  ;

        private:

            template <typename Q, typename Less>
            std::pair< iterator, bool > find_prev_item( Q const& key, Less pred )
            {
                iterator itPrev = m_List.before_begin() ;
                iterator itEnd = m_List.end() ;
                for ( iterator it = m_List.begin(); it != itEnd; ++it ) {
                    if ( pred( key, *it ) )
                        itPrev = it ;
                    else if ( pred( *it, key ) )
                        break;
                    else
                        return std::make_pair( itPrev, true ) ;
                }
                return std::make_pair( itPrev, false )   ;
            }

            template <typename Q>
            std::pair< iterator, bool > find_prev_item( Q const& key )
            {
                return find_prev_item_cmp( key, key_comparator() ) ;
            }

            template <typename Q, typename Compare>
            std::pair< iterator, bool > find_prev_item_cmp( Q const& key, Compare cmp )
            {
                iterator itPrev = m_List.before_begin() ;
                iterator itEnd = m_List.end() ;
                for ( iterator it = m_List.begin(); it != itEnd; ++it ) {
                    int nCmp = cmp( key, *it ) ;
                    if ( nCmp < 0 )
                        itPrev = it ;
                    else if ( nCmp > 0 )
                        break;
                    else
                        return std::make_pair( itPrev, true ) ;
                }
                return std::make_pair( itPrev, false )   ;
            }

            template <typename Q, typename Compare, typename Func>
            value_type * erase_( Q const& key, Compare cmp, Func f )
            {
                std::pair< iterator, bool > pos = find_prev_item_cmp( key, cmp )    ;
                if ( !pos.second )
                    return null_ptr<value_type *>() ;

                // key exists
                iterator it = pos.first ;
                value_type& val = *(++it) ;
                cds::unref( f )( val )  ;
                m_List.erase_after( pos.first )      ;

                return &val ;
            }


#       ifndef CDS_CXX11_LAMBDA_SUPPORT
            struct empty_insert_functor {
                void operator()( value_type& )
                {}
            };
#       endif

        private:
            container_type  m_List  ;

        public:
            adapted_container()
            {}

            container_type& base_container()
            {
                return m_List ;
            }

            template <typename Func>
            bool insert( value_type& val, Func f )
            {
                std::pair< iterator, bool > pos = find_prev_item( val )    ;
                if ( !pos.second ) {
                    m_List.insert_after( pos.first, val )   ;
                    cds::unref( f )( val )      ;
                    return true     ;
                }

                // key already exists
                return false    ;
            }

            template <typename Func>
            std::pair<bool, bool> ensure( value_type& val, Func f )
            {
                std::pair< iterator, bool > pos = find_prev_item( val )    ;
                if ( !pos.second ) {
                    // insert new
                    m_List.insert_after( pos.first, val )       ;
                    cds::unref( f )( true, val, val )    ;
                    return std::make_pair( true, true )     ;
                }
                else {
                    // already exists
                    cds::unref( f )( false, *(++pos.first), val )   ;
                    return std::make_pair( true, false )    ;
                }
            }

            bool unlink( value_type& val )
            {
                std::pair< iterator, bool > pos = find_prev_item( val )    ;
                if ( !pos.second )
                    return false ;

                ++pos.first ;
                if ( &(*pos.first) != &val )
                    return false ;

                m_List.erase( pos.first ) ;
                return true ;
            }

            template <typename Q, typename Func>
            value_type * erase( Q const& key, Func f )
            {
                return erase_( key, key_comparator(), f ) ;
            }

            template <typename Q, typename Less, typename Func>
            value_type * erase( Q const& key, Less pred, Func f )
            {
                return erase_( key, cds::opt::details::make_comparator_from_less<Less>(), f ) ;
            }

            template <typename Q, typename Func>
            bool find( Q& key, Func f )
            {
                std::pair< iterator, bool > pos = find_prev_item( key )    ;
                if ( !pos.second )
                    return false ;

                // key exists
                cds::unref( f )( *(++pos.first), key )  ;
                return true     ;
            }

            template <typename Q, typename Less, typename Func>
            bool find( Q& key, Less pred, Func f )
            {
                std::pair< iterator, bool > pos = find_prev_item( key, pred )    ;
                if ( !pos.second )
                    return false ;

                // key exists
                cds::unref( f )( *(++pos.first), key )  ;
                return true     ;
            }

            void clear()
            {
                m_List.clear()    ;
            }

            template <typename Disposer>
            void clear( Disposer disposer )
            {
                m_List.clear_and_dispose( disposer )    ;
            }

            iterator begin()                { return m_List.begin(); }
            const_iterator begin() const    { return m_List.begin(); }
            iterator end()                  { return m_List.end(); }
            const_iterator end() const      { return m_List.end(); }

            size_t size() const
            {
                return (size_t) m_List.size() ;
            }

            void move_item( adapted_container& from, iterator itWhat )
            {
                value_type& val = *itWhat ;
                from.base_container().erase( itWhat )    ;
#           ifdef CDS_CXX11_LAMBDA_SUPPORT
                insert( val, []( value_type& ) {} )    ;
#           else
                insert( val, empty_insert_functor() )    ;
#           endif
            }

        };
    public:
        typedef adapted_container   type ;  ///< Result of the metafunction
    };
}}} // namespace cds::intrusive::striped_set
//@endcond

#endif // #ifndef __CDS_INTRUSIVE_STRIPED_SET_BOOST_SLIST_ADAPTER_H
