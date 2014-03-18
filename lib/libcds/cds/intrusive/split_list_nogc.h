//$$CDS-header$$

#ifndef __CDS_INTRUSIVE_SPLIT_LIST_NOGC_H
#define __CDS_INTRUSIVE_SPLIT_LIST_NOGC_H

#include <cds/intrusive/split_list_base.h>
#include <cds/gc/nogc.h>

namespace cds { namespace intrusive {

    /// Split-ordered list (template specialization for gc::nogc)
    /** @ingroup cds_intrusive_map
        \anchor cds_intrusive_SplitListSet_nogc

        This specialization is intended for so-called persistent usage when no item
        reclamation may be performed. The class does not support deleting of list item.

        See \ref cds_intrusive_SplitListSet_hp "SplitListSet" for description of template parameters.
        The template parameter \p OrderedList should be any gc::nogc-derived ordered list, for example,
        \ref cds_intrusive_MichaelList_nogc "persistent MichaelList",
        \ref cds_intrusive_LazyList_nogc "persistent LazyList"

        The interface of the specialization is a slightly different.
    */
    template <
        class OrderedList,
#ifdef CDS_DOXYGEN_INVOKED
        class Traits = split_list::type_traits
#else
        class Traits
#endif
    >
    class SplitListSet< gc::nogc, OrderedList, Traits >
    {
    public:
        typedef Traits          options ;   ///< Traits template parameters
        typedef gc::nogc        gc      ;   ///< Garbage collector

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
        typedef typename wrapped_ordered_list::result    ordered_list    ;
#   endif
        typedef typename ordered_list::value_type   value_type  ;   ///< type of value stored in the split-list
        typedef typename ordered_list::key_comparator    key_comparator  ;   ///< key comparison functor
        typedef typename ordered_list::disposer disposer        ;   ///< Node disposer functor

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

        typedef typename ordered_list::iterator list_iterator   ;
        typedef typename ordered_list::const_iterator list_const_iterator   ;
        //@endcond

    protected:
        //@cond
        /// Ordered list wrapper to access protected members
        class ordered_list_wrapper: public ordered_list
        {
            typedef ordered_list base_class ;
            typedef typename base_class::auxiliary_head       bucket_head_type;

        public:
            list_iterator insert_at_( dummy_node_type * pHead, value_type& val )
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(static_cast<list_node_type *>(pHead)) ;
                return base_class::insert_at_( h, val )    ;
            }

            template <typename Func>
            std::pair<list_iterator, bool> ensure_at_( dummy_node_type * pHead, value_type& val, Func func )
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(static_cast<list_node_type *>(pHead)) ;
                return base_class::ensure_at_( h, val, func )  ;
            }

            template <typename Q, typename Compare, typename Func>
            bool find_at( dummy_node_type * pHead, split_list::details::search_value_type<Q>& val, Compare cmp, Func f )
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(static_cast<list_node_type *>(pHead)) ;
                return base_class::find_at( h, val, cmp, f )   ;
            }

            template <typename Q, typename Compare>
            list_iterator find_at_( dummy_node_type * pHead, split_list::details::search_value_type<Q> const & val, Compare cmp )
            {
                assert( pHead != null_ptr<dummy_node_type *>() ) ;
                bucket_head_type h(static_cast<list_node_type *>(pHead)) ;
                return base_class::find_at_( h, val, cmp )   ;
            }

            bool insert_aux_node( dummy_node_type * pNode )
            {
                return base_class::insert_aux_node( pNode ) ;
            }
            bool insert_aux_node( dummy_node_type * pHead, dummy_node_type * pNode )
            {
                bucket_head_type h(static_cast<list_node_type *>(pHead)) ;
                return base_class::insert_aux_node( h, pNode ) ;
            }
        };

        //@endcond

    protected:
        ordered_list_wrapper    m_List              ;   ///< Ordered list containing split-list items
        bucket_table            m_Buckets           ;   ///< bucket table
        CDS_ATOMIC::atomic<size_t>  m_nBucketCountLog2  ;   ///< log2( current bucket count )
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
                if ( p && p != null_ptr<dummy_node_type volatile *>() )
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
            size_t sz = m_nBucketCountLog2.load(CDS_ATOMIC::memory_order_relaxed) ;
            if ( ( ++m_ItemCounter >> sz ) > m_Buckets.load_factor() && ((size_t)(1 << sz )) < m_Buckets.capacity() )
            {
                m_nBucketCountLog2.compare_exchange_strong( sz, sz + 1, CDS_ATOMIC::memory_order_seq_cst, CDS_ATOMIC::memory_order_relaxed ) ;
            }
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
            , size_t nLoadFactor = 1    ///< load factor - average item count per bucket. Small integer up to 10, default is 1.
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

            Returns \p true if \p val is placed into the set, \p false otherwise.
        */
        bool insert( value_type& val )
        {
            return insert_( val ) != end()  ;
        }

        /// Ensures that the \p item exists in the set
        /**
            The operation performs inserting or changing data with lock-free manner.

            If the item \p val not found in the set, then \p val is inserted into the set.
            Otherwise, the functor \p func is called with item found.
            The functor signature is:
            \code
            struct ensure_functor {
                void operator()( bool bNew, value_type& item, value_type& val ) ;
            };
            \endcode
            with arguments:
            - \p bNew - \p true if the item has been inserted, \p false otherwise
            - \p item - item of the set
            - \p val - argument \p val passed into the \p ensure function
            If new item has been inserted (i.e. \p bNew is \p true) then \p item and \p val arguments
            refers to the same thing.

            The functor can change non-key fields of the \p item; however, \p func must guarantee
            that during changing no any other modifications could be made on this item by concurrent threads.

            You can pass \p func argument by value by reference using <tt>boost::ref</tt> or cds::ref.

            Returns std::pair<bool, bool> where \p first is \p true if operation is successfull,
            \p second is \p true if new item has been added or \p false if the item with given key
            already is in the set.
        */
        template <typename Func>
        std::pair<bool, bool> ensure( value_type& val, Func func )
        {
            std::pair<iterator, bool> ret = ensure_( val, func )    ;
            return std::make_pair( ret.first != end(), ret.second ) ;
        }

        /// Finds the key \p val
        /** \anchor cds_intrusive_SplitListSet_nogc_find_val
            The function searches the item with key equal to \p val
            and returns pointer to item found or , and \p NULL otherwise.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q>
        value_type * find( Q const & val )
        {
            iterator it = find_( val )  ;
            if ( it == end() )
                return null_ptr<value_type *>() ;
            return &*it         ;
        }

        /// Finds the key \p val with \p pred predicate for comparing
        /**
            The function is an analog of \ref cds_intrusive_SplitListSet_nogc_find_val "find(Q const&)"
            but \p cmp is used for key compare.
            \p Less has the interface like \p std::less.
            \p cmp must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        value_type * find_with( Q const& val, Less pred )
        {
            iterator it = find_with_( val, pred )  ;
            if ( it == end() )
                return null_ptr<value_type *>() ;
            return &*it         ;
        }

        /// Finds the key \p val
        /** \anchor cds_intrusive_SplitListSet_nogc_find_func
            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You can pass \p f argument by value or by reference using <tt>boost::ref</tt> or cds::ref.

            The functor can change non-key fields of \p item.
            The functor does not serialize simultaneous access to the set \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            The \p val argument is non-const since it can be used as \p f functor destination i.e., the functor
            may modify both arguments.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q& val, Func f )
        {
            return find_( val, key_comparator(), f )  ;
        }

        /// Finds the key \p val with \p pred predicate for comparing
        /**
            The function is an analog of \ref cds_intrusive_SplitListSet_nogc_find_func "find(Q&, Func)"
            but \p cmp is used for key compare.
            \p Less has the interface like \p std::less.
            \p cmp must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q& val, Less pred, Func f )
        {
            return find_( val, typename wrapped_ordered_list::template make_compare_from_less<Less>(), f )  ;
        }

        /// Find the key \p val
        /** \anchor cds_intrusive_SplitListSet_nogc_find_cfunc
            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q const& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You can pass \p f argument by value or by reference using <tt>boost::ref</tt> or cds::ref.

            The functor can change non-key fields of \p item.
            The functor does not serialize simultaneous access to the set \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q const& val, Func f )
        {
            return find_( val, key_comparator(), f )  ;
        }

        /// Finds the key \p val with \p pred predicate for comparing
        /**
            The function is an analog of \ref cds_intrusive_SplitListSet_nogc_find_cfunc "find(Q const&, Func)"
            but \p cmp is used for key compare.
            \p Less has the interface like \p std::less.
            \p cmp must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q const& val, Less pred, Func f )
        {
            return find_( val, typename wrapped_ordered_list::template make_compare_from_less<Less>(), f )  ;
        }

        /// Checks if the set is empty
        /**
            Emptiness is checked by item counting: if item count is zero then the set is empty.
            Thus, the correct item counting feature is an important part of split-list implementation.
        */
        bool empty() const
        {
            return size() == 0  ;
        }

        /// Returns item count in the set
        size_t size() const
        {
            return m_ItemCounter    ;
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
        //@{
        const_iterator begin() const
        {
            return const_iterator( m_List.begin(), m_List.end() ) ;
        }
        const_iterator cbegin()
        {
            return const_iterator( m_List.cbegin(), m_List.cend() ) ;
        }
        //@}

        /// Returns an const iterator that addresses the location succeeding the last element in a split-list
        //@{
        const_iterator end() const
        {
            return const_iterator( m_List.end(), m_List.end() ) ;
        }
        const_iterator cend()
        {
            return const_iterator( m_List.cend(), m_List.cend() ) ;
        }
        //@}

    protected:
        //@cond
        iterator insert_( value_type& val )
        {
            size_t nHash = hash_value( val )    ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() )             ;

            node_traits::to_node_ptr( val )->m_nHash = split_list::regular_hash( nHash ) ;

            list_iterator it = m_List.insert_at_( pHead, val )  ;
            if ( it != m_List.end() ) {
                inc_item_count()  ;
                return iterator( it, m_List.end() )   ;
            }
            return end() ;
        }

        template <typename Func>
        std::pair<iterator, bool> ensure_( value_type& val, Func func )
        {
            size_t nHash = hash_value( val )    ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

            node_traits::to_node_ptr( val )->m_nHash = split_list::regular_hash( nHash ) ;

            std::pair<list_iterator, bool> ret = m_List.ensure_at_( pHead, val, func ) ;
            if ( ret.first != m_List.end() ) {
                if ( ret.second )
                    inc_item_count()  ;
                return std::make_pair( iterator(ret.first, m_List.end()), ret.second )    ;
            }
            return std::make_pair( end(), ret.second )    ;
        }

        template <typename Q, typename Less >
        iterator find_with_( Q const& val, Less pred )
        {
            size_t nHash = hash_value( val )    ;
            split_list::details::search_value_type<Q const>  sv( val, split_list::regular_hash( nHash )) ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

            return iterator( m_List.find_at_( pHead, sv, typename wrapped_ordered_list::template make_compare_from_less<Less>() ), m_List.end() ) ;
        }

        template <typename Q>
        iterator find_( Q const& val )
        {
            size_t nHash = hash_value( val )    ;
            split_list::details::search_value_type<Q const>  sv( val, split_list::regular_hash( nHash )) ;
            dummy_node_type * pHead = get_bucket( nHash ) ;
            assert( pHead != null_ptr<dummy_node_type *>() ) ;

            return iterator( m_List.find_at_( pHead, sv, key_comparator() ), m_List.end() ) ;

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

        //@endcond
    };

}} // namespace cds::intrusive

#endif // #ifndef __CDS_INTRUSIVE_SPLIT_LIST_NOGC_H
