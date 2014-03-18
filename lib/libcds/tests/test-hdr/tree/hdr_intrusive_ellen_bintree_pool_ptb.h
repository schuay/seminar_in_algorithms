//$$CDS-header$$

#ifndef CDSHDRTEST_INTRUSIVE_ELLEN_BINTREE_POOL_PTB_H
#define CDSHDRTEST_INTRUSIVE_ELLEN_BINTREE_POOL_PTB_H

#include "tree/hdr_intrusive_bintree.h"
#include <cds/intrusive/ellen_bintree_ptb.h>

#include <cds/memory/vyukov_queue_pool.h>
#include <cds/memory/pool_allocator.h>

namespace tree {

    namespace ellen_bintree_ptb {
        typedef cds::intrusive::ellen_bintree::node_types<cds::gc::PTB, IntrusiveBinTreeHdrTest::key_type>   node_types  ;
        typedef node_types::leaf_node_type                                  leaf_node       ;
        typedef IntrusiveBinTreeHdrTest::base_hook_value< leaf_node >       base_value      ;
        typedef node_types::internal_node_type                              internal_node   ;
        typedef node_types::update_desc_type                                update_desc     ;


        // Internal node pool based on Vyukov's queue
        typedef cds::memory::lazy_vyukov_queue_pool<
            internal_node,
            cds::opt::buffer< cds::opt::v::dynamic_buffer< cds::any_type > >
        > internal_node_pool_type   ;

        extern internal_node_pool_type s_InternalNodePool ;

        struct internal_node_pool_accessor {
            typedef internal_node_pool_type::value_type     value_type  ;

            internal_node_pool_type& operator()() const
            {
                return s_InternalNodePool ;
            }
        };

        // Update descriptor pool based on Vyukov's queue
        typedef cds::memory::vyukov_queue_pool<
            update_desc,
            cds::opt::buffer< cds::opt::v::static_buffer< cds::any_type, 16 > >
        > update_desc_pool_type   ;

        extern update_desc_pool_type s_UpdateDescPool ;

        struct update_desc_pool_accessor {
            typedef update_desc_pool_type::value_type     value_type  ;

            update_desc_pool_type& operator()() const
            {
                return s_UpdateDescPool ;
            }
        };

    } // namespace ellen_bintree_ptb

} // namespace tree

#endif // #ifndef CDSHDRTEST_INTRUSIVE_ELLEN_BINTREE_POOL_PTB_H
