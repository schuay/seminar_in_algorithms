//$$CDS-header$$

#ifndef __CDS_CONTAINER_DETAILS_MAKE_LAZY_LIST_H
#define __CDS_CONTAINER_DETAILS_MAKE_LAZY_LIST_H

#include <cds/details/binary_functor_wrapper.h>

namespace cds { namespace container {

    //@cond
    namespace details {

        template <class GC, typename T, class Traits>
        struct make_lazy_list
        {
            typedef GC      gc          ;
            typedef T       value_type  ;
            typedef Traits original_type_traits ;

            struct node_type : public intrusive::lazy_list::node<gc, typename original_type_traits::lock_type>
            {
                value_type  m_Value     ;

                node_type()
                {}

                template <typename Q>
                node_type( Q const& v )
                    : m_Value(v)
                {}
#       ifdef CDS_EMPLACE_SUPPORT
                template <typename... Args>
                node_type( Args&&... args )
                    : m_Value( std::forward<Args>(args)...)
                {}
#       endif
            };

            typedef typename original_type_traits::allocator::template rebind<node_type>::other  allocator_type  ;
            typedef cds::details::Allocator< node_type, allocator_type >                cxx_allocator   ;

            struct node_deallocator
            {
                void operator ()( node_type * pNode )
                {
                    cxx_allocator().Delete( pNode ) ;
                }
            }   ;

            typedef typename opt::details::make_comparator< value_type, original_type_traits >::type key_comparator ;

            struct value_accessor {
                value_type const & operator()( node_type const & node ) const
                {
                    return node.m_Value ;
                }
            };

            template <typename Less>
            struct less_wrapper {
                typedef cds::details::compare_wrapper< node_type, cds::opt::details::make_comparator_from_less<Less>, value_accessor > type ;
            };

            struct type_traits: public original_type_traits
            {
                typedef intrusive::lazy_list::base_hook< opt::gc<gc> >  hook        ;
                typedef node_deallocator               disposer                     ;

                typedef cds::details::compare_wrapper< node_type, key_comparator, value_accessor > compare ;
            };

            typedef intrusive::LazyList<gc, node_type, type_traits>  type       ;
        };
    }   // namespace details
    //@endcond

}}  // namespace cds::container

#endif  // #ifndef __CDS_CONTAINER_DETAILS_MAKE_MICHAEL_LIST_H
