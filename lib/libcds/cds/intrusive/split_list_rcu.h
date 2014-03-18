//$$CDS-header$$

#ifndef __CDS_INTRUSIVE_SPLIT_LIST_RCU_H
#define __CDS_INTRUSIVE_SPLIT_LIST_RCU_H

#include <cds/intrusive/split_list_base.h>
#include <cds/details/binary_functor_wrapper.h>

namespace cds { namespace intrusive {

    /// Split-ordered list RCU specialization
    /** @ingroup cds_intrusive_map
        \anchor cds_intrusive_SplitListSet_rcu

        Hash table implementation based on split-ordered list algorithm discovered by Ori Shalev and Nir Shavit, see
        - [2003] Ori Shalev, Nir Shavit "Split-Ordered Lists - Lock-free Resizable Hash Tables"
        - [2008] Nir Shavit "The Art of Multiprocessor Programming"

        The split-ordered list is a lock-free implementation of an extensible unbounded hash table. It uses original
        recursive split-ordering algorithm discovered by Ori Shalev and Nir Shavit that allows to split buckets
        without moving an item on resizing, see \ref cds_SplitList_algo_desc "short algo description".

        <b>Implementation</b>

        Template parameters are:
        - \p RCU - one of \ref cds_urcu_gc "RCU type"
        - \p OrderedList - ordered list implementation used as bucket for hash set, for example, MichaelList, LazyList.
            The intrusive ordered list implementation specifies the type \p T stored in the hash-set, 
            the comparing functor for the type \p T and other features specific for the ordered list.
        - \p Traits - type traits. See split_list::type_traits for explanation.
            Instead of defining \p Traits struct you may use option-based syntax with split_list::make_traits metafunction.

        @note About features of hash functor needed for \p %SplitList see \ref cds_SplitList_hash_functor "SplitList general description".

        \par How to use
        Before including <tt><cds/intrusive/split_list_rcu.h></tt> you should include appropriate RCU header file,
        see \ref cds_urcu_gc "RCU type" for list of existing RCU class and corresponding header files.
        For example, for \ref cds_urcu_general_buffered_gc "general-purpose buffered RCU" and
        MichaelList-based split-list you should include:
        \code
        #include <cds/urcu/general_buffered.h>
        #include <cds/intrusive/michael_list_rcu.h>
        #include <cds/intrusive/split_list_rcu.h>

        // Declare Michael's list for type Foo and default traits:
        typedef cds::intrusive::MichaelList< cds::urcu::gc< cds::urcu::general_buffered<> >, Foo > rcu_michael_list ;

        // Declare split-list based on rcu_michael_list
        typedef cds::intrusive::SplitListSet< cds::urcu::gc< cds::urcu::general_buffered<> >, rcu_michael_list > rcu_split_list;
        \endcode

    */
    template <
        class RCU,
        class OrderedList,
#   ifdef CDS_DOXYGEN_INVOKED
        class Traits = split_list::type_traits
#   else
        class Traits
#   endif
    >
    class SplitListSet< cds::urcu::gc< RCU >, OrderedList, Traits >
    {
    public:
        typedef Traits               options ;   ///< Traits template parameters
        typedef cds::urcu::gc< RCU > gc      ;   ///< RCU garbage collector

        /// Hash functor for \ref value_type and all its derivatives that you use
        typedef typename cds::opt::v::hash_selector< typename options::hash >::type   hash ;

    protected:
        //@cond
        typedef split_list::details::rebind_list_options<OrderedList, options> wrapped_ordered_list ;
        //@endcond

    public:
#   ifdef CDS_DOXYGEN_INVOKED
        typedef OrderedList         ordered_list    ;   ///< type of ordered list used as base for split-list
#   else
        typedef typename wrapped_ordered_list::result    ordered_list   ;
#   endif
        typedef typename ordered_list::value_type       value_type      ;   ///< type of value stored in the split-list
        typedef typename ordered_list::key_comparator   key_comparator  ;   ///< key compare functor
        typedef typename ordered_list::disposer         disposer        ;   ///< Node disposer functor
        typedef typename ordered_list::rcu_lock         rcu_lock        ;   ///< RCU scoped lock
        typedef typename ordered_list::exempt_ptr       exempt_ptr      ;   ///< pointer to extracted node

        typedef typename options::item_counter          item_counter    ;   ///< Item counter type
        typedef typename options::back_off              back_off        ;   ///< back-off strategy for spinning
        typedef typename options::memory_model          memory_model    ;   ///< Memory ordering. See cds::opt::memory_model option

    protected:
        typedef typename ordered_list::node_type    list_node_type      ;   ///< Node type as declared in ordered list
        typedef split_list::node<list_node_type>    node_type           ;   ///< split-list node type
        typedef node_type                           dummy_node_type     ;   ///< dummy node type

        /// Split-list node traits
        /**
            This traits is intended for converting between underlying ordered list node type \ref list_node_type
            and split-list node type \ref node_type
        */
        typedef split_list::node_traits<typename ordered_list::node_traits>  node_traits ;

        //@cond
        /// Bucket table implementation
        typedef typename split_list::details::bucket_table_selector<
            options::dynamic_bucket_table
            , gc
            , dummy_node_type
            , opt::allocator< typename options::allocator >
            , opt::memory_model< memory_model >
        >::type bucket_table    ;

        //@endcond

    protected:
        //@cond
        /// Ordered list wrapper to access protected members of OrderedList
        class ordered_list_wrapper: public ordered_list
        {
            typedef ordered_list base_class ;
            typedef typename base_class::auxiliary_head       bucket_head_type;

        public:
            bool insert_at( dummy_node_type * pHead, value_type& val )
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(pHead) ;
                return base_class::insert_at( h, val )    ;
            }

            template <typename Func>
            bool insert_at( dummy_node_type * pHead, value_type& val, Func f )
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(pHead) ;
                return base_class::insert_at( h, val, f ) ;
            }

            template <typename Func>
            std::pair<bool, bool> ensure_at( dummy_node_type * pHead, value_type& val, Func func )
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(pHead) ;
                return base_class::ensure_at( h, val, func )  ;
            }

            bool unlink_at( dummy_node_type * pHead, value_type& val )
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(pHead) ;
                return base_class::unlink_at( h, val )    ;
            }

            template <typename Q, typename Compare, typename Func>
            bool erase_at( dummy_node_type * pHead, split_list::details::search_value_type<Q> const& val, Compare cmp, Func f )
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(pHead) ;
                return base_class::erase_at( h, val, cmp, f )  ;
            }

            template <typename Q, typename Compare>
            bool erase_at( dummy_node_type * pHead, split_list::details::search_value_type<Q> const& val, Compare cmp )
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(pHead) ;
                return base_class::erase_at( h, val, cmp )  ;
            }

            template <typename Q, typename Compare>
            value_type * extract_at( dummy_node_type * pHead, split_list::details::search_value_type<Q>& val, Compare cmp )
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(pHead) ;
                return base_class::extract_at( h, val, cmp )  ;
            }

            template <typename Q, typename Compare, typename Func>
            bool find_at( dummy_node_type * pHead, split_list::details::search_value_type<Q>& val, Compare cmp, Func f ) const
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(pHead) ;
                return base_class::find_at( h, val, cmp, f )   ;
            }

            template <typename Q, typename Compare>
            bool find_at( dummy_node_type * pHead, split_list::details::search_value_type<Q> const & val, Compare cmp ) const
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(pHead) ;
                return base_class::find_at( h, val, cmp )   ;
            }

            template <typename Q, typename Compare>
            value_type * get_at( dummy_node_type * pHead, split_list::details::search_value_type<Q>& val, Compare cmp ) const
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(pHead) ;
                return base_class::get_at( h, val, cmp )   ;
            }

            bool insert_aux_node( dummy_node_type * pNode )
            {
                return base_class::insert_aux_node( pNode ) ;
            }
            bool insert_aux_node( dummy_node_type * pHead, dummy_node_type * pNode )
            {
                bucket_head_type h(pHead) ;
                return base_class::insert_aux_node( h, pNode ) ;
            }
        };

        template <typename Less>
        struct less_wrapper: public cds::opt::details::make_comparator_from_less<Less>
        {
            typedef cds::opt::details::make_comparator_from_less<Less> base_wrapper ;

            template <typename Q1, typename Q2>
            int operator()( split_list::details::search_value_type<Q1> const& v1, Q2 const& v2 ) const
            {
                return base_wrapper::operator()( v1.val, v2 ) ;
            }

            template <typename Q1, typename Q2>
            int operator()( Q1 const& v1, split_list::details::search_value_type<Q2> const& v2 ) const
            {
                return base_wrapper::operator()( v1, v2.val ) ;
            }
        };
        //@endcond

    protected:
        ordered_list_wrapper    m_List              ;   ///< Ordered list containing split-list items
        bucket_table            m_Buckets           ;   ///< bucket table
        CDS_ATOMIC::atomic<size_t> m_nBucketCountLog2  ;   ///< log2( current bucket count )
        item_counter            m_ItemCounter       ;   ///< Item counter
        hash                    m_HashFunctor       ;   ///< Hash functor

    protected:
        //@cond
        typedef cds::details::Allocator< dummy_node_type, typename options::allocator >   dummy_node_allocator    ;
        static dummy_node_type * alloc_dummy_node( size_t nHash )
        {
            return dummy_node_allocator().New( nHash )  ;
        }
        static void free_dummy_node( dummy_node_type * p )
        {
            dummy_node_allocator().Delete( p )  ;
        }

        /// Calculates hash value of \p key
        template <typename Q>
        size_t hash_value( Q const& key ) const
        {
            return m_HashFunctor( key ) ;
        }

        size_t bucket_no( size_t nHash ) const
        {
            return nHash & ( (1 << m_nBucketCountLog2.load(CDS_ATOMIC::memory_order_relaxed)) - 1 )    ;
        }

        static size_t parent_bucket( size_t nBucket )
        {
            assert( nBucket > 0 )    ;
            return nBucket & ~( 1 << bitop::MSBnz( nBucket ) ) ;
        }

        dummy_node_type * init_bucket( size_t nBucket )
        {
            assert( nBucket > 0 )    ;
            size_t nParent = parent_bucket( nBucket )    ;

            dummy_node_type * pParentBucket = m_Buckets.bucket( nParent )    ;
            if ( pParentBucket == null_ptr<dummy_node_type *>() ) {
                pParentBucket = init_bucket( nParent )    ;
            }

            assert( pParentBucket != null_ptr<dummy_node_type *>() ) ;

            // Allocate a dummy node for new bucket
            {
                dummy_node_type * pBucket = alloc_dummy_node( split_list::dummy_hash( nBucket ) )   ;
                if ( m_List.insert_aux_node( pParentBucket, pBucket ) ) {
                    m_Buckets.bucket( nBucket, pBucket )    ;
                    return pBucket  ;
                }
                free_dummy_node( pBucket )  ;
            }

            // Another thread set the bucket. Wait while it done

            // In this point, we must wait while nBucket is empty.
            // The compiler can decide that waiting loop can be "optimized" (stripped)
            // To prevent this situation, we use waiting on volatile bucket_head_ptr pointer.
            //
            back_off bkoff  ;
            while ( true ) {
                dummy_node_type volatile * p = m_Buckets.bucket( nBucket )    ;
                if ( p != null_ptr<dummy_node_type volatile *>() )
                    return const_cast<dummy_node_type *>( p )   ;
                bkoff() ;
            }
        }

        dummy_node_type * get_bucket( size_t nHash )
        {
            size_t nBucket = bucket_no( nHash )     ;

            dummy_node_type * pHead = m_Buckets.bucket( nBucket )   ;
            if ( pHead == null_ptr<dummy_node_type *>() )
                pHead = init_bucket( nBucket )      ;

            assert( pHead->is_dummy() )   ;

            return pHead    ;
        }

        void init()
        {
            // GC and OrderedList::gc must be the same
            static_assert(( std::is_same<gc, typename ordered_list::gc>::value ), "GC and OrderedList::gc must be the same")  ;

            // atomicity::empty_item_counter is not allowed as a item counter
            static_assert(( !std::is_same<item_counter, atomicity::empty_item_counter>::value ), "atomicity::empty_item_counter is not allowed as a item counter")  ;

            // Initialize bucket 0
            dummy_node_type * pNode = alloc_dummy_node( 0 /*split_list::dummy_hash(0)*/ )   ;

            // insert_aux_node cannot return false for empty list
            CDS_VERIFY( m_List.insert_aux_node( pNode )) ;

            m_Buckets.bucket( 0, pNode )    ;
        }

        void    inc_item_count()
        {
            size_t sz = m_nBucketCountLog2.load(CDS_ATOMIC::memory_order_relaxed)    ;
            if ( ( ++m_ItemCounter >> sz ) > m_Buckets.load_factor() && ((size_t)(1 << sz )) < m_Buckets.capacity() )
            {
                m_nBucketCountLog2.compare_exchange_strong( sz, sz + 1, CDS_ATOMIC::memory_order_seq_cst, CDS_ATOMIC::memory_order_relaxed ) ;
            }
        }

        template <typename Q, typename Compare, typename Func>
        bool find_( Q& val, Compare cmp, Func f )
        {
            size_t nHash = hash_value( val )    ;
            split_list::details::search_value_type<Q>  sv( val, split_list::regular_hash( nHash )) ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return m_List.find_at( pHead, sv, cmp,
                [&f](value_type& item, split_list::details::search_value_type<Q>& val){ cds::unref(f)(item, val.val ); }) ;
#       else
            split_list::details::find_functor_wrapper<Func> ffw( f )   ;
            return m_List.find_at( pHead, sv, cmp, cds::ref(ffw) )  ;
#       endif
        }

        template <typename Q, typename Compare>
        bool find_value( Q const& val, Compare cmp )
        {
            size_t nHash = hash_value( val )    ;
            split_list::details::search_value_type<Q const>  sv( val, split_list::regular_hash( nHash )) ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

            return m_List.find_at( pHead, sv, cmp )  ;
        }

        template <typename Q, typename Compare>
        value_type * get_( Q const& val, Compare cmp )
        {
            size_t nHash = hash_value( val )    ;
            split_list::details::search_value_type<Q const>  sv( val, split_list::regular_hash( nHash )) ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

            return m_List.get_at( pHead, sv, cmp )  ;
        }

        template <typename Q, typename Compare>
        bool extract_( exempt_ptr& dest, Q const& val, Compare cmp )
        {
            size_t nHash = hash_value( val )    ;
            split_list::details::search_value_type<Q const>  sv( val, split_list::regular_hash( nHash )) ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

            dest = m_List.extract_at( pHead, sv, cmp ) ;
            if ( !dest.empty() ) {
                --m_ItemCounter;
                return true;
            }
            return false;
        }

        template <typename Q, typename Compare>
        bool erase_( const Q& val, Compare cmp )
        {
            size_t nHash = hash_value( val )    ;
            split_list::details::search_value_type<Q const>  sv( val, split_list::regular_hash( nHash )) ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

            if ( m_List.erase_at( pHead, sv, cmp ) ) {
                --m_ItemCounter     ;
                return true         ;
            }
            return false    ;
        }

        template <typename Q, typename Compare, typename Func>
        bool erase_( Q const& val, Compare cmp, Func f )
        {
            size_t nHash = hash_value( val )    ;
            split_list::details::search_value_type<Q const>  sv( val, split_list::regular_hash( nHash )) ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

            if ( m_List.erase_at( pHead, sv, cmp, f )) {
                --m_ItemCounter     ;
                return true         ;
            }
            return false    ;
        }

        //@endcond

    public:
        /// Initialize split-ordered list of default capacity
        /**
            The default capacity is defined in bucket table constructor.
            See split_list::expandable_bucket_table, split_list::static_ducket_table
            which selects by split_list::dynamic_bucket_table option.
        */
        SplitListSet()
            : m_nBucketCountLog2(1)
        {
            init()  ;
        }

        /// Initialize split-ordered list
        SplitListSet(
            size_t nItemCount           ///< estimate average of item count
            , size_t nLoadFactor = 1    ///< load factor - average item count per bucket. Small integer up to 8, default is 1.
            )
            : m_Buckets( nItemCount, nLoadFactor )
            , m_nBucketCountLog2(1)
        {
            init()  ;
        }

    public:
        /// Inserts new node
        /**
            The function inserts \p val in the set if it does not contain
            an item with key equal to \p val.

            The function makes RCU lock internally.

            Returns \p true if \p val is placed into the set, \p false otherwise.
        */
        bool insert( value_type& val )
        {
            size_t nHash = hash_value( val )    ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() )             ;

            node_traits::to_node_ptr( val )->m_nHash = split_list::regular_hash( nHash ) ;

            if ( m_List.insert_at( pHead, val )) {
                inc_item_count()  ;
                return true         ;
            }
            return false    ;
        }

        /// Inserts new node
        /**
            This function is intended for derived non-intrusive containers.

            The function allows to split creating of new item into two part:
            - create item with key only
            - insert new item into the set
            - if inserting is success, calls  \p f functor to initialize value-field of \p val.

            The functor signature is:
            \code
                void func( value_type& val ) ;
            \endcode
            where \p val is the item inserted. User-defined functor \p f should guarantee that during changing
            \p val no any other changes could be made on this set's item by concurrent threads.
            The user-defined functor is called only if the inserting is success and may be passed by reference
            using <tt>boost::ref</tt>

            The function makes RCU lock internally.
        */
        template <typename Func>
        bool insert( value_type& val, Func f )
        {
            size_t nHash = hash_value( val )    ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

            node_traits::to_node_ptr( val )->m_nHash = split_list::regular_hash( nHash ) ;

            if ( m_List.insert_at( pHead, val, f )) {
                inc_item_count()  ;
                return true         ;
            }
            return false    ;
        }

        /// Ensures that the \p val exists in the set
        /**
            The operation performs inserting or changing data with lock-free manner.

            If the item \p val is not found in the set, then \p val is inserted into the set.
            Otherwise, the functor \p func is called with item found.
            The functor signature is:
            \code
                void func( bool bNew, value_type& item, value_type& val ) ;
            \endcode
            with arguments:
            - \p bNew - \p true if the item has been inserted, \p false otherwise
            - \p item - item of the set
            - \p val - argument \p val passed into the \p ensure function
            If new item has been inserted (i.e. \p bNew is \p true) then \p item and \p val arguments
            refers to the same thing.

            The functor can change non-key fields of the \p item; however, \p func must guarantee
            that during changing no any other modifications could be made on this item by concurrent threads.

            You can pass \p func argument by value or by reference using <tt>boost::ref</tt> or cds::ref.

            The function makes RCU lock internally.

            Returns std::pair<bool, bool> where \p first is \p true if operation is successfull,
            \p second is \p true if new item has been added or \p false if the item with \p key
            already is in the set.
        */
        template <typename Func>
        std::pair<bool, bool> ensure( value_type& val, Func func )
        {
            size_t nHash = hash_value( val )    ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

            node_traits::to_node_ptr( val )->m_nHash = split_list::regular_hash( nHash ) ;

            std::pair<bool, bool> bRet = m_List.ensure_at( pHead, val, func ) ;
            if ( bRet.first && bRet.second )
                inc_item_count()  ;
            return bRet ;
        }

        /// Unlinks the item \p val from the set
        /**
            The function searches the item \p val in the set and unlinks it from the set
            if it is found and is equal to \p val.

            Difference between \ref erase and \p unlink functions: \p erase finds <i>a key</i>
            and deletes the item found. \p unlink finds an item by key and deletes it
            only if \p val is an item of that set, i.e. the pointer to item found
            is equal to <tt> &val </tt>.

            RCU \p synchronize method can be called, therefore, RCU should not be locked.

            The function returns \p true if success and \p false otherwise.
        */
        bool unlink( value_type& val )
        {
            size_t nHash = hash_value( val )    ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

            if ( m_List.unlink_at( pHead, val ) ) {
                --m_ItemCounter     ;
                return true     ;
            }
            return false    ;
        }

        /// Deletes the item from the set
        /** \anchor cds_intrusive_SplitListSet_rcu_erase
            The function searches an item with key equal to \p val in the set,
            unlinks it from the set, and returns \p true.
            If the item with key equal to \p val is not found the function return \p false.

            Difference between \ref erase and \p unlink functions: \p erase finds <i>a key</i>
            and deletes the item found. \p unlink finds an item by key and deletes it
            only if \p val is an item of that set, i.e. the pointer to item found
            is equal to <tt> &val </tt>.

            RCU \p synchronize method can be called, therefore, RCU should not be locked.

            Note the hash functor should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q>
        bool erase( Q const& val )
        {
            return erase_( val, key_comparator() )  ;
        }

        /// Deletes the item from the set using \p pred for searching
        /**
            The function is an analog of \ref cds_intrusive_SplitListSet_rcu_erase "erase(Q const&)"
            but \p cmp is used for key compare.
            \p Less has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool erase_with( Q const& val, Less pred )
        {
            return erase_( val, typename wrapped_ordered_list::template make_compare_from_less<Less>() )  ;
        }

        /// Deletes the item from the set
        /** \anchor cds_intrusive_SplitListSet_rcu_erase_func
            The function searches an item with key equal to \p val in the set,
            call \p f functor with item found, unlinks it from the set, and returns \p true.
            The \ref disposer specified by \p OrderedList class template parameter is called
            by garbage collector \p GC asynchronously.

            The \p Func interface is
            \code
            struct functor {
                void operator()( value_type const& item ) ;
            } ;
            \endcode
            The functor can be passed by reference with <tt>boost:ref</tt>

            If the item with key equal to \p val is not found the function return \p false.

            RCU \p synchronize method can be called, therefore, RCU should not be locked.

            Note the hash functor should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q, typename Func>
        bool erase( Q const& val, Func f )
        {
            return erase_( val, key_comparator(), f ) ;
        }

        /// Deletes the item from the set using \p pred for searching
        /**
            The function is an analog of \ref cds_intrusive_SplitListSet_rcu_erase_func "erase(Q const&, Func)"
            but \p cmp is used for key compare.
            \p Less has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool erase_with( Q const& val, Less pred, Func f )
        {
            return erase_( val, typename wrapped_ordered_list::template make_compare_from_less<Less>(), f ) ;
        }

        /// Extracts an item from the set
        /** \anchor cds_intrusive_SplitListSet_rcu_extract
            The function searches an item with key equal to \p val in the set,
            unlinks it, and returns pointer to an item found in \p dest argument.
            If the item with the key equal to \p val is not found the function returns \p false.

            @note The function does NOT call RCU read-side lock or synchronization,
            and does NOT dispose the item found. It just excludes the item from the set
            and returns a pointer to item found.
            You should lock RCU before calling of the function, and you should synchronize RCU
            outside the RCU lock before reusing returned pointer.

            \code
            typedef cds::urcu::gc< general_buffered<> > rcu ;
            typedef cds::intrusive::MichaelList< rcu, Foo > rcu_michael_list ;
            typedef cds::intrusive::SplitListSet< rcu, rcu_michael_list, foo_traits > rcu_splitlist_set;

            rcu_splitlist_set theSet ;
            // ...

            rcu_splitlist_set::exempt_ptr p;
            {
                // first, we should lock RCU
                rcu_splitlist_set::rcu_lock lock;

                // Now, you can apply extract function
                // Note that you must not delete the item found inside the RCU lock
                if ( theList.extract( p, 10 )) {
                    // do something with p
                    ...
                }
            }

            // We may safely release p here
            // release() passes the pointer to RCU reclamation cycle:
            // it invokes RCU retire_ptr function with the disposer you provided for rcu_michael_list.
            p.release();
            \endcode
        */
        template <typename Q>
        bool extract( exempt_ptr& dest, Q const& val )
        {
            return extract_( dest, val, key_comparator() ) ;
        }

        /// Extracts an item from the set using \p pred for searching
        /**
            The function is an analog of \ref cds_intrusive_SplitListSet_rcu_extract "extract(exempt_ptr&, Q const&)"
            but \p pred is used for key compare.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool extract_with( exempt_ptr& dest, Q const& val, Less pred )
        {
            return extract_( dest, val, typename wrapped_ordered_list::template make_compare_from_less<Less>() ) ;
        }

        /// Finds the key \p val
        /** \anchor cds_intrusive_SplitListSet_rcu_find_func
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
            The functor does not serialize simultaneous access to the set \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            The \p val argument is non-const since it can be used as \p f functor destination i.e., the functor
            can modify both arguments.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function applies RCU lock internally.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q& val, Func f )
        {
            return find_( val, key_comparator(), f ) ;
        }

        /// Finds the key \p val with \p pred predicate for comparing
        /**
            The function is an analog of \ref cds_intrusive_SplitListSet_rcu_find_func "find(Q&, Func)"
            but \p cmp is used for key compare.
            \p Less has the interface like \p std::less.
            \p cmp must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q& val, Less pred, Func f )
        {
            return find_( val, typename wrapped_ordered_list::template make_compare_from_less<Less>(), f ) ;
        }

        /// Finds the key \p val
        /** \anchor cds_intrusive_SplitListSet_rcu_find_cfunc
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
            The functor does not serialize simultaneous access to the set \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function applies RCU lock internally.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q const& val, Func f )
        {
            return find_( val, key_comparator(), f )  ;
        }

        /// Finds the key \p val with \p pred predicate for comparing
        /**
            The function is an analog of \ref cds_intrusive_SplitListSet_rcu_find_cfunc "find(Q const&, Func)"
            but \p cmp is used for key compare.
            \p Less has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q const& val, Less pred, Func f )
        {
            return find_( val, typename wrapped_ordered_list::template make_compare_from_less<Less>(), f ) ;
        }

        /// Finds the key \p val
        /** \anchor cds_intrusive_SplitListSet_rcu_find_val
            The function searches the item with key equal to \p val
            and returns \p true if \p val found or \p false otherwise.
        */
        template <typename Q>
        bool find( Q const& val )
        {
            return find_value( val, key_comparator() ) ;
        }

        /// Finds the key \p val with \p pred predicate for comparing
        /**
            The function is an analog of \ref cds_intrusive_SplitListSet_rcu_find_val "find(Q const&)"
            but \p cmp is used for key compare.
            \p Less has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool find_with( Q const& val, Less pred )
        {
            return find_value( val, typename wrapped_ordered_list::template make_compare_from_less<Less>() ) ;
        }

        /// Finds the key \p val and return the item found
        /** \anchor cds_intrusive_SplitListSet_rcu_get
            The function searches the item with key equal to \p val and returns the pointer to item found.
            If \p val is not found it returns \p NULL.

            Note the compare functor should accept a parameter of type \p Q that can be not the same as \p value_type.

            RCU should be locked before call of this function.
            Returned item is valid only while RCU is locked:
            \code
            cds::intrusive::SplitListSet< your_template_parameters > theSet ;
            // ...
            {
                // Lock RCU
                hash_set::rcu_lock lock;

                foo * pVal = theSet.get( 5 );
                if ( pVal ) {
                    // Deal with pVal
                    //...
                }
                // Unlock RCU by rcu_lock destructor
                // pVal can be retired by disposer at any time after RCU has been unlocked
            }
            \endcode
        */
        template <typename Q>
        value_type * get( Q const& val )
        {
            return get_( val, key_comparator() );
        }

        /// Finds the key \p val and return the item found
        /**
            The function is an analog of \ref cds_intrusive_SplitListSet_rcu_get "get(Q const&)"
            but \p pred is used for comparing the keys.

            \p Less functor has the semantics like \p std::less but should take arguments of type \ref value_type and \p Q
            in any order.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        value_type * get_with( Q const& val, Less pred )
        {
            return get_( val, typename wrapped_ordered_list::template make_compare_from_less<Less>());
        }


        /// Returns item count in the set
        size_t size() const
        {
            return m_ItemCounter    ;
        }

        /// Checks if the set is empty
        /**
            Emptiness is checked by item counting: if item count is zero then the set is empty.
            Thus, the correct item counting feature is an important part of split-list set implementation.
        */
        bool empty() const
        {
            return size() == 0  ;
        }

        /// Clears the set (non-atomic)
        /**
            The function unlink all items from the set.
            The function is not atomic. Therefore, \p clear may be used only for debugging purposes.
        */
        void clear()
        {
            iterator it = begin()   ;
            while ( it != end() ) {
                iterator i(it) ;
                ++i ;
                unlink( *it )   ;
                it = i          ;
            }
        }

    protected:
        //@cond
        template <bool IsConst>
        class iterator_type
            :public split_list::details::iterator_type<node_traits, ordered_list, IsConst>
        {
            typedef split_list::details::iterator_type<node_traits, ordered_list, IsConst> iterator_base_class ;
            typedef typename iterator_base_class::list_iterator list_iterator ;
        public:
            iterator_type()
                : iterator_base_class()
            {}

            iterator_type( iterator_type const& src )
                : iterator_base_class( src )
            {}

            // This ctor should be protected...
            iterator_type( list_iterator itCur, list_iterator itEnd )
                : iterator_base_class( itCur, itEnd )
            {}
        };
        //@endcond
    public:
        /// Forward iterator
        /**
            The forward iterator for a split-list has some features:
            - it has no post-increment operator
            - it depends on iterator of underlying \p OrderedList
            - The iterator cannot be moved across thread boundary since it may contain GC's guard that is thread-private GC data.
            - Iterator ensures thread-safety even if you delete the item that iterator points to. However, in case of concurrent
              deleting operations it is no guarantee that you iterate all item in the split-list.

            Therefore, the use of iterators in concurrent environment is not good idea. Use the iterator on the concurrent container
            for debug purpose only.
        */
        typedef iterator_type<false>    iterator        ;
        /// Const forward iterator
        /**
            For iterator's features and requirements see \ref iterator
        */
        typedef iterator_type<true>     const_iterator  ;

        /// Returns a forward iterator addressing the first element in a split-list
        /**
            For empty list \code begin() == end() \endcode
        */
        iterator begin()
        {
            return iterator( m_List.begin(), m_List.end() ) ;
        }

        /// Returns an iterator that addresses the location succeeding the last element in a split-list
        /**
            Do not use the value returned by <tt>end</tt> function to access any item.

            The returned value can be used only to control reaching the end of the split-list.
            For empty list \code begin() == end() \endcode
        */
        iterator end()
        {
            return iterator( m_List.end(), m_List.end() ) ;
        }

        /// Returns a forward const iterator addressing the first element in a split-list
        const_iterator begin() const
        {
            return const_iterator( m_List.begin(), m_List.end() ) ;
        }

        /// Returns an const iterator that addresses the location succeeding the last element in a split-list
        const_iterator end() const
        {
            return const_iterator( m_List.end(), m_List.end() ) ;
        }

    };

}}  // namespace cds::intrusive

#endif // #ifndef __CDS_INTRUSIVE_SPLIT_LIST_RCU_H
