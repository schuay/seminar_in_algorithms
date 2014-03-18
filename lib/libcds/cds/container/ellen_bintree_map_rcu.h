//$$CDS-header$$

#ifndef __CDS_CONTAINER_ELLEN_BINTREE_MAP_RCU_H
#define __CDS_CONTAINER_ELLEN_BINTREE_MAP_RCU_H

#include <cds/container/ellen_bintree_base.h>
#include <cds/intrusive/ellen_bintree_rcu.h>
#include <cds/details/functor_wrapper.h>

namespace cds { namespace container {

    /// Map based on Ellen's et al binary search tree (RCU specialization)
    /** @ingroup cds_nonintrusive_map
        @ingroup cds_nonintrusive_tree
        @anchor cds_container_EllenBinTreeMap_rcu

        Source:
            - [2010] F.Ellen, P.Fatourou, E.Ruppert, F.van Breugel "Non-blocking Binary Search Tree"

        %EllenBinTreeMap is an unbalanced leaf-oriented binary search tree that implements the <i>map</i>
        abstract data type. Nodes maintains child pointers but not parent pointers.
        Every internal node has exactly two children, and all data of type <tt>std::pair<Key const, T></tt>
        currently in the tree are stored in the leaves. Internal nodes of the tree are used to direct \p find
        operation along the path to the correct leaf. The keys (of \p Key type) stored in internal nodes
        may or may not be in the map.
        Unlike \ref cds_container_EllenBinTreeSet_rcu "EllenBinTreeSet" keys are not a part of \p T type.
        The map can be represented as a set containing <tt>std::pair< Key const, T> </tt> values.

        Due to \p extract_min and \p extract_max member functions the \p %EllenBinTreeMap can act as
        a <i>priority queue</i>. In this case you should provide unique compound key, for example,
        the priority value plus some uniformly distributed random value.

        @warning Recall the tree is <b>unbalanced</b>. The complexity of operations is <tt>O(log N)</tt>
        for uniformly distributed random keys, but in worst case the complexity is <tt>O(N)</tt>.

        @note In the current implementation we do not use helping technique described in original paper.
        So, the current implementation is near to fine-grained lock-based tree.
        Helping will be implemented in future release

        <b>Template arguments</b> :
        - \p RCU - one of \ref cds_urcu_gc "RCU type"
        - \p Key - key type
        - \p T - value type to be stored in tree's leaf nodes.
        - \p Traits - type traits. See ellen_bintree::type_traits for explanation.

        It is possible to declare option-based tree with ellen_bintree::make_map_traits metafunction
        instead of \p Traits template argument.
        Template argument list \p Options of ellen_bintree::make_map_traits metafunction are:
        - opt::compare - key compare functor. No default functor is provided.
            If the option is not specified, \p %opt::less is used.
        - opt::less - specifies binary predicate used for key compare. At least \p %opt::compare or \p %opt::less should be defined.
        - opt::item_counter - the type of item counting feature. Default is \ref atomicity::empty_item_counter that is no item counting.
        - opt::memory_model - C++ memory ordering model. Can be opt::v::relaxed_ordering (relaxed memory model, the default)
            or opt::v::sequential_consistent (sequentially consisnent memory model).
        - opt::allocator - the allocator used for \ref ellen_bintree::map_node "leaf nodes" which contains data.
            Default is \ref CDS_DEFAULT_ALLOCATOR.
        - opt::node_allocator - the allocator used for \ref ellen_bintree::internal_node "internal nodes".
            Default is \ref CDS_DEFAULT_ALLOCATOR.
        - ellen_bintree::update_desc_allocator - an allocator of \ref ellen_bintree::update_desc "update descriptors",
            default is \ref CDS_DEFAULT_ALLOCATOR.
            Note that update descriptor is helping data structure with short lifetime and it is good candidate for pooling.
            The number of simultaneously existing descriptors is a relatively small number limited the number of threads
            working with the tree and RCU buffer size.
            Therefore, a bounded lock-free container like \p cds::container::VyukovMPMCCycleQueue is good choice for the free-list
            of update descriptors, see cds::memory::vyukov_queue_pool free-list implementation.
            Also notice that size of update descriptor is not dependent on the type of data
            stored in the tree so single free-list object can be used for several EllenBinTree-based object.
        - opt::stat - internal statistics. Available types: ellen_bintree::stat, ellen_bintree::empty_stat (the default)
        - opt::rcu_check_deadlock - a deadlock checking policy. Default is opt::v::rcu_throw_deadlock
        - opt::copy_policy - key copy policy defines a functor to copy leaf node's key to internal node.
            By default, assignment operator is used.
            The copy functor interface is:
            \code
            struct copy_functor {
                void operator()( Key& dest, Key const& src ) ;
            };
            \endcode

        @note Before including <tt><cds/container/ellen_bintree_map_rcu.h></tt> you should include appropriate RCU header file,
        see \ref cds_urcu_gc "RCU type" for list of existing RCU class and corresponding header files.
    */
    template <
        class RCU,
        typename Key,
        typename T,
#ifdef CDS_DOXYGEN_INVOKED
        class Traits = ellen_bintree::type_traits
#else
        class Traits
#endif
    >
    class EllenBinTreeMap< cds::urcu::gc<RCU>, Key, T, Traits >
#ifdef CDS_DOXYGEN_INVOKED
        : public cds::intrusive::EllenBinTree< cds::urcu::gc<RCU>, Key, T, Traits >
#else
        : public ellen_bintree::details::make_ellen_bintree_map< cds::urcu::gc<RCU>, Key, T, Traits >::type
#endif
    {
        //@cond
        typedef ellen_bintree::details::make_ellen_bintree_map< cds::urcu::gc<RCU>, Key, T, Traits > maker ;
        typedef typename maker::type base_class ;
        //@endcond
    public:
        typedef cds::urcu::gc<RCU>  gc  ;   ///< RCU Garbage collector
        typedef Key     key_type        ;   ///< type of a key stored in the map
        typedef T       mapped_type      ;  ///< type of value stored in the map
        typedef std::pair< key_type const, mapped_type >    value_type  ;   ///< Key-value pair stored in leaf node of the mp
        typedef Traits  options         ;   ///< Traits template parameter

#   ifdef CDS_DOXYGEN_INVOKED
        typedef implementation_defined key_comparator  ;    ///< key compare functor based on opt::compare and opt::less option setter.
#   else
        typedef typename maker::intrusive_type_traits::compare   key_comparator  ;
#   endif
        typedef typename base_class::item_counter           item_counter        ; ///< Item counting policy used
        typedef typename base_class::memory_model           memory_model        ; ///< Memory ordering. See cds::opt::memory_model option
        typedef typename base_class::node_allocator         node_allocator_type ; ///< allocator for maintaining internal node
        typedef typename base_class::stat                   stat                ; ///< internal statistics type
        typedef typename base_class::rcu_check_deadlock     rcu_check_deadlock  ; ///< Deadlock checking policy
        typedef typename options::copy_policy               copy_policy         ; ///< key copy policy

        typedef typename options::allocator                 allocator_type      ;   ///< Allocator for leaf nodes
        typedef typename base_class::node_allocator         node_allocator      ;   ///< Internal node allocator
        typedef typename base_class::update_desc_allocator  update_desc_allocator ; ///< Update descriptor allocator

    protected:
        //@cond
        typedef typename base_class::value_type         leaf_node       ;
        typedef typename base_class::internal_node      internal_node   ;
        typedef typename base_class::update_desc        update_desc     ;

        typedef typename maker::cxx_leaf_node_allocator cxx_leaf_node_allocator ;

        typedef std::unique_ptr< leaf_node, typename maker::leaf_deallocator >    scoped_node_ptr ;
        //@endcond

    protected:
        //@cond
#   ifndef CDS_CXX11_LAMBDA_SUPPORT
        struct empty_insert_functor
        {
            void operator()( value_type& ) const
            {}
        };

        template <typename Q>
        class insert_value_functor
        {
            Q const&    m_val   ;
        public:
            insert_value_functor( Q const& v)
                : m_val(v)
            {}

            void operator()( value_type& item )
            {
                item.second = m_val ;
            }
        };

        template <typename Func>
        class insert_key_wrapper: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            insert_key_wrapper( Func f ): base_class(f) {}

            void operator()( leaf_node& item )
            {
                base_class::get()( item.m_Value ) ;
            }
        };

        template <typename Func>
        class ensure_wrapper: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            ensure_wrapper( Func f) : base_class(f) {}

            void operator()( bool bNew, leaf_node& item, leaf_node const& )
            {
                base_class::get()( bNew, item.m_Value ) ;
            }
        };

        template <typename Func>
        struct erase_functor
        {
            Func        m_func  ;

            erase_functor( Func f )
                : m_func(f)
            {}

            void operator()( leaf_node& node )
            {
                cds::unref(m_func)( node.m_Value )  ;
            }
        };

        template <typename Func>
        class find_wrapper: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            find_wrapper( Func f )
                : base_class(f)
            {}

            template <typename Q>
            void operator()( leaf_node& item, Q& val )
            {
                base_class::get()( item.m_Value, val ) ;
            }
        };
#   endif
        //@endcond

    public:
        /// Default constructor
        EllenBinTreeMap()
            : base_class()
        {}

        /// Clears the map
        ~EllenBinTreeMap()
        {}

        /// Inserts new node with key and default value
        /**
            The function creates a node with \p key and default value, and then inserts the node created into the map.

            Preconditions:
            - The \ref key_type should be constructible from a value of type \p K.
                In trivial case, \p K is equal to \ref key_type.
            - The \ref mapped_type should be default-constructible.

            RCU \p synchronize method can be called. RCU should not be locked.

            Returns \p true if inserting successful, \p false otherwise.
        */
        template <typename K>
        bool insert( K const& key )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return insert_key( key, [](value_type&){} )    ;
#       else
            return insert_key( key, empty_insert_functor() )   ;
#       endif
        }

        /// Inserts new node
        /**
            The function creates a node with copy of \p val value
            and then inserts the node created into the map.

            Preconditions:
            - The \ref key_type should be constructible from \p key of type \p K.
            - The \ref value_type should be constructible from \p val of type \p V.

            RCU \p synchronize method can be called. RCU should not be locked.

            Returns \p true if \p val is inserted into the map, \p false otherwise.
        */
        template <typename K, typename V>
        bool insert( K const& key, V const& val )
        {
            scoped_node_ptr pNode( cxx_leaf_node_allocator().New( key, val )) ;
            if ( base_class::insert( *pNode ))
            {
                pNode.release() ;
                return true     ;
            }
            return false ;
        }

        /// Inserts new node and initialize it by a functor
        /**
            This function inserts new node with key \p key and if inserting is successful then it calls
            \p func functor with signature
            \code
                struct functor {
                    void operator()( value_type& item )  ;
                };
            \endcode

            The argument \p item of user-defined functor \p func is the reference
            to the map's item inserted:
                - <tt>item.first</tt> is a const reference to item's key that cannot be changed.
                - <tt>item.second</tt> is a reference to item's value that may be changed.

            The user-defined functor can be passed by reference using <tt>boost::ref</tt>
            and it is called only if inserting is successful.

            The key_type should be constructible from value of type \p K.

            The function allows to split creating of new item into two part:
            - create item from \p key;
            - insert new item into the map;
            - if inserting is successful, initialize the value of item by calling \p func functor

            This can be useful if complete initialization of object of \p value_type is heavyweight and
            it is preferable that the initialization should be completed only if inserting is successful.

            RCU \p synchronize method can be called. RCU should not be locked.
        */
        template <typename K, typename Func>
        bool insert_key( const K& key, Func func )
        {
            scoped_node_ptr pNode( cxx_leaf_node_allocator().New( key )) ;
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            if ( base_class::insert( *pNode, [&func]( leaf_node& item ) { cds::unref(func)( item.m_Value ); } ))
#       else
            insert_key_wrapper<Func> wrapper(func) ;
            if ( base_class::insert( *pNode, cds::ref(wrapper) ))
#endif
            {
                pNode.release() ;
                return true     ;
            }
            return false ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        /// For key \p key inserts data of type \ref value_type constructed with <tt>std::forward<Args>(args)...</tt>
        /**
            Returns \p true if inserting successful, \p false otherwise.

            RCU \p synchronize method can be called. RCU should not be locked.

            @note This function is available only for compiler that supports
            variadic template and move semantics
        */
        template <typename K, typename... Args>
        bool emplace( K&& key, Args&&... args )
        {
            scoped_node_ptr pNode( cxx_leaf_node_allocator().New( std::forward<K>(key), std::forward<Args>(args)... )) ;
            if ( base_class::insert( *pNode )) {
                pNode.release() ;
                return true     ;
            }
            return false ;
        }
#   endif

        /// Ensures that the \p key exists in the map
        /**
            The operation performs inserting or changing data with lock-free manner.

            If the \p key not found in the map, then the new item created from \p key
            is inserted into the map (note that in this case the \ref key_type should be
            constructible from type \p K).
            Otherwise, the functor \p func is called with item found.
            The functor \p Func may be a function with signature:
            \code
                void func( bool bNew, value_type& item ) ;
            \endcode
            or a functor:
            \code
                struct my_functor {
                    void operator()( bool bNew, value_type& item ) ;
                };
            \endcode

            with arguments:
            - \p bNew - \p true if the item has been inserted, \p false otherwise
            - \p item - item of the list

            The functor may change any fields of the \p item.second that is \ref value_type.

            You may pass \p func argument by reference using <tt>boost::ref</tt>.

            RCU \p synchronize method can be called. RCU should not be locked.

            Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
            \p second is true if new item has been added or \p false if the item with \p key
            already is in the list.
        */
        template <typename K, typename Func>
        std::pair<bool, bool> ensure( K const& key, Func func )
        {
            scoped_node_ptr pNode( cxx_leaf_node_allocator().New( key )) ;
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            std::pair<bool, bool> res = base_class::ensure( *pNode,
                [&func](bool bNew, leaf_node& item, leaf_node const& ){ cds::unref(func)( bNew, item.m_Value ); }
            ) ;
#       else
            ensure_wrapper<Func> wrapper( func )   ;
            std::pair<bool, bool> res = base_class::ensure( *pNode, cds::ref(wrapper) ) ;
#       endif
            if ( res.first && res.second )
                pNode.release() ;
            return res ;
        }

        /// Delete \p key from the map
        /**\anchor cds_nonintrusive_EllenBinTreeMap_rcu_erase_val

            RCU \p synchronize method can be called. RCU should not be locked.

            Return \p true if \p key is found and deleted, \p false otherwise
        */
        template <typename K>
        bool erase( K const& key )
        {
            return base_class::erase(key) ;
        }

        /// Deletes the item from the map using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_EllenBinTreeMap_rcu_erase_val "erase(K const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less>
        bool erase_with( K const& key, Less pred )
        {
            return base_class::erase_with( key, cds::details::predicate_wrapper< leaf_node, Less, typename maker::key_accessor >()) ;
        }

        /// Delete \p key from the map
        /** \anchor cds_nonintrusive_EllenBinTreeMap_rcu_erase_func

            The function searches an item with key \p key, calls \p f functor
            and deletes the item. If \p key is not found, the functor is not called.

            The functor \p Func interface:
            \code
            struct extractor {
                void operator()(value_type& item) { ... }
            };
            \endcode
            The functor may be passed by reference using <tt>boost:ref</tt>

            RCU \p synchronize method can be called. RCU should not be locked.

            Return \p true if key is found and deleted, \p false otherwise
        */
        template <typename K, typename Func>
        bool erase( K const& key, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::erase( key, [&f]( leaf_node& node) { cds::unref(f)( node.m_Value ); } ) ;
#       else
            erase_functor<Func> wrapper(f) ;
            return base_class::erase( key, cds::ref(wrapper)) ;
#       endif
        }

        /// Deletes the item from the map using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_EllenBinTreeMap_rcu_erase_func "erase(K const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less, typename Func>
        bool erase_with( K const& key, Less pred, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::erase_with( key, cds::details::predicate_wrapper< leaf_node, Less, typename maker::key_accessor >(),
                [&f]( leaf_node& node) { cds::unref(f)( node.m_Value ); } ) ;
#       else
            erase_functor<Func> wrapper(f) ;
            return base_class::erase_with( key, cds::details::predicate_wrapper< leaf_node, Less, typename maker::key_accessor >(), cds::ref(wrapper)) ;
#       endif
        }

        /// Extracts an item with minimal key from the map
        /**
            If the map is not empty, the function returns \p true, \p dest contains the copy of minimum key/value
            pair (assignment operator is called for \p value_type).
            If the map is empty, the function returns \p false, \p dest is left unchanged.

            @note Due the concurrent nature of the map, the function extracts <i>nearly</i> minimum key.
            It means that the function gets leftmost leaf of the tree and tries to unlink it.
            During unlinking, a concurrent thread may insert an item with key less than leftmost item's key.
            So, the function returns the item with minimum key at the moment of tree traversing.

            RCU \p synchronize method can be called. RCU should not be locked.
        */
        bool extract_min( std::pair<key_type, mapped_type>& dest )
        {
            leaf_node * pFound = base_class::extract_min() ;
            if ( !pFound )
                return false ;

            dest = pFound->m_Value;
            gc::retire_ptr( pFound, base_class::free_leaf_node ) ;

            return true ;
        }

        /// Extracts an item with minimal key from the map
        /**
            If the map is not empty, the function returns \p true, \p f functor is called with value found.
            If the map is empty, the function returns \p false, \p f functor is not called, \p dest is left unchanged.

            The functor \p Func interface is:
            \code
            struct extract_functor {
                void operator()( Q& dest, value_type& valueFound ) ;
            };
            \endcode
            The functor should copy interesting part of value found into \p dest.
            \p f can be passed by value or by reference using \p boost::ref or cds::ref.

            @note Due the concurrent nature of the map, the function extracts <i>nearly</i> minimum key.
            It means that the function gets leftmost leaf of the tree and tries to unlink it.
            During unlinking, a concurrent thread may insert an item with key less than leftmost item's key.
            So, the function returns the item with minimum key at the moment of tree traversing.

            RCU \p synchronize method can be called. RCU should not be locked.
        */
        template <typename Q, typename Func>
        bool extract_min( Q& dest, Func f )
        {
            leaf_node * pFound = base_class::extract_min() ;
            if ( !pFound )
                return false ;

            cds::unref(f)( dest, pFound->m_Value ) ;
            gc::retire_ptr( pFound, base_class::free_leaf_node ) ;

            return true ;
        }

        /// Extracts an item with maximal key from the map
        /**
            If the map is not empty, the function returns \p true, \p dest contains the copy of maximum key/value pair
            (assignment operator is called for \p value_type).
            If the map is empty, the function returns \p false, \p dest is left unchanged.

            @note Due the concurrent nature of the map, the function extracts <i>nearly</i> maximal key.
            It means that the function gets rightmost leaf of the tree and tries to unlink it.
            During unlinking, a concurrent thread may insert an item with key great than leftmost item's key.
            So, the function returns the item with maximum key at the moment of tree traversing.

            RCU \p synchronize method can be called. RCU should not be locked.
        */
        bool extract_max( std::pair<key_type, mapped_type>& dest)
        {
            leaf_node * pFound = base_class::extract_max() ;
            if ( !pFound )
                return false ;

            dest = pFound->m_Value ;
            gc::retire_ptr( pFound, base_class::free_leaf_node ) ;

            return true ;
        }

        /// Extracts an item with maximal key from the map
        /**
            If the map is not empty, the function returns \p true, \p f functor is called with value found.
            If the map is empty, the function returns \p false, \p f functor is not called, \p dest is left unchanged.

            The functor \p Func interface is:
            \code
            struct extract_functor {
                void operator()( Q& dest, value_type& valueFound ) ;
            };
            \endcode
            The functor should copy interesting part of value found into \p dest.
            \p f can be passed by value or by reference using \p boost::ref or cds::ref.

            @note Due the concurrent nature of the map, the function extracts <i>nearly</i> maximal key.
            It means that the function gets rightmost leaf of the tree and tries to unlink it.
            During unlinking, a concurrent thread may insert an item with key great than leftmost item's key.
            So, the function returns the item with maximum key at the moment of tree traversing.

            RCU \p synchronize method can be called. RCU should not be locked.
        */
        template <typename Q, typename Func>
        bool extract_max( Q& dest, Func f )
        {
            leaf_node * pFound = base_class::extract_max() ;
            if ( !pFound )
                return false ;

            cds::unref(f)( dest, pFound->m_Value ) ;
            gc::retire_ptr( pFound, base_class::free_leaf_node ) ;

            return true ;
        }

        /// Find the key \p key
        /** \anchor cds_nonintrusive_EllenBinTreeMap_rcu_find_cfunc

            The function searches the item with key equal to \p key and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item ) ;
            };
            \endcode
            where \p item is the item found.

            You can pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change \p item.second.

            The function applies RCU lock internally.

            The function returns \p true if \p key is found, \p false otherwise.
        */
        template <typename K, typename Func>
        bool find( K const& key, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find( key, [&f](leaf_node& item, K const& ) { cds::unref(f)( item.m_Value );}) ;
#       else
            find_wrapper<Func> wrapper(f)       ;
            return base_class::find( key, cds::ref(wrapper) )   ;
#       endif
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_EllenBinTreeMap_rcu_find_cfunc "find(K const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less, typename Func>
        bool find_with( K const& key, Less pred, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find_with( key, cds::details::predicate_wrapper< leaf_node, Less, typename maker::key_accessor >(),
                [&f](leaf_node& item, K const& ) { cds::unref(f)( item.m_Value );}) ;
#       else
            find_wrapper<Func> wrapper(f)       ;
            return base_class::find_with( key, cds::details::predicate_wrapper< leaf_node, Less, typename maker::key_accessor >(), cds::ref(wrapper) )   ;
#       endif
        }

        /// Find the key \p key
        /** \anchor cds_nonintrusive_EllenBinTreeMap_rcu_find_val

            The function searches the item with key equal to \p key
            and returns \p true if it is found, and \p false otherwise.

            The function applies RCU lock internally.
        */
        template <typename K>
        bool find( K const& key )
        {
            return base_class::find( key ) ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_EllenBinTreeMap_rcu_find_val "find(K const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less>
        bool find_with( K const& key, Less pred )
        {
            return base_class::find_with( key, cds::details::predicate_wrapper< leaf_node, Less, typename maker::key_accessor >() ) ;
        }

        /// Clears the map
        void clear()
        {
            base_class::clear() ;
        }

        /// Checks if the map is empty
        /**
            Emptiness is checked by item counting: if item count is zero then the map is empty.
        */
        bool empty() const
        {
            return base_class::empty() ;
        }

        /// Returns item count in the map
        size_t size() const
        {
            return base_class::size()    ;
        }

        /// Returns const reference to internal statistics
        stat const& statistics() const
        {
            return base_class::statistics() ;
        }

        /// Checks internal consistency (not atomic, not thread-safe)
        /**
            The debugging function to check internal consistency of the tree.
        */
        bool check_consistency() const
        {
            return base_class::check_consistency() ;
        }

    };
}} // namespace cds::container

#endif //#ifndef __CDS_CONTAINER_ELLEN_BINTREE_MAP_RCU_H
