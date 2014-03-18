//$$CDS-header$$

#ifndef __CDS_INTRUSIVE_DETAILS_ELLEN_BINTREE_BASE_H
#define __CDS_INTRUSIVE_DETAILS_ELLEN_BINTREE_BASE_H

#include <cds/intrusive/base.h>
#include <cds/opt/options.h>
#include <cds/urcu/options.h>
#include <cds/details/std/type_traits.h>
#include <cds/details/marked_ptr.h>
#include <cds/details/allocator.h>

namespace cds { namespace intrusive {

    /// EllenBinTree related declarations
    namespace ellen_bintree {

        //Forwards
        template <class GC> struct base_node ;
        template <class GC, typename Tag = opt::none> struct node ;
        template <class GC, typename Key> struct internal_node  ;

        /// Update descriptor
        /**
            Update descriptor is used internally for helping concurrent threads
            to complete modifying operation.
            Usually, you should not use \p update_desc type directly until
            you want to develop special free-list of update descriptor.

            Template parameters:
            - \p LeafNode - leaf node type, see \ref node
            - \p InternalNode - internal node type, see \ref internal_node

            @note Size of update descriptor is constant.
            It does not depends of template arguments.
        */
        template <typename LeafNode, typename InternalNode>
        struct update_desc {
            //@cond
            typedef LeafNode        leaf_node       ;
            typedef InternalNode    internal_node   ;

            typedef cds::details::marked_ptr< update_desc, 3 > update_ptr ;

            enum {
                Clean = 0,
                DFlag = 1,
                IFlag = 2,
                Mark  = 3
            };

            static CDS_CONSTEXPR update_desc * locked_desc()
            {
                return reinterpret_cast<update_desc *>( ~uintptr_t(0) ) ;
            }

            struct insert_info {
                internal_node *    pParent ;
                internal_node *    pNew    ;
                leaf_node *        pLeaf   ;
                bool               bRightLeaf ;
            } ;
            struct delete_info {
                internal_node *    pGrandParent    ;
                internal_node *    pParent         ;
                leaf_node *        pLeaf           ;
                bool               bDisposeLeaf    ; // true if pLeaf should be disposed, false otherwise (for extract operation)
                bool               bRightParent    ;
                bool               bRightLeaf      ;
            } ;

            union {
                insert_info     iInfo   ;
                delete_info     dInfo   ;
            };

            update_ptr      pUpdate         ;   // for dInfo
            update_desc *   pNextRetire     ;   // for local retired list

            update_desc()
                : pNextRetire( null_ptr<update_desc *>() )
            {}
            //@endcond
        };

        //@cond
        template <class GC>
        struct base_node
        {
            typedef GC              gc       ;   ///< Garbage collector

            enum flags {
                internal        = 1,    ///< set for internal node
                key_infinite1   = 2,    ///< set if node's key is Inf1
                key_infinite2   = 4,    ///< set if node's key is Inf2

                key_infinite = key_infinite1 | key_infinite2    ///< Cumulative infinite flags
            };

            unsigned int    m_nFlags    ;   ///< Internal flags
            base_node *     m_pNextRetired  ;   ///< Retired node chain

            /// Constructs leaf (bIntrenal == false) or internal (bInternal == true) node
            explicit base_node( bool bInternal )
                : m_nFlags( bInternal ? internal : 0 )
                , m_pNextRetired( null_ptr<base_node *>())
            {}

            /// Checks if the node is a leaf
            bool is_leaf() const
            {
                return !is_internal()   ;
            }

            /// Checks if the node is internal
            bool is_internal() const
            {
                return (m_nFlags & internal) != 0 ;
            }

            /// Returns infinite key, 0 if the node is not infinite
            unsigned int infinite_key() const
            {
                return m_nFlags & key_infinite ;
            }

            /// Sets infinite key for the node (for internal use only!!!)
            void infinite_key( int nInf )
            {
                m_nFlags &= ~key_infinite ;
                switch ( nInf ) {
                    case 1:
                        m_nFlags |= key_infinite1;
                        break;
                    case 2:
                        m_nFlags |= key_infinite2;
                        break;
                    case 0:
                        m_nFlags &= ~(key_infinite1|key_infinite2) ;
                        break;
                    default:
                        assert( false ) ;
                        break;
                }
            }
        };
        //@endcond

        /// Ellen's binary tree leaf node
        /**
            Template parameters:
            - \p GC - one of \ref cds_garbage_collector "garbage collector type"
            - \p Tag - a tag used to distinguish between different implementation. An incomplete type may be used as a tag.
        */
        template <typename GC,
#   ifdef CDS_DOXYGEN_INVOKED
            typename Tag = opt::none
#   else
            typename Tag
#   endif
        >
        struct node
#   ifndef CDS_DOXYGEN_INVOKED
            : public base_node< GC >
#   endif
        {
            //@cond
            typedef base_node< GC >   base_class ;
            //@endcond

            typedef GC              gc       ;   ///< Garbage collector type
            typedef Tag             tag      ;   ///< Tag

            /// Default ctor
            node()
                : base_class( false )
            {}
        };

        /// Ellen's binary tree internal node
        /**
            Template arguments:
            - \p Key - key type
            - \p LeafNode - leaf node type
        */
        template <typename Key, typename LeafNode>
        struct internal_node
#   ifndef CDS_DOXYGEN_INVOKED
            : public base_node<typename LeafNode::gc>
#   endif
        {
            //@cond
            typedef base_node<typename LeafNode::gc>   base_class  ;
            //@endcond

            typedef Key         key_type    ;   ///< key type
            typedef LeafNode    leaf_node   ;   ///< type of leaf node
            typedef typename update_desc< leaf_node, internal_node >::update_ptr  update_ptr ; ///< Marked pointer to update descriptor

            key_type                            m_Key       ;   ///< Regular key
            CDS_ATOMIC::atomic<base_class *>    m_pLeft     ;   ///< Left subtree
            CDS_ATOMIC::atomic<base_class *>    m_pRight    ;   ///< Right subtree
            CDS_ATOMIC::atomic<update_ptr>      m_pUpdate   ;   ///< Update descriptor

            /// Default ctor
            internal_node()
                : base_class( true )
                , m_pLeft( null_ptr<base_class *>() )
                , m_pRight( null_ptr<base_class *>() )
                , m_pUpdate( update_ptr() )
            {}

            //@cond
            /*
            void clean()
            {
                m_pLeft.store( null_ptr<base_class *>(), CDS_ATOMIC::memory_order_relaxed ) ;
                m_pRight.store( null_ptr<base_class *>(), CDS_ATOMIC::memory_order_relaxed ) ;
                m_pUpdate.store( update_ptr(), CDS_ATOMIC::memory_order_release ) ;
                base_class::infinite_key(0) ;
            }
            */
            //@endcond
        };

        /// Types of EllenBinTree node
        /**
            This struct declares different \p %EllenBinTree node types.
            It can be useful for simplifying \p %EllenBinTree node declaration in your application.
        */
        template <typename GC, typename Key, typename Tag = opt::none>
        struct node_types
        {
            typedef node<GC, Tag>                       leaf_node_type      ;   ///< Leaf node type
            typedef internal_node<Key, leaf_node_type>  internal_node_type  ;   ///< Internal node type
            typedef update_desc<leaf_node_type, internal_node_type> update_desc_type ;  ///< Update descriptor type
        };

        //@cond
        struct undefined_gc ;
        struct default_hook {
            typedef undefined_gc    gc  ;
            typedef opt::none       tag ;
        };
        //@endcond

        //@cond
        template < typename HookType, CDS_DECL_OPTIONS2>
        struct hook
        {
            typedef typename opt::make_options< default_hook, CDS_OPTIONS2>::type  options ;
            typedef typename options::gc    gc  ;
            typedef typename options::tag   tag ;
            typedef node<gc, tag>           node_type   ;
            typedef HookType                hook_type   ;
        };
        //@endcond

        /// Base hook
        /**
            \p Options are:
            - opt::gc - garbage collector used.
            - opt::tag - a tag
        */
        template < CDS_DECL_OPTIONS2 >
        struct base_hook: public hook< opt::base_hook_tag, CDS_OPTIONS2 >
        {};

        /// Member hook
        /**
            \p MemberOffset defines offset in bytes of \ref node member into your structure.
            Use \p offsetof macro to define \p MemberOffset

            \p Options are:
            - opt::gc - garbage collector used.
            - opt::tag - a tag
        */
        template < size_t MemberOffset, CDS_DECL_OPTIONS2 >
        struct member_hook: public hook< opt::member_hook_tag, CDS_OPTIONS2 >
        {
            //@cond
            static const size_t c_nMemberOffset = MemberOffset ;
            //@endcond
        };

        /// Traits hook
        /**
            \p NodeTraits defines type traits for node.
            See \ref node_traits for \p NodeTraits interface description

            \p Options are:
            - opt::gc - garbage collector used.
            - opt::tag - a tag
        */
        template <typename NodeTraits, CDS_DECL_OPTIONS2 >
        struct traits_hook: public hook< opt::traits_hook_tag, CDS_OPTIONS2 >
        {
            //@cond
            typedef NodeTraits node_traits ;
            //@endcond
        };

        /// Key extracting functor option setter
        template <typename Type>
        struct key_extractor {
            //@cond
            template <typename Base> struct pack: public Base
            {
                typedef Type key_extractor ;
            };
            //@endcond
        };

        /// Update descriptor allocator option setter
        template <typename Type>
        struct update_desc_allocator {
            //@cond
            template <typename Base> struct pack: public Base
            {
                typedef Type update_desc_allocator ;
            };
            //@endcond
        };


        /// Update descriptor pool option setter
        template <typename Type>
        struct update_desc_pool {
            //@cond
            template <typename Base> struct pack: public Base
            {
                typedef Type update_desc_pool ;
            };
            //@endcond
        };

        /// EllenBinTree internal statistics
        struct stat {
            typedef cds::atomicity::event_counter   event_counter ; ///< Event counter type

            event_counter   m_nInternalNodeCreated  ;   ///< Total count of created internal node
            event_counter   m_nInternalNodeDeleted  ;   ///< Total count of deleted internal node
            event_counter   m_nUpdateDescCreated    ;   ///< Total count of created update descriptors
            event_counter   m_nUpdateDescDeleted    ;   ///< Total count of deleted update descriptors

            event_counter   m_nInsertSuccess        ; ///< Count of success insertion
            event_counter   m_nInsertFailed         ; ///< Count of failed insertion
            event_counter   m_nInsertRetries        ; ///< Count of unsuccessful retries of insertion
            event_counter   m_nEnsureExist          ; ///< Count of \p ensure call for existed node
            event_counter   m_nEnsureNew            ; ///< Count of \p ensure call for new node
            event_counter   m_nEnsureRetries        ; ///< Count of unsuccessful retries of ensuring
            event_counter   m_nEraseSuccess         ; ///< Count of successful call of \p erase and \p unlink
            event_counter   m_nEraseFailed          ; ///< Count of failed call of \p erase and \p unlink
            event_counter   m_nEraseRetries         ; ///< Count of unsuccessful retries inside erasing/unlinking
            event_counter   m_nFindSuccess          ; ///< Count of successful \p find call
            event_counter   m_nFindFailed           ; ///< Count of failed \p find call
            event_counter   m_nExtractMinSuccess    ; ///< Count of successful call of \p extract_min
            event_counter   m_nExtractMinFailed     ; ///< Count of failed call of \p extract_min
            event_counter   m_nExtractMinRetries    ; ///< Count of unsuccessful retries inside \p extract_min
            event_counter   m_nExtractMaxSuccess    ; ///< Count of successful call of \p extract_max
            event_counter   m_nExtractMaxFailed     ; ///< Count of failed call of \p extract_max
            event_counter   m_nExtractMaxRetries    ; ///< Count of unsuccessful retries inside \p extract_max
            event_counter   m_nExtractSuccess       ; ///< Count of successful call of \p extract
            event_counter   m_nExtractFailed        ; ///< Count of failed call of \p extract
            event_counter   m_nExtractRetries       ; ///< Count of unsuccessful retries inside \p extract

            event_counter   m_nHelpInsert           ; ///< The number of insert help from the other thread
            event_counter   m_nHelpDelete           ; ///< The number of delete help from the other thread
            event_counter   m_nHelpMark             ; ///< The number of delete help (mark phase) from the other thread
            event_counter   m_nNodeLocked           ; ///< Count of locked node encountered

            //@cond
            void    onInternalNodeCreated()         { ++m_nInternalNodeCreated  ; }
            void    onInternalNodeDeleted()         { ++m_nInternalNodeDeleted  ; }
            void    onUpdateDescCreated()           { ++m_nUpdateDescCreated    ; }
            void    onUpdateDescDeleted()           { ++m_nUpdateDescDeleted    ; }
            void    onInsertSuccess()               { ++m_nInsertSuccess        ; }
            void    onInsertFailed()                { ++m_nInsertFailed         ; }
            void    onInsertRetry()                 { ++m_nInsertRetries        ; }
            void    onEnsureExist()                 { ++m_nEnsureExist          ; }
            void    onEnsureNew()                   { ++m_nEnsureNew            ; }
            void    onEnsureRetry()                 { ++m_nEnsureRetries        ; }
            void    onEraseSuccess()                { ++m_nEraseSuccess         ; }
            void    onEraseFailed()                 { ++m_nEraseFailed          ; }
            void    onEraseRetry()                  { ++m_nEraseRetries         ; }
            void    onExtractMinSuccess()           { ++m_nExtractMinSuccess    ; }
            void    onExtractMinFailed()            { ++m_nExtractMinFailed     ; }
            void    onExtractMinRetry()             { ++m_nExtractMinRetries    ; }
            void    onExtractMaxSuccess()           { ++m_nExtractMaxSuccess    ; }
            void    onExtractMaxFailed()            { ++m_nExtractMaxFailed     ; }
            void    onExtractMaxRetry()             { ++m_nExtractMaxRetries    ; }
            void    onExtractSuccess()              { ++m_nExtractSuccess       ; }
            void    onExtractFailed()               { ++m_nExtractFailed        ; }
            void    onExtractRetry()                { ++m_nExtractRetries       ; }
            void    onFindSuccess()                 { ++m_nFindSuccess          ; }
            void    onFindFailed()                  { ++m_nFindFailed           ; }
            void    onHelpInsert()                  { ++m_nHelpInsert           ; }
            void    onHelpDelete()                  { ++m_nHelpDelete           ; }
            void    onHelpMark()                    { ++m_nHelpMark             ; }
            void    onNodeLocked()                  { ++m_nNodeLocked           ; }
            //@endcond
        };

        /// EllenBinTree empty statistics
        struct empty_stat {
            //@cond
            void    onInternalNodeCreated()         {}
            void    onInternalNodeDeleted()         {}
            void    onUpdateDescCreated()           {}
            void    onUpdateDescDeleted()           {}
            void    onInsertSuccess()               {}
            void    onInsertFailed()                {}
            void    onInsertRetry()                 {}
            void    onEnsureExist()                 {}
            void    onEnsureNew()                   {}
            void    onEnsureRetry()                 {}
            void    onEraseSuccess()                {}
            void    onEraseFailed()                 {}
            void    onEraseRetry()                  {}
            void    onExtractMinSuccess()           {}
            void    onExtractMinFailed()            {}
            void    onExtractMinRetry()             {}
            void    onExtractMaxSuccess()           {}
            void    onExtractMaxFailed()            {}
            void    onExtractMaxRetry()             {}
            void    onExtractSuccess()              {}
            void    onExtractFailed()               {}
            void    onExtractRetry()                {}
            void    onFindSuccess()                 {}
            void    onFindFailed()                  {}
            void    onHelpInsert()                  {}
            void    onHelpDelete()                  {}
            void    onHelpMark()                    {}
            void    onNodeLocked()                  {}
            //@endcond
        };

        /// Type traits for EllenBinTree class
        struct type_traits
        {
            /// Hook used
            /**
                Possible values are: ellen_bintree::base_hook, ellen_bintree::member_hook, ellen_bintree::traits_hook.
            */
            typedef base_hook<>       hook        ;

            /// Key extracting functor
            /**
                You should explicit define a valid functor.
                The functor has the following prototype:
                \code
                struct key_extractor {
                    void operator ()( Key& dest, T const& src ) ;
                };
                \endcode
                It should initialize \p dest key from \p src data.
                The functor is used to initialize internal nodes.
            */
            typedef opt::none           key_extractor   ;

            /// Key comparison functor
            /**
                No default functor is provided. If the option is not specified, the \p less is used.

                See cds::opt::compare option description for functor interface.

                You should provide \p compare or \p less functor.
                See \ref cds_intrusive_EllenBinTree_rcu_less "predicate requirements".
            */
            typedef opt::none                       compare     ;

            /// Specifies binary predicate used for key compare.
            /**
                See cds::opt::less option description for predicate interface.

                You should provide \p compare or \p less functor.
                See \ref cds_intrusive_EllenBinTree_rcu_less "predicate requirements".
            */
            typedef opt::none                       less        ;

            /// Disposer
            /**
                The functor used for dispose removed items. Default is opt::v::empty_disposer.
            */
            typedef opt::v::empty_disposer          disposer    ;

            /// Item counter
            /**
                The type for item counting feature (see cds::opt::item_counter).
                Default is no item counter (\ref atomicity::empty_item_counter)
            */
            typedef atomicity::empty_item_counter     item_counter  ;

            /// C++ memory ordering model
            /**
                List of available memory ordering see opt::memory_model
            */
            typedef opt::v::relaxed_ordering        memory_model    ;

            /// Allocator for update descriptors
            /**
                The allocator type is used for \ref update_desc.

                Update descriptor is helping data structure with short lifetime and it is good candidate
                for pooling. The number of simultaneously existing descriptors is bounded number
                limited the number of threads working with the tree.
                Therefore, a bounded lock-free container like \p cds::container::VyukovMPMCCycleQueue
                is good choice for the free-list of update descriptors,
                see cds::memory::vyukov_queue_pool free-list implementation.

                Also notice that size of update descriptor is constant and not dependent on the type of data
                stored in the tree so single free-list object can be used for several \p EllenBinTree object.
            */
            typedef CDS_DEFAULT_ALLOCATOR           update_desc_allocator ;

            /// Allocator for internal nodes
            /**
                The allocator type is used for \ref internal_node.
            */
            typedef CDS_DEFAULT_ALLOCATOR           node_allocator   ;

            /// Internal statistics
            /**
                Possible types: ellen_bintree::empty_stat (the default), ellen_bintree::stat or any
                other with interface like \p %stat.
            */
            typedef empty_stat                      stat ;

            /// RCU deadlock checking policy (only for \ref cds_intrusive_EllenBinTree_rcu "RCU-based EllenBinTree")
            /**
                List of available options see opt::rcu_check_deadlock
            */
            typedef cds::opt::v::rcu_throw_deadlock      rcu_check_deadlock ;
        };

        /// Metafunction converting option list to EllenBinTree traits
        /**
            This is a wrapper for <tt> cds::opt::make_options< type_traits, Options...> </tt>
            \p Options list see \ref EllenBinTree.
        */
        template <CDS_DECL_OPTIONS12>
        struct make_traits {
#   ifdef CDS_DOXYGEN_INVOKED
            typedef implementation_defined type ;   ///< Metafunction result
#   else
            typedef typename cds::opt::make_options<
                typename cds::opt::find_type_traits< type_traits, CDS_OPTIONS12 >::type
                ,CDS_OPTIONS12
            >::type   type ;
#   endif
        };

        //@cond
        namespace details {

            template <typename Key, typename T, typename Compare, typename NodeTraits>
            struct compare
            {
                typedef Compare     key_compare ;
                typedef Key         key_type    ;
                typedef T           value_type  ;
                typedef NodeTraits  node_traits ;

                template <typename Q1, typename Q2>
                int operator()( Q1 const& v1, Q2 const& v2) const
                {
                    return key_compare()( v1, v2 ) ;
                }

                template <typename LeafNode>
                int operator()( internal_node<key_type, LeafNode> const& n1, internal_node<key_type, LeafNode> const& n2 ) const
                {
                    if ( n1.infinite_key() )
                        return n2.infinite_key() ? n1.infinite_key() - n2.infinite_key() : 1 ;
                    else if ( n2.infinite_key() )
                        return -1 ;
                    return operator()( n1.m_Key, n2.m_Key ) ;
                }

                template <typename LeafNode, typename Q>
                int operator()( internal_node<key_type, LeafNode> const& n, Q const& v ) const
                {
                    if ( n.infinite_key() )
                        return 1    ;
                    return operator()( n.m_Key, v ) ;
                }

                template <typename LeafNode, typename Q>
                int operator()( Q const& v, internal_node<key_type, LeafNode> const& n ) const
                {
                    if ( n.infinite_key() )
                        return -1    ;
                    return operator()( v, n.m_Key ) ;
                }

                template <typename GC, typename Tag>
                int operator()( node<GC, Tag> const& n1, node<GC, Tag> const& n2 ) const
                {
                    if ( n1.infinite_key() != n2.infinite_key() )
                        return n1.infinite_key() - n2.infinite_key() ;
                    return operator()( *node_traits::to_value_ptr( n1 ), *node_traits::to_value_ptr( n2 )) ;
                }

                template <typename GC, typename Tag, typename Q>
                int operator()( node<GC, Tag> const& n, Q const& v ) const
                {
                    if ( n.infinite_key() )
                        return 1 ;
                    return operator()( *node_traits::to_value_ptr( n ), v ) ;
                }

                template <typename GC, typename Tag, typename Q>
                int operator()( Q const& v, node<GC, Tag> const& n ) const
                {
                    if ( n.infinite_key() )
                        return -1 ;
                    return operator()( v, *node_traits::to_value_ptr( n ) ) ;
                }

                template <typename GC>
                int operator()( base_node<GC> const& n1, base_node<GC> const& n2 ) const
                {
                    if ( n1.infinite_key() != n2.infinite_key() )
                        return n1.infinite_key() - n2.infinite_key() ;
                    if ( n1.is_leaf() ) {
                        if ( n2.is_leaf() )
                            return operator()( node_traits::to_leaf_node( n1 ), node_traits::to_leaf_node( n2 )) ;
                        else
                            return operator()( node_traits::to_leaf_node( n1 ), node_traits::to_internal_node( n2 )) ;
                    }

                    if ( n2.is_leaf() )
                        return operator()( node_traits::to_internal_node( n1 ), node_traits::to_leaf_node( n2 )) ;
                    else
                        return operator()( node_traits::to_internal_node( n1 ), node_traits::to_internal_node( n2 )) ;
                }

                template <typename GC, typename Q>
                int operator()( base_node<GC> const& n, Q const& v ) const
                {
                    if ( n.infinite_key())
                        return 1 ;
                    if ( n.is_leaf() )
                        return operator()( node_traits::to_leaf_node( n ), v ) ;
                    return operator()( node_traits::to_internal_node( n ), v ) ;
                }

                template <typename GC, typename Q>
                int operator()( Q const& v, base_node<GC> const& n ) const
                {
                    return -operator()( n, v ) ;
                }

                template <typename GC, typename LeafNode >
                int operator()( base_node<GC> const& i, internal_node<key_type, LeafNode> const& n ) const
                {
                    if ( i.is_leaf() )
                        return operator()( static_cast<LeafNode const&>(i), n ) ;
                    return operator()( static_cast<internal_node<key_type, LeafNode> const&>(i), n ) ;
                }

                template <typename GC, typename LeafNode >
                int operator()( internal_node<key_type, LeafNode> const& n, base_node<GC> const& i ) const
                {
                    return -operator()( i, n )  ;
                }

                template <typename GC, typename Tag >
                int operator()( node<GC, Tag> const& n, internal_node<key_type, node<GC, Tag> > const& i ) const
                {
                    if ( !n.infinite_key() ) {
                        if ( i.infinite_key() )
                            return -1 ;
                        return operator()( n, i.m_Key ) ;
                    }

                    if ( !i.infinite_key())
                        return 1 ;
                    return int( n.infinite_key()) - int( i.infinite_key()) ;
                }

                template <typename GC, typename Tag >
                int operator()( internal_node<key_type, node<GC, Tag> > const& i, node<GC, Tag> const& n ) const
                {
                    return -operator()( n, i ) ;
                }

            };

        } // namespace details
        //@endcond

    }   // namespace ellen_bintree

    // Forwards
    template < class GC, typename Key, typename T, class Traits = ellen_bintree::type_traits >
    class EllenBinTree ;

}} // namespace cds::intrusive

#endif  // #ifndef __CDS_INTRUSIVE_DETAILS_ELLEN_BINTREE_BASE_H
