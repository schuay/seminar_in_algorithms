//$$CDS-header$$

#ifndef __CDS_CONTAINER_DETAILS_MAKE_SPLIT_LIST_SET_H
#define __CDS_CONTAINER_DETAILS_MAKE_SPLIT_LIST_SET_H

#include <cds/container/split_list_base.h>
#include <cds/details/allocator.h>
#include <cds/details/binary_functor_wrapper.h>

//@cond
namespace cds { namespace container {

    // Forward declaration
    struct michael_list_tag ;
    struct lazy_list_tag    ;

    namespace details {

#ifdef __CDS_CONTAINER_MICHAEL_LIST_BASE_H
        // if michael_list included

        template <typename GC, typename T, typename Traits>
        struct make_split_list_set< GC, T, michael_list_tag, Traits >
        {
            typedef GC      gc          ;
            typedef T       value_type  ;
            typedef Traits  original_type_traits ;

            typedef typename cds::opt::select_default<
                typename original_type_traits::ordered_list_traits,
                cds::container::michael_list::type_traits
            >::type         original_ordered_list_traits  ;

            typedef cds::intrusive::split_list::node< cds::intrusive::michael_list::node<gc> > primary_node_type ;
            struct node_type: public primary_node_type
            {
                value_type  m_Value ;

                template <typename Q>
                explicit node_type( Q const& v )
                    : m_Value(v)
                {}
#       ifdef CDS_EMPLACE_SUPPORT
                template <typename Q, typename... Args>
                explicit node_type( Q&& q, Args&&... args )
                    : m_Value( std::forward<Q>(q), std::forward<Args>(args)... )
                {}
#       endif
            private:
                node_type() ;   // no default ctor
            };

            typedef typename cds::opt::select_default<
                typename original_type_traits::ordered_list_traits,
                typename original_type_traits::allocator,
                typename cds::opt::select_default<
                    typename original_type_traits::ordered_list_traits::allocator,
                    typename original_type_traits::allocator
                >::type
            >::type node_allocator_  ;

            typedef typename node_allocator_::template rebind<node_type>::other node_allocator_type ;

            typedef cds::details::Allocator< node_type, node_allocator_type >   cxx_node_allocator   ;
            struct node_deallocator
            {
                void operator ()( node_type * pNode )
                {
                    cxx_node_allocator().Delete( pNode ) ;
                }
            }   ;

            typedef typename opt::details::make_comparator< value_type, original_ordered_list_traits >::type key_comparator ;

            typedef typename original_type_traits::key_accessor key_accessor ;

            struct value_accessor
            {
                typename key_accessor::key_type const& operator()( node_type const& node ) const
                {
                    return key_accessor()(node.m_Value) ;
                }
            };

            template <typename Predicate>
            struct predicate_wrapper {
                typedef cds::details::predicate_wrapper< node_type, Predicate, value_accessor > type ;
            };

            struct ordered_list_traits: public original_ordered_list_traits
            {
                typedef cds::intrusive::michael_list::base_hook<
                    opt::gc<gc>
                >   hook        ;
                typedef atomicity::empty_item_counter   item_counter;
                typedef node_deallocator                disposer    ;
                typedef cds::details::compare_wrapper< node_type, key_comparator, value_accessor > compare ;
            };

            struct type_traits: public original_type_traits
            {
                struct hash: public original_type_traits::hash
                {
                    typedef typename original_type_traits::hash  base_class ;

                    size_t operator()(node_type const& v ) const
                    {
                        return base_class::operator()( key_accessor()( v.m_Value ) )  ;
                    }
                    template <typename Q>
                    size_t operator()( Q const& k ) const
                    {
                        return base_class::operator()( k ) ;
                    }
                    //using base_class::operator() ;
                };
            };

            typedef cds::intrusive::MichaelList< gc, node_type, ordered_list_traits >   ordered_list;
            typedef cds::intrusive::SplitListSet< gc, ordered_list, type_traits >       type        ;
        };
#endif  // ifdef __CDS_CONTAINER_MICHAEL_LIST_BASE_H

#ifdef __CDS_CONTAINER_LAZY_LIST_BASE_H
        // if lazy_list included
        template <typename GC, typename T, typename Traits>
        struct make_split_list_set< GC, T, lazy_list_tag, Traits >
        {
            typedef GC      gc          ;
            typedef T       value_type  ;
            typedef Traits  original_type_traits ;

            typedef typename cds::opt::select_default<
                typename original_type_traits::ordered_list_traits,
                cds::container::lazy_list::type_traits
            >::type         original_ordered_list_traits  ;

            typedef typename cds::opt::select_default<
                typename original_ordered_list_traits::lock_type,
                typename cds::container::lazy_list::type_traits::lock_type
            >::type   lock_type   ;

            typedef cds::intrusive::split_list::node< cds::intrusive::lazy_list::node<gc, lock_type > > primary_node_type ;
            struct node_type: public primary_node_type
            {
                value_type  m_Value ;

                template <typename Q>
                explicit node_type( const Q& v )
                    : m_Value(v)
                {}

#       ifdef CDS_EMPLACE_SUPPORT
                template <typename Q, typename... Args>
                explicit node_type( Q&& q, Args&&... args )
                    : m_Value( std::forward<Q>(q), std::forward<Args>(args)... )
                {}
#       endif
            private:
                node_type() ;   // no default ctor
            };

            typedef typename cds::opt::select_default<
                typename original_type_traits::ordered_list_traits,
                typename original_type_traits::allocator,
                typename cds::opt::select_default<
                    typename original_type_traits::ordered_list_traits::allocator,
                    typename original_type_traits::allocator
                >::type
            >::type node_allocator_  ;

            typedef typename node_allocator_::template rebind<node_type>::other node_allocator_type ;

            typedef cds::details::Allocator< node_type, node_allocator_type >   cxx_node_allocator   ;
            struct node_deallocator
            {
                void operator ()( node_type * pNode )
                {
                    cxx_node_allocator().Delete( pNode ) ;
                }
            }   ;

            typedef typename opt::details::make_comparator< value_type, original_ordered_list_traits >::type key_comparator ;

            typedef typename original_type_traits::key_accessor key_accessor ;

            struct value_accessor
            {
                typename key_accessor::key_type const & operator()( node_type const & node ) const
                {
                    return key_accessor()(node.m_Value) ;
                }
            };

            template <typename Predicate>
            struct predicate_wrapper {
                typedef cds::details::predicate_wrapper< node_type, Predicate, value_accessor > type ;
            };

            struct ordered_list_traits: public original_ordered_list_traits
            {
                typedef cds::intrusive::lazy_list::base_hook<
                    opt::gc<gc>
                    ,opt::lock_type< lock_type >
                >  hook        ;
                typedef atomicity::empty_item_counter   item_counter;
                typedef node_deallocator                disposer    ;
                typedef cds::details::compare_wrapper< node_type, key_comparator, value_accessor > compare ;
            };

            struct type_traits: public original_type_traits
            {
                struct hash: public original_type_traits::hash
                {
                    typedef typename original_type_traits::hash  base_class ;

                    size_t operator()(node_type const& v ) const
                    {
                        return base_class::operator()( key_accessor()( v.m_Value ))  ;
                    }
                    template <typename Q>
                    size_t operator()( Q const& k ) const
                    {
                        return base_class::operator()( k ) ;
                    }
                    //using base_class::operator()    ;
                };
            };

            typedef cds::intrusive::LazyList< gc, node_type, ordered_list_traits >  ordered_list;
            typedef cds::intrusive::SplitListSet< gc, ordered_list, type_traits >   type        ;
        };
#endif  // ifdef __CDS_CONTAINER_LAZY_LIST_BASE_H

    }   // namespace details
}}  // namespace cds::container
//@endcond

#endif // #ifndef __CDS_CONTAINER_DETAILS_MAKE_SPLIT_LIST_SET_H
