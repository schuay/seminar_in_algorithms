//$$CDS-header$$

#ifndef __CDS_INTRUSIVE_ELLEN_BINTREE_RCU_H
#define __CDS_INTRUSIVE_ELLEN_BINTREE_RCU_H

#include <cds/intrusive/details/ellen_bintree_base.h>
#include <cds/opt/compare.h>
#include <cds/ref.h>
#include <cds/details/binary_functor_wrapper.h>
#include <cds/urcu/details/check_deadlock.h>
#include <cds/details/std/memory.h>

namespace cds { namespace intrusive {

    /// Ellen's et al binary search tree (RCU specialization)
    /** @ingroup cds_intrusive_map
        @ingroup cds_intrusive_tree
        @anchor cds_intrusive_EllenBinTree_rcu

        Source:
            - [2010] F.Ellen, P.Fatourou, E.Ruppert, F.van Breugel "Non-blocking Binary Search Tree"

        %EllenBinTree is an unbalanced leaf-oriented binary search tree that implements the <i>set</i>
        abstract data type. Nodes maintains child pointers but not parent pointers.
        Every internal node has exactly two children, and all data of type \p T currently in
        the tree are stored in the leaves. Internal nodes of the tree are used to direct \p find
        operation along the path to the correct leaf. The keys (of \p Key type) stored in internal nodes
        may or may not be in the set. \p Key type is a subset of \p T type.
        There should be exactly defined a key extracting functor for converting object of type \p T to
        object of type \p Key.

        Due to \p extract_min and \p extract_max member functions the \p %EllenBinTree can act as
        a <i>priority queue</i>. In this case you should provide unique compound key, for example,
        the priority value plus some uniformly distributed random value.

        @warning Recall the tree is <b>unbalanced</b>. The complexity of operations is <tt>O(log N)</tt>
        for uniformly distributed random keys, but in worst case the complexity is <tt>O(N)</tt>.

        @note In the current implementation we do not use helping technique described in original paper.
        So, the current implementation is near to fine-grained lock-based tree.
        Helping will be implemented in future release

        <b>Template arguments</b> :
        - \p RCU - one of \ref cds_urcu_gc "RCU type"
        - \p Key - key type, a subset of \p T
        - \p T - type to be stored in tree's leaf nodes. The type must be based on ellen_bintree::node
            (for ellen_bintree::base_hook) or it must have a member of type ellen_bintree::node
            (for ellen_bintree::member_hook).
        - \p Traits - type traits. See ellen_bintree::type_traits for explanation.

        It is possible to declare option-based tree with cds::intrusive::ellen_bintree::make_traits metafunction
        instead of \p Traits template argument.
        Template argument list \p Options of cds::intrusive::ellen_bintree::make_traits metafunction are:
        - opt::hook - hook used. Possible values are: ellen_bintree::base_hook, ellen_bintree::member_hook, ellen_bintree::traits_hook.
            If the option is not specified, <tt>ellen_bintree::base_hook<></tt> is used.
        - ellen_bintree::key_extractor - key extracting functor, mandatory option. The functor has the following prototype:
            \code
                struct key_extractor {
                    void operator ()( Key& dest, T const& src ) ;
                };
            \endcode
            It should initialize \p dest key from \p src data. The functor is used to initialize internal nodes.
        - opt::compare - key compare functor. No default functor is provided.
            If the option is not specified, \p %opt::less is used.
        - opt::less - specifies binary predicate used for key compare. At least \p %opt::compare or \p %opt::less should be defined.
        - opt::disposer - the functor used for dispose removed nodes. Default is opt::v::empty_disposer. Due the nature
            of GC schema the disposer may be called asynchronously. The disposer is used only for leaf nodes.
        - opt::item_counter - the type of item counting feature. Default is \ref atomicity::empty_item_counter that is no item counting.
        - opt::memory_model - C++ memory ordering model. Can be opt::v::relaxed_ordering (relaxed memory model, the default)
            or opt::v::sequential_consistent (sequentially consisnent memory model).
        - ellen_bintree::update_desc_allocator - an allocator of \ref ellen_bintree::update_desc "update descriptors",
            default is CDS_DEFAULT_ALLOCATOR.
            Note that update descriptor is helping data structure with short lifetime and it is good candidate for pooling.
            The number of simultaneously existing descriptors is bounded and depends on the number of threads
            working with the tree and RCU buffer size (if RCU is buffered).
            Therefore, a bounded lock-free container like \p cds::container::VyukovMPMCCycleQueue is good candidate
            for the free-list of update descriptors, see cds::memory::vyukov_queue_pool free-list implementation.
            Also notice that size of update descriptor is constant and not dependent on the type of data
            stored in the tree so single free-list object can be used for all \p %EllenBinTree objects.
        - opt::node_allocator - the allocator used for internal nodes. Default is \ref CDS_DEFAULT_ALLOCATOR.
        - opt::stat - internal statistics. Available types: ellen_bintree::stat, ellen_bintree::empty_stat (the default)
        - opt::rcu_check_deadlock - a deadlock checking policy. Default is opt::v::rcu_throw_deadlock

        @anchor cds_intrusive_EllenBinTree_rcu_less
        <b>Predicate requirements</b>

        opt::less, opt::compare and other predicates using with member fuctions should accept at least parameters
        of type \p T and \p Key in any combination.
        For example, for \p Foo struct with \p std::string key field the appropiate \p less functor is:
        \code
        struct Foo: public cds::intrusive::ellen_bintree::node< ... >
        {
            std::string m_strKey ;
            ...
        } ;

        struct less {
            bool operator()( Foo const& v1, Foo const& v2 ) const
            { return v1.m_strKey < v2.m_strKey ; }

            bool operator()( Foo const& v, std::string const& s ) const
            { return v.m_strKey < s ; }

            bool operator()( std::string const& s, Foo const& v ) const
            { return s < v.m_strKey ; }

            // Support comparing std::string and char const *
            bool operator()( std::string const& s, char const * p ) const
            { return s.compare(p) < 0 ; }

            bool operator()( Foo const& v, char const * p ) const
            { return v.m_strKey.compare(p) < 0 ; }

            bool operator()( char const * p, std::string const& s ) const
            { return s.compare(p) > 0; }

            bool operator()( char const * p, Foo const& v ) const
            { return v.m_strKey.compare(p) > 0; }
        };
        \endcode

        @note Before including <tt><cds/intrusive/ellen_bintree_rcu.h></tt> you should include appropriate RCU header file,
        see \ref cds_urcu_gc "RCU type" for list of existing RCU class and corresponding header files.

        <b>Usage</b>

        Suppose we have the following Foo struct with string key type:
        \code
        struct Foo {
            std::string     m_strKey    ;   // The key
            //...                           // other non-key data
        };
        \endcode

        We want to utilize RCU-based \p %cds::intrusive::EllenBinTree set for \p Foo data.
        We may use base hook or member hook. Consider base hook variant.
        First, we need deriving \p Foo struct from \p cds::intrusive::ellen_bintree::node:
        \code
        #include <cds/urcu/general_buffered.h>
        #include <cds/intrusive/ellen_bintree_rcu.h>

        // RCU type we use
        typedef cds::urcu::gc< cds::urcu::general_buffered<> >  gpb_rcu ;

        struct Foo: public cds::intrusive:ellen_bintree::node< gpb_rcu >
        {
            std::string     m_strKey    ;   // The key
            //...                           // other non-key data
        };
        \endcode

        Second, we need to implement auxiliary structures and functors:
        - key extractor functor for extracting the key from \p Foo object.
            Such functor is necessary because the tree internal nodes store the keys.
        - \p less predicate. We want our set should accept \p std::string
            and <tt>char const *</tt> parameters for searching, so our \p less
            predicate should be non-trivial, see below.
        - item counting feature: we want our set's \p size() member function
            returns actual item count.

        \code
        // Key extractor functor
        struct my_key_extractor
        {
            void operator ()( std::string& key, Foo const& src ) const
            {
                key = src.m_strKey  ;
            }
        };

        // Less predicate
        struct my_less {
            bool operator()( Foo const& v1, Foo const& v2 ) const
            { return v1.m_strKey < v2.m_strKey ; }

            bool operator()( Foo const& v, std::string const& s ) const
            { return v.m_strKey < s ; }

            bool operator()( std::string const& s, Foo const& v ) const
            { return s < v.m_strKey ; }

            // Support comparing std::string and char const *
            bool operator()( std::string const& s, char const * p ) const
            { return s.compare(p) < 0 ; }

            bool operator()( Foo const& v, char const * p ) const
            { return v.m_strKey.compare(p) < 0 ; }

            bool operator()( char const * p, std::string const& s ) const
            { return s.compare(p) > 0; }

            bool operator()( char const * p, Foo const& v ) const
            { return v.m_strKey.compare(p) > 0; }
        };

        // Type traits for our set
        // It is necessary to specify only those typedefs that differ from
        // cds::intrusive::ellen_bintree::type_traits defaults.
        struct set_traits: public cds::intrusive::ellen_bintree::type_traits
        {
            typedef cds::intrusive::ellen_bintree::base_hook< cds::opt::gc<gpb_rcu> > > hook ;
            typedef my_key_extractor    key_extractor   ;
            typedef my_less             less            ;
            typedef cds::atomicity::item_counter item_counter  ;
        };
        \endcode

        Now we declare \p %EllenBinTree set and use it:
        \code
        typedef cds::intrusive::EllenBinTree< gpb_rcu, std::string, Foo, set_traits >   set_type ;

        set_type    theSet  ;
        // ...
        \endcode

        Instead of declaring \p set_traits type traits we can use option-based syntax with \p %make_traits metafunction,
        for example:
        \code
        typedef cds::intrusive::EllenBinTree< gpb_rcu, std::string, Foo,
            typename cds::intrusive::ellen_bintree::make_traits<
                cds::opt::hook< cds::intrusive::ellen_bintree::base_hook< cds::opt::gc<gpb_rcu> > >
                ,cds::intrusive::ellen_bintree::key_extractor< my_key_extractor >
                ,cds::opt::less< my_less >
                ,cds::opt::item_counter< cds::atomicity::item_counter >
            >::type
        >   set_type2 ;
        \endcode

        Functionally, \p set_type and \p set_type2 are equivalent.

        <b>Member-hooked tree</b>

        Sometimes, we cannot use base hook, for example, when the \p Foo structure is external.
        In such case we can use member hook feature.
        \code
        #include <cds/urcu/general_buffered.h>
        #include <cds/intrusive/ellen_bintree_rcu.h>

        // Struct Foo is external and its declaration cannot be modified.
        struct Foo {
            std::string     m_strKey    ;   // The key
            //...                           // other non-key data
        };

        // RCU type we use
        typedef cds::urcu::gc< cds::urcu::general_buffered<> >  gpb_rcu ;

        // Foo wrapper
        struct MyFoo
        {
            Foo     m_foo   ;
            cds::intrusive:ellen_bintree::node< gpb_rcu >   set_hook  ;   // member hook
        } ;

        // Key extractor functor
        struct member_key_extractor
        {
            void operator ()( std::string& key, MyFoo const& src ) const
            {
                key = src.m_foo.m_strKey  ;
            }
        };

        // Less predicate
        struct member_less {
            bool operator()( MyFoo const& v1, MyFoo const& v2 ) const
            { return v1.m_foo.m_strKey < v2.m_foo.m_strKey ; }

            bool operator()( MyFoo const& v, std::string const& s ) const
            { return v.m_foo.m_strKey < s ; }

            bool operator()( std::string const& s, MyFoo const& v ) const
            { return s < v.m_foo.m_strKey ; }

            // Support comparing std::string and char const *
            bool operator()( std::string const& s, char const * p ) const
            { return s.compare(p) < 0 ; }

            bool operator()( MyFoo const& v, char const * p ) const
            { return v.m_foo.m_strKey.compare(p) < 0 ; }

            bool operator()( char const * p, std::string const& s ) const
            { return s.compare(p) > 0; }

            bool operator()( char const * p, MyFoo const& v ) const
            { return v.m_foo.m_strKey.compare(p) > 0; }
        };

        // Type traits for our member-based set
        struct member_set_traits: public cds::intrusive::ellen_bintree::type_traits
        {
            cds::intrusive::ellen_bintree::member_hook< offsetof(MyFoo, set_hook), cds::opt::gc<gpb_rcu> > > hook ;
            typedef member_key_extractor    key_extractor   ;
            typedef member_less             less            ;
            typedef cds::atomicity::item_counter item_counter  ;
        };

        // Tree containing MyFoo objects
        typedef cds::intrusive::EllenBinTree< gpb_rcu, std::string, MyFoo, member_set_traits >   member_set_type ;

        member_set_type    theMemberSet  ;
        \endcode

        <b>Multiple containers</b>

        Sometimes we need that our \p Foo struct should be used in several different containers.
        Suppose, \p Foo struct has two key fields:
        \code
        struct Foo {
            std::string m_strKey    ;   // string key
            int         m_nKey      ;   // int key
            //...                       // other non-key data fields
        };
        \endcode

        We want to build two intrusive \p %EllenBinTree sets: one indexed on \p Foo::m_strKey field,
        another indexed on \p Foo::m_nKey field. To decide such case we should use a tag option for
        tree's hook:
        \code
        #include <cds/urcu/general_buffered.h>
        #include <cds/intrusive/ellen_bintree_rcu.h>

        // RCU type we use
        typedef cds::urcu::gc< cds::urcu::general_buffered<> >  gpb_rcu ;

        // Declare tag structs
        struct int_tag      ;   // in key tag
        struct string_tag   ;   // string key tag

        // Foo struct is derived from two ellen_bintree::node class
        // with different tags
        struct Foo
            : public cds::intrusive::ellen_bintree::node< gpb_rcu, cds::opt::tag< string_tag > >
            , public cds::intrusive::ellen_bintree::node< gpb_rcu >, cds::opt::tag< int_tag >
        {
            std::string m_strKey    ;   // string key
            int         m_nKey      ;   // int key
            //...                       // other non-key data fields
        };

        // String key extractor functor
        struct string_key_extractor
        {
            void operator ()( std::string& key, Foo const& src ) const
            {
                key = src.m_strKey  ;
            }
        };

        // Int key extractor functor
        struct int_key_extractor
        {
            void operator ()( int& key, Foo const& src ) const
            {
                key = src.m_nKey  ;
            }
        };

        // String less predicate
        struct string_less {
            bool operator()( Foo const& v1, Foo const& v2 ) const
            { return v1.m_strKey < v2.m_strKey ; }

            bool operator()( Foo const& v, std::string const& s ) const
            { return v.m_strKey < s ; }

            bool operator()( std::string const& s, Foo const& v ) const
            { return s < v.m_strKey ; }

            // Support comparing std::string and char const *
            bool operator()( std::string const& s, char const * p ) const
            { return s.compare(p) < 0 ; }

            bool operator()( Foo const& v, char const * p ) const
            { return v.m_strKey.compare(p) < 0 ; }

            bool operator()( char const * p, std::string const& s ) const
            { return s.compare(p) > 0; }

            bool operator()( char const * p, Foo const& v ) const
            { return v.m_strKey.compare(p) > 0; }
        };

        // Int less predicate
        struct int_less {
            bool operator()( Foo const& v1, Foo const& v2 ) const
            { return v1.m_nKey < v2.m_nKey ; }

            bool operator()( Foo const& v, int n ) const
            { return v.m_nKey < n ; }

            bool operator()( int n, Foo const& v ) const
            { return n < v.m_nKey ; }
        };

        // Type traits for string-indexed set
        struct string_set_traits: public cds::intrusive::ellen_bintree::type_traits
        {
            typedef cds::intrusive::ellen_bintree::base_hook< cds::opt::gc<gpb_rcu> >, cds::opt::tag< string_tag > > hook ;
            typedef string_key_extractor    key_extractor   ;
            typedef string_less             less            ;
            typedef cds::atomicity::item_counter item_counter  ;
        };

        // Type traits for int-indexed set
        struct int_set_traits: public cds::intrusive::ellen_bintree::type_traits
        {
            typedef cds::intrusive::ellen_bintree::base_hook< cds::opt::gc<gpb_rcu> >, cds::opt::tag< int_tag > > hook ;
            typedef int_key_extractor    key_extractor   ;
            typedef int_less             less            ;
            typedef cds::atomicity::item_counter item_counter  ;
        };

        // Declare string-indexed set
        typedef cds::intrusive::EllenBinTree< gpb_rcu, std::string, Foo, string_set_traits >   string_set_type ;
        string_set_type theStringSet    ;

        // Declare int-indexed set
        typedef cds::intrusive::EllenBinTree< gpb_rcu, int, Foo, int_set_traits >   int_set_type ;
        int_set_type    theIntSet   ;

        // Now we can use theStringSet and theIntSet in our program
        // ...
        \endcode
    */
    template < class RCU,
        typename Key,
        typename T,
#ifdef CDS_DOXYGEN_INVOKED
        class Traits = ellen_bintree::type_traits
#else
        class Traits
#endif
    >
    class EllenBinTree< cds::urcu::gc<RCU>, Key, T, Traits >
    {
    public:
        typedef cds::urcu::gc<RCU>  gc  ;   ///< RCU Garbage collector
        typedef Key     key_type        ;   ///< type of a key stored in internal nodes; key is a part of \p value_type
        typedef T       value_type      ;   ///< type of value stored in the binary tree
        typedef Traits  options         ;   ///< Traits template parameter

        typedef typename options::hook      hook        ;   ///< hook type
        typedef typename hook::node_type    node_type   ;   ///< node type

        typedef typename options::disposer  disposer    ;   ///< leaf node disposer

    protected:
        //@cond
        typedef ellen_bintree::base_node< gc >                      tree_node       ; ///< Base type of tree node
        typedef node_type                                           leaf_node       ; ///< Leaf node type
        typedef ellen_bintree::internal_node< key_type, leaf_node > internal_node   ; ///< Internal node type
        typedef ellen_bintree::update_desc< leaf_node, internal_node> update_desc   ; ///< Update descriptor
        typedef typename update_desc::update_ptr                    update_ptr      ; ///< Marked pointer to update descriptor
        //@endcond

    public:
#   ifdef CDS_DOXYGEN_INVOKED
        typedef implementation_defined key_comparator  ;    ///< key compare functor based on opt::compare and opt::less option setter.
        typedef typename get_node_traits< value_type, node_type, hook>::type node_traits ; ///< Node traits
#   else
        typedef typename opt::details::make_comparator< value_type, options >::type key_comparator ;
        struct node_traits: public get_node_traits< value_type, node_type, hook>::type
        {
            static internal_node const& to_internal_node( tree_node const& n )
            {
                assert( n.is_internal() ) ;
                return static_cast<internal_node const&>( n ) ;
            }

            static leaf_node const& to_leaf_node( tree_node const& n )
            {
                assert( n.is_leaf() ) ;
                return static_cast<leaf_node const&>( n ) ;
            }
        };
#   endif

        typedef typename options::item_counter  item_counter;   ///< Item counting policy used
        typedef typename options::memory_model  memory_model;   ///< Memory ordering. See cds::opt::memory_model option
        typedef typename options::stat          stat        ;   ///< internal statistics type
        typedef typename options::rcu_check_deadlock    rcu_check_deadlock ; ///< Deadlock checking policy
        typedef typename options::key_extractor         key_extractor   ;   ///< key extracting functor

        typedef typename options::node_allocator        node_allocator  ;   ///< Internal node allocator
        typedef typename options::update_desc_allocator update_desc_allocator ; ///< Update descriptor allocator

    protected:
        //@cond
        typedef ellen_bintree::details::compare< key_type, value_type, key_comparator, node_traits > node_compare ;

        typedef typename gc::scoped_lock    rcu_lock ;
        typedef cds::urcu::details::check_deadlock_policy< gc, rcu_check_deadlock >   check_deadlock_policy ;

        typedef cds::details::Allocator< internal_node, node_allocator >        cxx_node_allocator   ;
        typedef cds::details::Allocator< update_desc, update_desc_allocator >   cxx_update_desc_allocator   ;

        struct search_result {
            internal_node *     pGrandParent    ;
            internal_node *     pParent         ;
            leaf_node *         pLeaf           ;
            update_ptr          updParent       ;
            update_ptr          updGrandParent  ;
            bool                bRightLeaf      ; // true if pLeaf is right child of pParent, false otherwise
            bool                bRightParent    ; // true if pParent is right child of pGrandParent, false otherwise

            search_result()
                :pGrandParent( null_ptr<internal_node *>() )
                ,pParent( null_ptr<internal_node *>() )
                ,pLeaf( null_ptr<leaf_node *>() )
                ,bRightLeaf( false )
                ,bRightParent( false )
            {}
        };
        //@endcond

    protected:
        //@cond
        internal_node       m_Root          ;   ///< Tree root node (key= Infinite2)
        leaf_node           m_LeafInf1      ;
        leaf_node           m_LeafInf2      ;
        //@endcond

        item_counter        m_ItemCounter   ;   ///< item counter
        mutable stat        m_Stat          ;   ///< internal statistics

    protected:
        //@cond
        static void free_leaf_node( value_type * p )
        {
            disposer()( p ) ;
        }

        internal_node * alloc_internal_node() const
        {
            m_Stat.onInternalNodeCreated() ;
            internal_node * pNode = cxx_node_allocator().New() ;
            //pNode->clean()  ;
            return pNode    ;
        }

        static void free_internal_node( internal_node * pNode )
        {
            cxx_node_allocator().Delete( pNode ) ;
        }

        struct internal_node_deleter {
            void operator()( internal_node * p) const
            {
                free_internal_node( p ) ;
            }
        };

        typedef std::unique_ptr< internal_node, internal_node_deleter>  unique_internal_node_ptr ;

        update_desc * alloc_update_desc() const
        {
            m_Stat.onUpdateDescCreated()    ;
            return cxx_update_desc_allocator().New() ;
        }

        static void free_update_desc( update_desc * pDesc )
        {
            cxx_update_desc_allocator().Delete( pDesc ) ;
        }

        class retired_list
        {
            update_desc *   pUpdateHead ;
            tree_node *     pNodeHead   ;

        private:
            class forward_iterator
            {
                update_desc *   m_pUpdate ;
                tree_node *     m_pNode   ;

            public:
                forward_iterator( retired_list const& l )
                    : m_pUpdate( l.pUpdateHead )
                    , m_pNode( l.pNodeHead )
                {}

                forward_iterator()
                    : m_pUpdate( null_ptr<update_desc *>() )
                    , m_pNode( null_ptr< tree_node *>() )
                {}

                cds::urcu::retired_ptr operator *()
                {
                    if ( m_pUpdate ) {
                        return cds::urcu::retired_ptr( reinterpret_cast<void *>( m_pUpdate ),
                            reinterpret_cast<cds::urcu::free_retired_ptr_func>( free_update_desc ) ) ;
                    }
                    if ( m_pNode ) {
                        if ( m_pNode->is_leaf() ) {
                            return cds::urcu::retired_ptr( reinterpret_cast<void *>( node_traits::to_value_ptr( static_cast<leaf_node *>( m_pNode ))),
                                reinterpret_cast< cds::urcu::free_retired_ptr_func>( free_leaf_node ) ) ;
                        }
                        else {
                            return cds::urcu::retired_ptr( reinterpret_cast<void *>( static_cast<internal_node *>( m_pNode ) ),
                                reinterpret_cast<cds::urcu::free_retired_ptr_func>( free_internal_node ) ) ;
                        }
                    }
                    return cds::urcu::retired_ptr( null_ptr<void *>(),
                        reinterpret_cast<cds::urcu::free_retired_ptr_func>( free_update_desc ) ) ;
                }

                void operator ++()
                {
                    if ( m_pUpdate ) {
                        m_pUpdate = m_pUpdate->pNextRetire  ;
                        return ;
                    }
                    if ( m_pNode )
                        m_pNode = m_pNode->m_pNextRetired   ;
                }

                friend bool operator ==( forward_iterator const& i1, forward_iterator const& i2 )
                {
                    return i1.m_pUpdate == i2.m_pUpdate && i1.m_pNode == i2.m_pNode ;
                }
                friend bool operator !=( forward_iterator const& i1, forward_iterator const& i2 )
                {
                    return !( i1 == i2 ) ;
                }
            };

        public:
            retired_list()
                : pUpdateHead( null_ptr<update_desc *>() )
                , pNodeHead( null_ptr<tree_node *>() )
            {}

            ~retired_list()
            {
                gc::batch_retire( forward_iterator(*this), forward_iterator() ) ;
            }

            void push( update_desc * p )
            {
                p->pNextRetire = pUpdateHead ;
                pUpdateHead = p ;
            }

            void push( tree_node * p )
            {
                p->m_pNextRetired = pNodeHead   ;
                pNodeHead = p   ;
            }
        };

        void retire_node( tree_node * pNode, retired_list& rl ) const
        {
            if ( pNode->is_leaf() ) {
                assert( static_cast<leaf_node *>( pNode ) != &m_LeafInf1 ) ;
                assert( static_cast<leaf_node *>( pNode ) != &m_LeafInf2 ) ;
            }
            else {
                assert( static_cast<internal_node *>( pNode ) != &m_Root ) ;
                m_Stat.onInternalNodeDeleted() ;
            }
            rl.push( pNode ) ;
        }

        void retire_update_desc( update_desc * p, retired_list& rl, bool bDirect ) const
        {
            m_Stat.onUpdateDescDeleted() ;
            if ( bDirect )
                free_update_desc( p ) ;
            else
                rl.push( p ) ;
        }

        void make_empty_tree()
        {
            m_Root.infinite_key( 2 )    ;
            m_LeafInf1.infinite_key( 1 ) ;
            m_LeafInf2.infinite_key( 2 ) ;
            m_Root.m_pLeft.store( &m_LeafInf1, memory_model::memory_order_relaxed ) ;
            m_Root.m_pRight.store( &m_LeafInf2, memory_model::memory_order_release );
        }

#   ifndef CDS_CXX11_LAMBDA_SUPPORT
        struct trivial_equal_functor {
            template <typename Q>
            bool operator()( Q const& , leaf_node const& ) const
            {
                return true ;
            }
        };

        struct empty_insert_functor {
            void operator()( value_type& )
            {}
        };
#   endif
#   if !defined(CDS_CXX11_LAMBDA_SUPPORT) || (CDS_COMPILER == CDS_COMPILER_MSVC && CDS_COMPILER_VERSION == CDS_COMPILER_MSVC10)
        struct unlink_equal_functor {
            bool operator()( value_type const& v, leaf_node const& n ) const
            {
                return &v == node_traits::to_value_ptr( n ) ;
            }
        };
        struct empty_erase_functor  {
            void operator()( value_type const& )
            {}
        };
#   endif
        //@endcond

    public:
        /// Default constructor
        EllenBinTree()
        {
            static_assert( (!std::is_same< key_extractor, opt::none >::value), "The key extractor option must be specified" ) ;
            make_empty_tree()   ;
        }

        /// Clears the tree
        ~EllenBinTree()
        {
            clear() ;
        }

        /// Inserts new node
        /**
            The function inserts \p val in the tree if it does not contain
            an item with key equal to \p val.

            The function applies RCU lock internally.

            Returns \p true if \p val is placed into the set, \p false otherwise.
        */
        bool insert( value_type& val )
        {
#   ifdef CDS_CXX11_LAMBDA_SUPPORT
            return insert( val, []( value_type& ) {} ) ;
#   else
            return insert( val, empty_insert_functor() ) ;
#   endif
        }

        /// Inserts new node
        /**
            This function is intended for derived non-intrusive containers.

            The function allows to split creating of new item into two part:
            - create item with key only
            - insert new item into the tree
            - if inserting is success, calls  \p f functor to initialize value-field of \p val.

            The functor signature is:
            \code
                void func( value_type& val ) ;
            \endcode
            where \p val is the item inserted. User-defined functor \p f should guarantee that during changing
            \p val no any other changes could be made on this tree's item by concurrent threads.
            The user-defined functor is called only if the inserting is success and may be passed by reference
            using <tt>boost::ref</tt>

            RCU \p synchronize method can be called. RCU should not be locked.
        */
        template <typename Func>
        bool insert( value_type& val, Func f )
        {
            check_deadlock_policy::check() ;

            unique_internal_node_ptr pNewInternal ;
            retired_list updRetire ;

            {
                rcu_lock l ;

                search_result res ;
                for ( ;; ) {
                    if ( search( res, val, node_compare() )) {
                        if ( pNewInternal.get() )
                            m_Stat.onInternalNodeDeleted() ;    // unique_internal_node_ptr deletes internal node
                        m_Stat.onInsertFailed() ;
                        return false    ;
                    }

                    if ( res.updParent.all() != update_desc::locked_desc() ) {
                        if ( res.updParent.bits() != update_desc::Clean )
                            help( res.updParent, updRetire ) ;
                        else {
                            if ( !pNewInternal.get() )
                                pNewInternal.reset( alloc_internal_node() );

                            if ( try_insert( val, pNewInternal.get(), res, updRetire )) {
                                cds::unref(f)( val )    ;
                                pNewInternal.release()  ;   // internal node is linked into the tree and should not be deleted
                                break;
                            }
                        }
                    }
                    else
                        m_Stat.onNodeLocked() ;

                    m_Stat.onInsertRetry()  ;
                }
            }

            ++m_ItemCounter ;
            m_Stat.onInsertSuccess()    ;

            return true ;
        }

        /// Ensures that the \p val exists in the tree
        /**
            The operation performs inserting or changing data with lock-free manner.

            If the item \p val is not found in the tree, then \p val is inserted into the tree.
            Otherwise, the functor \p func is called with item found.
            The functor signature is:
            \code
                void func( bool bNew, value_type& item, value_type& val ) ;
            \endcode
            with arguments:
            - \p bNew - \p true if the item has been inserted, \p false otherwise
            - \p item - item of the tree
            - \p val - argument \p val passed into the \p ensure function
            If new item has been inserted (i.e. \p bNew is \p true) then \p item and \p val arguments
            refer to the same thing.

            The functor can change non-key fields of the \p item; however, \p func must guarantee
            that during changing no any other modifications could be made on this item by concurrent threads.

            You can pass \p func argument by value or by reference using <tt>boost::ref</tt> or cds::ref.

            RCU \p synchronize method can be called. RCU should not be locked.

            Returns std::pair<bool, bool> where \p first is \p true if operation is successfull,
            \p second is \p true if new item has been added or \p false if the item with \p key
            already is in the tree.
        */
        template <typename Func>
        std::pair<bool, bool> ensure( value_type& val, Func func )
        {
            check_deadlock_policy::check() ;

            unique_internal_node_ptr pNewInternal ;
            retired_list updRetire ;

            {
                rcu_lock l ;

                search_result res ;
                for ( ;; ) {
                    if ( search( res, val, node_compare() )) {
                        cds::unref(func)( false, *node_traits::to_value_ptr( res.pLeaf ), val ) ;
                        if ( pNewInternal.get() )
                            m_Stat.onInternalNodeDeleted() ;    // unique_internal_node_ptr deletes internal node
                        m_Stat.onEnsureExist() ;
                        return std::make_pair( true, false ) ;
                    }

                    if ( res.updParent.bits() != update_desc::Clean )
                        help( res.updParent, updRetire ) ;
                    else {
                        if ( !pNewInternal.get() )
                            pNewInternal.reset( alloc_internal_node() );

                        if ( try_insert( val, pNewInternal.get(), res, updRetire )) {
                            cds::unref(func)( true, val, val )    ;
                            pNewInternal.release()  ;   // internal node is linked into the tree and should not be deleted
                            break;
                        }
                    }
                    m_Stat.onEnsureRetry()  ;
                }
            }

            ++m_ItemCounter ;
            m_Stat.onEnsureNew()    ;

            return std::make_pair( true, true ) ;
        }

        /// Unlinks the item \p val from the tree
        /**
            The function searches the item \p val in the tree and unlink it from the tree
            if it is found and is equal to \p val.

            Difference between \ref erase and \p unlink functions: \p erase finds <i>a key</i>
            and deletes the item found. \p unlink finds an item by key and deletes it
            only if \p val is an item of the tree, i.e. the pointer to item found
            is equal to <tt> &val </tt>.

            RCU \p synchronize method can be called. RCU should not be locked.

            The \ref disposer specified in \p Traits class template parameter is called
            by garbage collector \p GC asynchronously.

            The function returns \p true if success and \p false otherwise.
        */
        bool unlink( value_type& val )
        {
#       if defined(CDS_CXX11_LAMBDA_SUPPORT) && !(CDS_COMPILER == CDS_COMPILER_MSVC && CDS_COMPILER_VERSION == CDS_COMPILER_MSVC10)
            // vc10 generates an error for the lambda - it sees cds::intrusive::node_traits but not class-defined node_traits
            return erase_( val, node_compare(),
                []( value_type const& v, leaf_node const& n ) -> bool { return &v == node_traits::to_value_ptr( n ); },
                [](value_type const&) {} )  ;
#       else
            return erase_( val, node_compare(), unlink_equal_functor(), empty_erase_functor() )  ;
#       endif
        }

        /// Deletes the item from the tree
        /** \anchor cds_intrusive_EllenBinTree_rcu_erase
            The function searches an item with key equal to \p val in the tree,
            unlinks it from the tree, and returns \p true.
            If the item with key equal to \p val is not found the function return \p false.

            Note the hash functor should accept a parameter of type \p Q that can be not the same as \p value_type.

            RCU \p synchronize method can be called. RCU should not be locked.
        */
        template <typename Q>
        bool erase( const Q& val )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return erase_( val, node_compare(),
                []( Q const&, leaf_node const& ) -> bool { return true; },
                [](value_type const&) {} )  ;
#       else
            return erase_( val, node_compare(), trivial_equal_functor(), empty_erase_functor() )  ;
#       endif
        }

        /// Delete the item from the tree with comparing functor \p pred
        /**
            The function is an analog of \ref cds_intrusive_EllenBinTree_rcu_erase "erase(Q const&)"
            but \p pred predicate is used for key comparing.
            \p Less has the interface like \p std::less and should meet \ref cds_intrusive_EllenBinTree_rcu_less
            "Predicate requirements".
            \p pred must imply the same element order as the comparator used for building the tree.
        */
        template <typename Q, typename Less>
        bool erase_with( const Q& val, Less pred )
        {
            typedef ellen_bintree::details::compare<
                key_type,
                value_type,
                opt::details::make_comparator_from_less<Less>,
                node_traits
            > compare_functor ;

#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return erase_( val, compare_functor(),
                []( Q const&, leaf_node const& ) -> bool { return true; },
                [](value_type const&) {} )  ;
#       else
            return erase_( val, compare_functor(), trivial_equal_functor(), empty_erase_functor() )  ;
#       endif
        }

        /// Deletes the item from the tree
        /** \anchor cds_intrusive_EllenBinTree_rcu_erase_func
            The function searches an item with key equal to \p val in the tree,
            call \p f functor with item found, unlinks it from the tree, and returns \p true.
            The \ref disposer specified in \p Traits class template parameter is called
            by garbage collector \p GC asynchronously.

            The \p Func interface is
            \code
            struct functor {
                void operator()( value_type const& item ) ;
            } ;
            \endcode
            The functor can be passed by reference with <tt>boost:ref</tt>

            If the item with key equal to \p val is not found the function return \p false.

            Note the hash functor should accept a parameter of type \p Q that can be not the same as \p value_type.

            RCU \p synchronize method can be called. RCU should not be locked.
        */
        template <typename Q, typename Func>
        bool erase( Q const& val, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return erase_( val, node_compare(),
                []( Q const&, leaf_node const& ) -> bool { return true; },
                f )  ;
#       else
            return erase_( val, node_compare(), trivial_equal_functor(), f )  ;
#       endif
        }

        /// Delete the item from the tree with comparing functor \p pred
        /**
            The function is an analog of \ref cds_intrusive_EllenBinTree_rcu_erase_func "erase(Q const&, Func)"
            but \p pred predicate is used for key comparing.
            \p Less has the interface like \p std::less and should meet \ref cds_intrusive_EllenBinTree_rcu_less
            "Predicate requirements".
            \p pred must imply the same element order as the comparator used for building the tree.
        */
        template <typename Q, typename Less, typename Func>
        bool erase_with( Q const& val, Less pred, Func f )
        {
            typedef ellen_bintree::details::compare<
                key_type,
                value_type,
                opt::details::make_comparator_from_less<Less>,
                node_traits
            > compare_functor ;

#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return erase_( val, compare_functor(),
                []( Q const&, leaf_node const& ) -> bool { return true; },
                f )  ;
#       else
            return erase_( val, compare_functor(), trivial_equal_functor(), f )  ;
#       endif
        }

        /// Extracts an item with minimal key from the tree
        /**
            The function searches an item with minimal key, unlinks it, and returns pointer to an item found.
            If the tree is empty the function returns \p NULL.

            @note Due the concurrent nature of the tree, the function extracts <i>nearly</i> minimum key.
            It means that the function gets leftmost leaf of the tree and tries to unlink it.
            During unlinking, a concurrent thread may insert an item with key less than leftmost item's key.
            So, the function returns the item with minimum key at the moment of tree traversing.

            RCU \p synchronize method can be called. RCU should not be locked.
            The function does not call the disposer for the item found.
            Before reusing returned pointer you should explicitly call RCU \p synchronize function.
        */
        value_type * extract_min()
        {
            check_deadlock_policy::check() ;

            retired_list updRetire ;
            update_desc * pOp = null_ptr<update_desc *>() ;
            search_result res ;

            {
                rcu_lock l ;
                for ( ;; ) {
                    if ( !search_min( res )) {
                        // Tree is empty
                        if ( pOp )
                            retire_update_desc( pOp, updRetire, false ) ;
                        m_Stat.onExtractMinFailed() ;
                        return null_ptr<value_type *>() ;
                    }

                    if ( res.updGrandParent.bits() != update_desc::Clean )
                        help( res.updGrandParent, updRetire ) ;
                    else if ( res.updParent.bits() != update_desc::Clean )
                        help( res.updParent, updRetire ) ;
                    else {
                        if ( !pOp )
                            pOp = alloc_update_desc()   ;
                        if ( check_delete_precondition( res ) ) {
                            pOp->dInfo.pGrandParent = res.pGrandParent  ;
                            pOp->dInfo.pParent = res.pParent    ;
                            pOp->dInfo.pLeaf = res.pLeaf        ;
                            pOp->dInfo.bDisposeLeaf = false     ;
                            pOp->pUpdate = res.updParent        ;
                            pOp->dInfo.bRightParent = res.bRightParent;
                            pOp->dInfo.bRightLeaf = res.bRightLeaf      ;

                            res.pGrandParent->m_pUpdate.store( update_ptr( pOp, update_desc::DFlag ), memory_model::memory_order_release ) ;

                            if ( help_delete( pOp, updRetire ))
                                break;
                            pOp = null_ptr<update_desc *>() ;
                        }
                    }

                    m_Stat.onExtractMinRetry() ;
                }
            }

            --m_ItemCounter     ;
            m_Stat.onExtractMinSuccess() ;
            return node_traits::to_value_ptr( res.pLeaf ) ;
        }

        /// Extracts an item with maximal key from the tree
        /**
            The function searches an item with maximal key, unlinks it, and returns pointer to an item found.
            If the tree is empty the function returns \p NULL.

            @note Due the concurrent nature of the tree, the function extracts <i>nearly</i> maximal key.
            It means that the function gets rightmost leaf of the tree and tries to unlink it.
            During unlinking, a concurrent thread may insert an item with key great than leftmost item's key.
            So, the function returns the item with maximum key at the moment of tree traversing.

            RCU \p synchronize method can be called. RCU should not be locked.
            The function does not call the disposer for the item found.
            Before reusing returned pointer you should explicitly call RCU \p synchronize function.
        */
        value_type * extract_max()
        {
            check_deadlock_policy::check() ;

            retired_list updRetire ;
            update_desc * pOp = null_ptr<update_desc *>() ;
            search_result res ;

            {
                rcu_lock l ;
                for ( ;; ) {
                    if ( !search_max( res )) {
                        // Tree is empty
                        if ( pOp )
                            retire_update_desc( pOp, updRetire, false ) ;
                        m_Stat.onExtractMaxFailed() ;
                        return null_ptr<value_type *>() ;
                    }

                    if ( res.updGrandParent.bits() != update_desc::Clean )
                        help( res.updGrandParent, updRetire ) ;
                    else if ( res.updParent.bits() != update_desc::Clean )
                        help( res.updParent, updRetire ) ;
                    else {
                        if ( !pOp )
                            pOp = alloc_update_desc() ;
                        if ( check_delete_precondition( res ) ) {
                            pOp->dInfo.pGrandParent = res.pGrandParent  ;
                            pOp->dInfo.pParent = res.pParent    ;
                            pOp->dInfo.pLeaf = res.pLeaf        ;
                            pOp->dInfo.bDisposeLeaf = false     ;
                            pOp->pUpdate = res.updParent        ;
                            pOp->dInfo.bRightParent = res.bRightParent;
                            pOp->dInfo.bRightLeaf = res.bRightLeaf      ;

                            res.pGrandParent->m_pUpdate.store( update_ptr( pOp, update_desc::DFlag ), memory_model::memory_order_release ) ;
                            if ( help_delete( pOp, updRetire ))
                                break;
                            pOp = null_ptr<update_desc *>() ;
                        }
                    }

                    m_Stat.onExtractMaxRetry() ;
                }
            }

            --m_ItemCounter     ;
            m_Stat.onExtractMaxSuccess() ;
            return node_traits::to_value_ptr( res.pLeaf ) ;
        }

        /// Extracts an item from the tree
        /** \anchor cds_intrusive_EllenBinTree_rcu_extract
            The function searches an item with key equal to \p val in the tree,
            unlinks it, and returns pointer to an item found.
            If the item with the key equal to \p val is not found the function returns \p NULL.

            RCU \p synchronize method can be called. RCU should not be locked.
            The function does not call the disposer for the item found.
            Before reusing returned pointer you should explicitly call RCU \p synchronize function.
        */
        template <typename Q>
        value_type * extract( Q const& val )
        {
            return extract_( val, node_compare() ) ;
        }

        /// Extracts an item from the set using \p pred for searching
        /**
            The function is an analog of \ref cds_intrusive_EllenBinTree_rcu_extract "extract(Q const&)"
            but \p cmp is used for key compare.
            \p Less has the interface like \p std::less and should meet \ref cds_intrusive_EllenBinTree_rcu_less
            "Predicate requirements".
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        value_type * extract_with( Q const& val, Less pred )
        {
            typedef ellen_bintree::details::compare<
                key_type,
                value_type,
                opt::details::make_comparator_from_less<Less>,
                node_traits
            > compare_functor ;

            return extract_( val, compare_functor() ) ;
        }


        /// Finds the key \p val
        /** @anchor cds_intrusive_EllenBinTree_rcu_find_val
            The function searches the item with key equal to \p val
            and returns \p true if it is found, and \p false otherwise.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function applies RCU lock internally.
        */
        template <typename Q>
        bool find( Q const& val ) const
        {
            rcu_lock l ;
            search_result    res ;
            if ( search( res, val, node_compare() )) {
                m_Stat.onFindSuccess() ;
                return true ;
            }

            m_Stat.onFindFailed() ;
            return false ;
        }

        /// Finds the key \p val with comparing functor \p pred
        /**
            The function is an analog of \ref cds_intrusive_EllenBinTree_rcu_find_val "find(Q const&)"
            but \p pred is used for key compare.
            \p Less functor has the interface like \p std::less and should meet \ref cds_intrusive_EllenBinTree_rcu_less
            "Predicate requirements".
            \p pred must imply the same element order as the comparator used for building the tree.
            \p pred should accept arguments of type \p Q, \p key_type, \p value_type in any combination.
        */
        template <typename Q, typename Less>
        bool find_with( Q const& val, Less pred ) const
        {
            typedef ellen_bintree::details::compare<
                key_type,
                value_type,
                opt::details::make_comparator_from_less<Less>,
                node_traits
            > compare_functor ;

            rcu_lock l ;
            search_result    res ;
            if ( search( res, val, compare_functor() )) {
                m_Stat.onFindSuccess() ;
                return true ;
            }
            m_Stat.onFindFailed() ;
            return false ;
        }

        /// Finds the key \p val
        /** @anchor cds_intrusive_EllenBinTree_rcu_find_func
            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You can pass \p f argument by value or by reference using <tt>boost::ref</tt> or cds::ref.

            The functor can change non-key fields of \p item. Note that the functor is only guarantee
            that \p item cannot be disposed during functor is executing.
            The functor does not serialize simultaneous access to the tree \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            The \p val argument is non-const since it can be used as \p f functor destination i.e., the functor
            can modify both arguments.

            The function applies RCU lock internally.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q& val, Func f ) const
        {
            return find_( val, f ) ;
        }

        /// Finds the key \p val with comparing functor \p pred
        /**
            The function is an analog of \ref cds_intrusive_EllenBinTree_rcu_find_func "find(Q&, Func)"
            but \p cmp is used for key comparison.
            \p Less functor has the interface like \p std::less and should meet \ref cds_intrusive_EllenBinTree_rcu_less
            "Predicate requirements".
            \p cmp must imply the same element order as the comparator used for building the tree.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q& val, Less pred, Func f ) const
        {
            return find_with_( val, pred, f ) ;
        }

        /// Finds the key \p val
        /** @anchor cds_intrusive_EllenBinTree_rcu_find_cfunc
            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q const& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You can pass \p f argument by value or by reference using <tt>boost::ref</tt> or cds::ref.

            The functor can change non-key fields of \p item. Note that the functor is only guarantee
            that \p item cannot be disposed during functor is executing.
            The functor does not serialize simultaneous access to the tree \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            The function applies RCU lock internally.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q const& val, Func f ) const
        {
            return find_( val, f ) ;
        }

        /// Finds the key \p val with comparing functor \p pred
        /**
            The function is an analog of \ref cds_intrusive_EllenBinTree_rcu_find_cfunc "find(Q const&, Func)"
            but \p cmp is used for key compare.
            \p Less functor has the interface like \p std::less and should meet \ref cds_intrusive_EllenBinTree_rcu_less
            "Predicate requirements".
            \p cmp must imply the same element order as the comparator used for building the tree.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q const& val, Less pred, Func f ) const
        {
            return find_with_( val, pred, f ) ;
        }

        /// Checks if the tree is empty
        bool empty() const
        {
            return m_Root.m_pLeft.load( memory_model::memory_order_relaxed )->is_leaf() ;
        }

        /// Clears the tree (non-atomic)
        /**
            The function unlink all items from the tree.
            The function is not atomic, thus, in multi-threaded environment with parallel insertions
            this sequence
            \code
            set.clear() ;
            assert( set.empty() ) ;
            \endcode
            the assertion could be raised.

            For each leaf the \ref disposer will be called after unlinking.

            RCU \p synchronize method can be called. RCU should not be locked.
        */
        void clear()
        {
            value_type * p ;
            while ( (p = extract_min()) != null_ptr<value_type *>() )
                gc::retire_ptr( p, free_leaf_node ) ;
        }

        /// Returns item count in the tree
        /**
            Only leaf nodes containing user data are counted.

            The value returned depends on item counter type provided by \p Traits template parameter.
            If it is atomicity::empty_item_counter this function always returns 0.
            Therefore, the function is not suitable for checking the tree emptiness, use \ref empty
            member function for this purpose.
        */
        size_t size() const
        {
            return m_ItemCounter    ;
        }

        /// Returns const reference to internal statistics
        stat const& statistics() const
        {
            return m_Stat ;
        }

        /// Checks internal consistency (not atomic, not thread-safe)
        /**
            The debugging function to check internal consistency of the tree.
        */
        bool check_consistency() const
        {
            return check_consistency( &m_Root ) ;
        }

    protected:
        //@cond

        bool check_consistency( internal_node const * pRoot ) const
        {
            tree_node * pLeft  = pRoot->m_pLeft.load( CDS_ATOMIC::memory_order_relaxed ) ;
            tree_node * pRight = pRoot->m_pRight.load( CDS_ATOMIC::memory_order_relaxed );
            assert( pLeft ) ;
            assert( pRight ) ;

            if ( node_compare()( *pLeft, *pRoot ) < 0
                && node_compare()( *pRoot, *pRight ) <= 0
                && node_compare()( *pLeft, *pRight ) < 0 )
            {
                bool bRet = true ;
                if ( pLeft->is_internal() )
                    bRet = check_consistency( static_cast<internal_node *>( pLeft ) ) ;
                assert( bRet )  ;

                if ( bRet && pRight->is_internal() )
                    bRet = bRet && check_consistency( static_cast<internal_node *>( pRight )) ;
                assert( bRet ) ;

                return bRet ;
            }
            return false ;
        }

        void help( update_ptr pUpdate, retired_list& rl )
        {
            if ( pUpdate.all() != update_desc::locked_desc() ) {
                // Helping is disabled since it leads to a strange problems
                // like double deallocation of node. The cause is obscure
                /*
                switch ( pUpdate.bits() ) {
                    case update_desc::IFlag:
                        m_Stat.onHelpInsert() ;
                        help_insert( pUpdate.ptr(), rl ) ;
                        break;
                    case update_desc::DFlag:
                        m_Stat.onHelpDelete() ;
                        help_delete( pUpdate.ptr(), rl ) ;
                        break;
                    case update_desc::Mark:
                        m_Stat.onHelpMark() ;
                        help_marked( pUpdate.ptr(), rl ) ;
                        break;
                }
                */
            }
            else
                m_Stat.onNodeLocked() ;
        }

        void help_insert( update_desc * pOp, retired_list& rl )
        {
            assert( gc::is_locked() ) ;

            tree_node * pLeaf = static_cast<tree_node *>( pOp->iInfo.pLeaf ) ;
            if ( pOp->iInfo.bRightLeaf ) {
                pOp->iInfo.pParent->m_pRight.compare_exchange_strong( pLeaf, static_cast<tree_node *>( pOp->iInfo.pNew ),
                    memory_model::memory_order_relaxed, CDS_ATOMIC::memory_order_relaxed );
            }
            else {
                pOp->iInfo.pParent->m_pLeft.compare_exchange_strong( pLeaf, static_cast<tree_node *>( pOp->iInfo.pNew ),
                    memory_model::memory_order_relaxed, CDS_ATOMIC::memory_order_relaxed );
            }

            update_ptr cur( pOp, update_desc::IFlag ) ;
            if ( pOp->iInfo.pParent->m_pUpdate.compare_exchange_strong( cur, update_ptr( pOp ),
                      memory_model::memory_order_release, CDS_ATOMIC::memory_order_relaxed ))
            {
                retire_update_desc( pOp, rl, false )   ;
            }
        }

        bool check_delete_precondition( search_result& res )
        {
            assert( res.pGrandParent != null_ptr<internal_node *>() ) ;

            if ( res.pGrandParent->m_pUpdate.compare_exchange_strong( res.updGrandParent, update_ptr(update_desc::locked_desc()),
                memory_model::memory_order_acquire, CDS_ATOMIC::memory_order_relaxed ))
            {
                if ( static_cast<internal_node *>( res.bRightParent
                    ? res.pGrandParent->m_pRight.load(memory_model::memory_order_relaxed)
                    : res.pGrandParent->m_pLeft.load(memory_model::memory_order_relaxed) )
                    == res.pParent )
                {
                    if ( res.pParent->m_pUpdate.compare_exchange_strong( res.updParent, update_ptr(update_desc::locked_desc()),
                        memory_model::memory_order_acquire, CDS_ATOMIC::memory_order_relaxed ))
                    {
                        if ( static_cast<leaf_node *>(res.bRightLeaf
                            ? res.pParent->m_pRight.load(memory_model::memory_order_relaxed)
                            : res.pParent->m_pLeft.load(memory_model::memory_order_relaxed) )
                            == res.pLeaf )
                        {
                            return true ;
                        }
                        res.pParent->m_pUpdate.store( res.updParent, memory_model::memory_order_release ) ;
                    }
                }
                res.pGrandParent->m_pUpdate.store( res.updGrandParent, memory_model::memory_order_release ) ;
            }
            return false ;
        }

        bool help_delete( update_desc * pOp, retired_list& rl )
        {
            assert( gc::is_locked() ) ;

            update_ptr pUpdate( update_desc::locked_desc() ) ;
            update_ptr pMark( pOp, update_desc::Mark ) ;
            if ( pOp->dInfo.pParent->m_pUpdate.compare_exchange_strong( pUpdate, pMark,
                    memory_model::memory_order_release, CDS_ATOMIC::memory_order_relaxed )
                || pUpdate == pMark )
            {
                help_marked( pOp, rl ) ;
                return true ;
            }
            else {
                // Undo grandparent dInfo
                update_ptr pDel( pOp, update_desc::DFlag ) ;
                if ( pOp->dInfo.pGrandParent->m_pUpdate.compare_exchange_strong( pDel, update_ptr( pOp ),
                    memory_model::memory_order_release, CDS_ATOMIC::memory_order_relaxed ))
                {
                    retire_update_desc( pOp, rl, false )   ;
                }
                return false ;
            }
        }

        void help_marked( update_desc * pOp, retired_list& rl )
        {
            assert( gc::is_locked() ) ;

            bool bSuccess ;
            tree_node * p = pOp->dInfo.pParent ;
            if ( pOp->dInfo.bRightParent ) {
                bSuccess = pOp->dInfo.pGrandParent->m_pRight.compare_exchange_strong( p,
                    pOp->dInfo.bRightLeaf
                        ? pOp->dInfo.pParent->m_pLeft.load( memory_model::memory_order_relaxed )
                        : pOp->dInfo.pParent->m_pRight.load( memory_model::memory_order_relaxed ),
                    memory_model::memory_order_relaxed, CDS_ATOMIC::memory_order_relaxed ) ;
            }
            else {
                bSuccess = pOp->dInfo.pGrandParent->m_pLeft.compare_exchange_strong( p,
                    pOp->dInfo.bRightLeaf
                        ? pOp->dInfo.pParent->m_pLeft.load( memory_model::memory_order_relaxed )
                        : pOp->dInfo.pParent->m_pRight.load( memory_model::memory_order_relaxed ),
                    memory_model::memory_order_relaxed, CDS_ATOMIC::memory_order_relaxed ) ;
            }

            if ( bSuccess ) {
                // Retire pOp->dInfo.pParent
                retire_node( pOp->dInfo.pParent, rl )   ;

                // For extract operations the leaf should NOT be disposed
                if ( pOp->dInfo.bDisposeLeaf )
                    retire_node( pOp->dInfo.pLeaf, rl   )   ;
            }

            update_ptr upd( pOp, update_desc::DFlag ) ;
            if ( pOp->dInfo.pGrandParent->m_pUpdate.compare_exchange_strong( upd, update_ptr( pOp ),
                memory_model::memory_order_release, CDS_ATOMIC::memory_order_relaxed ))
            {
                //pOp->dInfo.pParent->m_pUpdate.store( update_ptr(pOp), memory_model::memory_order_release ) ;
                retire_update_desc( pOp, rl, false )   ;
            }
        }

        template <typename KeyValue, typename Compare>
        bool search( search_result& res, KeyValue const& key, Compare cmp ) const
        {
            assert( gc::is_locked() ) ;

            internal_node * pParent = null_ptr< internal_node *>() ;
            internal_node * pGrandParent = null_ptr<internal_node *>()   ;
            tree_node *     pLeaf = const_cast<internal_node *>( &m_Root ) ;
            update_ptr      updParent       ;
            update_ptr      updGrandParent  ;
            bool bRightLeaf = false     ;
            bool bRightParent = false   ;

            int nCmp = 0;

            while ( pLeaf->is_internal() ) {
                pGrandParent = pParent          ;
                pParent = static_cast<internal_node *>( pLeaf ) ;
                bRightParent = bRightLeaf       ;
                updGrandParent = updParent      ;
                updParent = pParent->m_pUpdate.load( memory_model::memory_order_acquire ) ;

                nCmp = cmp( key, *pParent ) ;
                bRightLeaf = nCmp >= 0      ;
                pLeaf = nCmp < 0 ? pParent->m_pLeft.load( memory_model::memory_order_acquire )
                                 : pParent->m_pRight.load( memory_model::memory_order_acquire ) ;
            }

            assert( pLeaf->is_leaf() )  ;
            nCmp = cmp( key, *static_cast<leaf_node *>(pLeaf) ) ;

            res.pGrandParent    = pGrandParent  ;
            res.pParent         = pParent       ;
            res.pLeaf           = static_cast<leaf_node *>( pLeaf ) ;
            res.updParent       = updParent     ;
            res.updGrandParent  = updGrandParent;
            res.bRightParent    = bRightParent  ;
            res.bRightLeaf      = bRightLeaf    ;

            return nCmp == 0    ;
        }

        bool search_min( search_result& res ) const
        {
            assert( gc::is_locked() ) ;

            internal_node * pParent = null_ptr< internal_node *>() ;
            internal_node * pGrandParent = null_ptr<internal_node *>()   ;
            tree_node *     pLeaf = const_cast<internal_node *>( &m_Root ) ;
            update_ptr      updParent       ;
            update_ptr      updGrandParent  ;

            while ( pLeaf->is_internal() ) {
                pGrandParent = pParent          ;
                pParent = static_cast<internal_node *>( pLeaf ) ;
                updGrandParent = updParent ;
                updParent = pParent->m_pUpdate.load( memory_model::memory_order_acquire ) ;
                pLeaf = pParent->m_pLeft.load( memory_model::memory_order_acquire ) ;
            }

            if ( pLeaf->infinite_key())
                return false ;

            res.pGrandParent    = pGrandParent  ;
            res.pParent         = pParent       ;
            assert( pLeaf->is_leaf() )  ;
            res.pLeaf           = static_cast<leaf_node *>( pLeaf ) ;
            res.updParent       = updParent     ;
            res.updGrandParent  = updGrandParent;
            res.bRightParent    = false         ;
            res.bRightLeaf      = false         ;

            return true ;
        }

        bool search_max( search_result& res ) const
        {
            assert( gc::is_locked() ) ;

            internal_node * pParent = null_ptr< internal_node *>() ;
            internal_node * pGrandParent = null_ptr<internal_node *>()   ;
            tree_node *     pLeaf = const_cast<internal_node *>( &m_Root ) ;
            update_ptr      updParent       ;
            update_ptr      updGrandParent  ;
            bool bRightLeaf = false     ;
            bool bRightParent = false   ;

            while ( pLeaf->is_internal() ) {
                pGrandParent = pParent          ;
                pParent = static_cast<internal_node *>( pLeaf ) ;
                bRightParent = bRightLeaf       ;
                updGrandParent = updParent      ;
                updParent = pParent->m_pUpdate.load( memory_model::memory_order_acquire ) ;
                if ( pParent->infinite_key()) {
                    pLeaf = pParent->m_pLeft.load( memory_model::memory_order_acquire ) ;
                    bRightLeaf = false ;
                }
                else {
                    pLeaf = pParent->m_pRight.load( memory_model::memory_order_acquire ) ;
                    bRightLeaf = true   ;
                }
            }

            if ( pLeaf->infinite_key())
                return false ;

            res.pGrandParent    = pGrandParent  ;
            res.pParent         = pParent       ;
            assert( pLeaf->is_leaf() )  ;
            res.pLeaf           = static_cast<leaf_node *>( pLeaf ) ;
            res.updParent       = updParent     ;
            res.updGrandParent  = updGrandParent    ;
            res.bRightParent    = bRightParent  ;
            res.bRightLeaf      = bRightLeaf    ;

            return true ;
        }

        template <typename Q, typename Compare, typename Equal, typename Func>
        bool erase_( Q const& val, Compare cmp, Equal eq, Func f )
        {
            check_deadlock_policy::check() ;

            retired_list updRetire ;
            update_desc * pOp = null_ptr<update_desc *>() ;
            search_result res ;

            {
                rcu_lock l ;
                for ( ;; ) {
                    if ( !search( res, val, cmp ) || !eq( val, *res.pLeaf ) ) {
                        if ( pOp )
                            retire_update_desc( pOp, updRetire, false ) ;
                        m_Stat.onEraseFailed() ;
                        return false ;
                    }

                    if ( res.updGrandParent.bits() != update_desc::Clean )
                        help( res.updGrandParent, updRetire ) ;
                    else if ( res.updParent.bits() != update_desc::Clean )
                        help( res.updParent, updRetire ) ;
                    else {
                        if ( !pOp )
                            pOp = alloc_update_desc() ;
                        if ( check_delete_precondition( res ) ) {
                            pOp->dInfo.pGrandParent = res.pGrandParent  ;
                            pOp->dInfo.pParent = res.pParent    ;
                            pOp->dInfo.pLeaf = res.pLeaf        ;
                            pOp->dInfo.bDisposeLeaf = true      ;
                            pOp->pUpdate = res.updParent        ;
                            pOp->dInfo.bRightParent = res.bRightParent  ;
                            pOp->dInfo.bRightLeaf = res.bRightLeaf      ;

                            res.pGrandParent->m_pUpdate.store( update_ptr( pOp, update_desc::DFlag ), memory_model::memory_order_release ) ;

                            if ( help_delete( pOp, updRetire )) {
                                // res.pLeaf is not deleted yet since RCU is blocked
                                cds::unref(f)( *node_traits::to_value_ptr( res.pLeaf )) ;
                                break;
                            }
                            pOp = null_ptr<update_desc *>() ;
                        }
                    }

                    m_Stat.onEraseRetry() ;
                }
            }

            --m_ItemCounter ;
            m_Stat.onEraseSuccess() ;
            return true ;
        }

        template <typename Q, typename Compare>
        value_type * extract_( Q const& val, Compare cmp )
        {
            check_deadlock_policy::check() ;

            retired_list updRetire  ;
            update_desc * pOp = null_ptr<update_desc *>() ;
            search_result res ;

            {
                rcu_lock l ;
                for ( ;; ) {
                    if ( !search( res, val, cmp ) ) {
                        if ( pOp )
                            retire_update_desc( pOp, updRetire, false ) ;
                        m_Stat.onExtractFailed() ;
                        return null_ptr<value_type *>() ;
                    }

                    if ( res.updGrandParent.bits() != update_desc::Clean )
                        help( res.updGrandParent, updRetire ) ;
                    else if ( res.updParent.bits() != update_desc::Clean )
                        help( res.updParent, updRetire ) ;
                    else {
                        if ( !pOp )
                            pOp = alloc_update_desc() ;
                        if ( check_delete_precondition( res )) {
                            pOp->dInfo.pGrandParent = res.pGrandParent  ;
                            pOp->dInfo.pParent = res.pParent    ;
                            pOp->dInfo.pLeaf = res.pLeaf        ;
                            pOp->dInfo.bDisposeLeaf = false     ;
                            pOp->pUpdate = res.updParent        ;
                            pOp->dInfo.bRightParent = res.bRightParent;
                            pOp->dInfo.bRightLeaf = res.bRightLeaf      ;

                            res.pGrandParent->m_pUpdate.store( update_ptr( pOp, update_desc::DFlag), memory_model::memory_order_release ) ;
                            if ( help_delete( pOp, updRetire ))
                                break;
                            pOp = null_ptr<update_desc *>() ;
                        }
                    }

                    m_Stat.onExtractRetry() ;
                }
            }

            --m_ItemCounter ;
            m_Stat.onExtractSuccess() ;
            return node_traits::to_value_ptr( res.pLeaf ) ;
        }

        template <typename Q, typename Less, typename Func>
        bool find_with_( Q& val, Less pred, Func f ) const
        {
            typedef ellen_bintree::details::compare<
                key_type,
                value_type,
                opt::details::make_comparator_from_less<Less>,
                node_traits
            > compare_functor ;

            rcu_lock l ;
            search_result    res ;
            if ( search( res, val, compare_functor() )) {
                assert( res.pLeaf ) ;
                cds::unref(f)( *node_traits::to_value_ptr( res.pLeaf ), val ) ;

                m_Stat.onFindSuccess() ;
                return true ;
            }

            m_Stat.onFindFailed() ;
            return false ;
        }

        template <typename Q, typename Func>
        bool find_( Q& val, Func f ) const
        {
            rcu_lock l ;
            search_result    res ;
            if ( search( res, val, node_compare() )) {
                assert( res.pLeaf ) ;
                cds::unref(f)( *node_traits::to_value_ptr( res.pLeaf ), val ) ;

                m_Stat.onFindSuccess() ;
                return true ;
            }

            m_Stat.onFindFailed() ;
            return false ;
        }

        bool try_insert( value_type& val, internal_node * pNewInternal, search_result& res, retired_list& updRetire )
        {
            assert( gc::is_locked() ) ;

            update_desc * pOp = alloc_update_desc()   ;
            leaf_node * pNewLeaf = node_traits::to_node_ptr( val ) ;

            int nCmp = node_compare()( val, *res.pLeaf ) ;
            if ( nCmp < 0 ) {
                if ( res.pGrandParent ) {
                    pNewInternal->infinite_key( 0 ) ;
                    key_extractor()( pNewInternal->m_Key, *node_traits::to_value_ptr( res.pLeaf ) ) ;
                    assert( !res.pLeaf->infinite_key() ) ;
                }
                else {
                    assert( res.pLeaf->infinite_key() == tree_node::key_infinite1 ) ;
                    pNewInternal->infinite_key( 1 ) ;
                }
                pNewInternal->m_pLeft.store( static_cast<tree_node *>(pNewLeaf), memory_model::memory_order_relaxed ) ;
                pNewInternal->m_pRight.store( static_cast<tree_node *>(res.pLeaf), memory_model::memory_order_release ) ;
            }
            else {
                assert( !res.pLeaf->is_internal() ) ;
                pNewInternal->infinite_key( 0 ) ;

                key_extractor()( pNewInternal->m_Key, val ) ;
                pNewInternal->m_pLeft.store( static_cast<tree_node *>(res.pLeaf), memory_model::memory_order_relaxed ) ;
                pNewInternal->m_pRight.store( static_cast<tree_node *>(pNewLeaf), memory_model::memory_order_release ) ;
                assert( !res.pLeaf->infinite_key()) ;
            }

            pOp->iInfo.pParent = res.pParent    ;
            pOp->iInfo.pNew = pNewInternal      ;
            pOp->iInfo.pLeaf = res.pLeaf        ;
            pOp->iInfo.bRightLeaf = res.bRightLeaf  ;

            assert( res.updParent.bits() == update_desc::Clean ) ;
            if ( res.pParent->m_pUpdate.compare_exchange_strong( res.updParent, update_ptr( update_desc::locked_desc() ),
                memory_model::memory_order_acquire, CDS_ATOMIC::memory_order_relaxed ))
            {
                // check search result
                if ( static_cast<leaf_node *>( pOp->iInfo.bRightLeaf
                    ? pOp->iInfo.pParent->m_pRight.load( memory_model::memory_order_relaxed )
                    : pOp->iInfo.pParent->m_pLeft.load( memory_model::memory_order_relaxed ) ) == res.pLeaf )
                {
                    res.pParent->m_pUpdate.store( update_ptr( pOp, update_desc::IFlag ), memory_model::memory_order_release ) ;

                    help_insert( pOp, updRetire )      ;
                    // pOp has been retired in help_insert
                    return true ;
                }
                res.pParent->m_pUpdate.store( res.updParent, memory_model::memory_order_release ) ;
            }
            retire_update_desc( pOp, updRetire, true )    ;
            return false ;
        }

        //@endcond
    };

}} // namespace cds::intrusive

#endif  // #ifndef __CDS_INTRUSIVE_ELLEN_BINTREE_RCU_H
