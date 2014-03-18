//$$CDS-header$$

#ifndef __CDS_INTRUSIVE_MICHAEL_SET_RCU_H
#define __CDS_INTRUSIVE_MICHAEL_SET_RCU_H

#include <cds/intrusive/michael_set_base.h>
#include <cds/details/allocator.h>

namespace cds { namespace intrusive {

    /// Michael's hash set, \ref cds_urcu_desc "RCU" specialization
    /** @ingroup cds_intrusive_map
        \anchor cds_intrusive_MichaelHashSet_rcu

        Source:
            - [2002] Maged Michael "High performance dynamic lock-free hash tables and list-based sets"

        Michael's hash table algorithm is based on lock-free ordered list and it is very simple.
        The main structure is an array \p T of size \p M. Each element in \p T is basically a pointer
        to a hash bucket, implemented as a singly linked list. The array of buckets cannot be dynamically expanded.
        However, each bucket may contain unbounded number of items.

        Template parameters are:
        - \p RCU - one of \ref cds_urcu_gc "RCU type"
        - \p OrderedList - ordered list implementation used as bucket for hash set, for example, MichaelList, LazyList.
            The intrusive ordered list implementation specifies the type \p T stored in the hash-set, the reclamation
            schema \p GC used by hash-set, the comparison functor for the type \p T and other features specific for
            the ordered list.
        - \p Traits - type traits. See michael_set::type_traits for explanation.
            Instead of defining \p Traits struct you can use option-based syntax with michael_set::make_traits metafunction.

        \par Usage
            Before including <tt><cds/intrusive/michael_set_rcu.h></tt> you should include appropriate RCU header file,
            see \ref cds_urcu_gc "RCU type" for list of existing RCU class and corresponding header files.
            For example, for \ref cds_urcu_general_buffered_gc "general-purpose buffered RCU" you should include:
            \code
            #include <cds/urcu/general_buffered.h>
            #include <cds/intrusive/michael_list_rcu.h>
            #include <cds/intrusive/michael_set_rcu.h>

            struct Foo { ... };
            // Hash functor for struct Foo
            struct foo_hash {
                size_t operator()( Foo const& foo ) const { return ... }
            };

            // Now, you can declare Michael's list for type Foo and default traits:
            typedef cds::intrusive::MichaelList<cds::urcu::gc< general_buffered<> >, Foo > rcu_michael_list ;

            // Declare Michael's set with MichaelList as bucket type
            typedef cds::intrusive::MichaelSet< 
                cds::urcu::gc< general_buffered<> >, 
                rcu_michael_list, 
                cds::intrusive::michael_set::make_traits<
                    cds::opt::::hash< foo_hash >
                >::type
            > rcu_michael_set;

            // Declares hash set for 1000000 items with load factor 2
            rcu_michael_set theSet( 1000000, 2 );

            // Now you can use theSet object in many threads without any synchronization.
            \endcode
    */
    template <
        class RCU,
        class OrderedList,
#ifdef CDS_DOXYGEN_INVOKED
        class Traits = michael_set::type_traits
#else
        class Traits
#endif
    >
    class MichaelHashSet< cds::urcu::gc< RCU >, OrderedList, Traits >
    {
    public:
        typedef OrderedList bucket_type     ;   ///< type of ordered list used as a bucket implementation
        typedef Traits      options         ;   ///< Traits template parameters

        typedef typename bucket_type::value_type        value_type      ;   ///< type of value stored in the list
        typedef cds::urcu::gc< RCU >                    gc              ;   ///< RCU schema
        typedef typename bucket_type::key_comparator    key_comparator  ;   ///< key comparison functor
        typedef typename bucket_type::disposer          disposer        ;   ///< Node disposer functor

        /// Hash functor for \ref value_type and all its derivatives that you use
        typedef typename cds::opt::v::hash_selector< typename options::hash >::type   hash ;
        typedef typename options::item_counter          item_counter    ;   ///< Item counter type

        /// Bucket table allocator
        typedef cds::details::Allocator< bucket_type, typename options::allocator >  bucket_table_allocator ;

    protected:
        item_counter    m_ItemCounter   ;   ///< Item counter
        hash            m_HashFunctor   ;   ///< Hash functor

        bucket_type *   m_Buckets       ;   ///< bucket table

    private:
        //@cond
        const size_t    m_nHashBitmask ;
        //@endcond

    protected:
        //@cond
        /// Calculates hash value of \p key
        template <typename Q>
        size_t hash_value( Q const& key ) const
        {
            return m_HashFunctor( key ) & m_nHashBitmask   ;
        }

        /// Returns the bucket (ordered list) for \p key
        template <typename Q>
        bucket_type& bucket( Q const& key )
        {
            return m_Buckets[ hash_value( key ) ]  ;
        }
        template <typename Q>
        bucket_type const& bucket( Q const& key ) const
        {
            return m_Buckets[ hash_value( key ) ]  ;
        }
        //@endcond

    public:
        /// Forward iterator
        /**
            The forward iterator for Michael's set is based on \p OrderedList forward iterator and has some features:
            - it has no post-increment operator
            - it iterates items in unordered fashion
        */
        typedef michael_set::details::iterator< bucket_type, false >    iterator    ;

        /// Const forward iterator
        /**
            For iterator's features and requirements see \ref iterator
        */
        typedef michael_set::details::iterator< bucket_type, true >     const_iterator    ;

        /// Returns a forward iterator addressing the first element in a set
        /**
            For empty set \code begin() == end() \endcode
        */
        iterator begin()
        {
            return iterator( m_Buckets[0].begin(), m_Buckets, m_Buckets + bucket_count() )    ;
        }

        /// Returns an iterator that addresses the location succeeding the last element in a set
        /**
            Do not use the value returned by <tt>end</tt> function to access any item.
            The returned value can be used only to control reaching the end of the set.
            For empty set \code begin() == end() \endcode
        */
        iterator end()
        {
            return iterator( m_Buckets[bucket_count() - 1].end(), m_Buckets + bucket_count() - 1, m_Buckets + bucket_count() )   ;
        }

        /// Returns a forward const iterator addressing the first element in a set
        //@{
        const_iterator begin() const
        {
            return get_const_begin();
        }
        const_iterator cbegin()
        {
            return get_const_begin();
        }
        //@}

        /// Returns an const iterator that addresses the location succeeding the last element in a set
        //@{
        const_iterator end() const
        {
            return get_const_end();
        }
        const_iterator cend()
        {
            return get_const_end();
        }
        //@}

    private:
        //@cond
        const_iterator get_const_begin() const
        {
            return const_iterator( m_Buckets[0].begin(), m_Buckets, m_Buckets + bucket_count() )    ;
        }
        const_iterator get_const_end() const
        {
            return const_iterator( m_Buckets[bucket_count() - 1].end(), m_Buckets + bucket_count() - 1, m_Buckets + bucket_count() )   ;
        }
        //@endcond

    public:
        /// Initialize hash set
        /**
            The Michael's hash set is an unbounded container, but its hash table is non-expandable.
            At construction time you should pass estimated maximum item count and a load factor.
            The load factor is average size of one bucket - a small number between 1 and 10.
            The bucket is an ordered single-linked list, the complexity of searching in the bucket is linear <tt>O(nLoadFactor)</tt>.
            The constructor defines hash table size as rounding <tt>nMaxItemCount / nLoadFactor</tt> up to nearest power of two.
        */
        MichaelHashSet(
            size_t nMaxItemCount,   ///< estimation of max item count in the hash set
            size_t nLoadFactor      ///< load factor: average size of the bucket
        ) : m_nHashBitmask( michael_set::details::init_hash_bitmask( nMaxItemCount, nLoadFactor ))
        {
            // GC and OrderedList::gc must be the same
            static_assert(( std::is_same<gc, typename bucket_type::gc>::value ), "GC and OrderedList::gc must be the same")  ;

            // atomicity::empty_item_counter is not allowed as a item counter
            static_assert(( !std::is_same<item_counter, atomicity::empty_item_counter>::value ), "atomicity::empty_item_counter is not allowed as a item counter")  ;

            m_Buckets = bucket_table_allocator().NewArray( bucket_count() ) ;
        }

        /// Clear hash set and destroy it
        ~MichaelHashSet()
        {
            clear() ;
            bucket_table_allocator().Delete( m_Buckets, bucket_count() ) ;
        }

        /// Inserts new node
        /**
            The function inserts \p val in the set if it does not contain
            an item with key equal to \p val.

            Returns \p true if \p val is placed into the set, \p false otherwise.
        */
        bool insert( value_type& val )
        {
            bool bRet = bucket( val ).insert( val ) ;
            if ( bRet )
                ++m_ItemCounter ;
            return bRet ;
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
            The user-defined functor is called only if the inserting is success and can be passed by reference
            using <tt>boost::ref</tt>
        */
        template <typename Func>
        bool insert( value_type& val, Func f )
        {
            bool bRet = bucket( val ).insert( val, f )    ;
            if ( bRet )
                ++m_ItemCounter ;
            return bRet ;
        }

        /// Ensures that the \p item exists in the set
        /**
            The operation performs inserting or changing data with lock-free manner.

            If the item \p val not found in the set, then \p val is inserted into the set.
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

            Returns std::pair<bool, bool> where \p first is \p true if operation is successfull,
            \p second is \p true if new item has been added or \p false if the item with \p key
            already is in the set.
        */
        template <typename Func>
        std::pair<bool, bool> ensure( value_type& val, Func func )
        {
            std::pair<bool, bool> bRet = bucket( val ).ensure( val, func )    ;
            if ( bRet.first && bRet.second )
                ++m_ItemCounter ;
            return bRet ;
        }

        /// Unlinks the item \p val from the set
        /**
            The function searches the item \p val in the set and unlink it from the set
            if it is found and is equal to \p val.

            The function returns \p true if success and \p false otherwise.
        */
        bool unlink( value_type& val )
        {
            bool bRet = bucket( val ).unlink( val )  ;
            if ( bRet )
                --m_ItemCounter    ;
            return bRet ;
        }

        /// Deletes the item from the set
        /** \anchor cds_intrusive_MichaelHashSet_rcu_erase
            The function searches an item with key equal to \p val in the set,
            unlinks it from the set, and returns \p true.
            If the item with key equal to \p val is not found the function return \p false.

            Note the hash functor should accept a parameter of type \p Q that may be not the same as \p value_type.
        */
        template <typename Q>
        bool erase( Q const& val )
        {
            if ( bucket( val ).erase( val )) {
                --m_ItemCounter     ;
                return true         ;
            }
            return false    ;
        }

        /// Deletes the item from the set using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_rcu_erase "erase(Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool erase_with( Q const& val, Less pred )
        {
            if ( bucket( val ).erase_with( val, pred )) {
                --m_ItemCounter     ;
                return true         ;
            }
            return false    ;
        }

        /// Deletes the item from the set
        /** \anchor cds_intrusive_MichaelHashSet_rcu_erase_func
            The function searches an item with key equal to \p val in the set,
            call \p f functor with item found, and unlinks it from the set.
            The \ref disposer specified in \p OrderedList class template parameter is called
            by garbage collector \p GC asynchronously.

            The \p Func interface is
            \code
            struct functor {
                void operator()( value_type const& item ) ;
            } ;
            \endcode
            The functor may be passed by reference with <tt>boost:ref</tt>

            If the item with key equal to \p val is not found the function return \p false.

            Note the hash functor should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q, typename Func>
        bool erase( const Q& val, Func f )
        {
            if ( bucket( val ).erase( val, f )) {
                --m_ItemCounter     ;
                return true         ;
            }
            return false    ;
        }

        /// Deletes the item from the set using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_rcu_erase_func "erase(Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool erase_with( const Q& val, Less pred, Func f )
        {
            if ( bucket( val ).erase_with( val, pred, f )) {
                --m_ItemCounter     ;
                return true         ;
            }
            return false    ;
        }

        /// Extracts an item from the set
        /** \anchor cds_intrusive_MichaelHashSet_rcu_extract
            The function searches an item with key equal to \p val in the set,
            unlinks it from the set, places item pointer into \p dest argument, and returns \p true.
            If the item with the key equal to \p val is not found the function return \p false.

            @note The function does NOT call RCU read-side lock or synchronization,
            and does NOT dispose the item found. It just excludes the item from the set
            and returns a pointer to item found.
            You should lock RCU before calling of the function, and you should synchronize RCU
            outside the RCU lock before reusing returned pointer.

            \code
            #include <cds/urcu/general_buffered.h>
            #include <cds/intrusive/michael_list_rcu.h>
            #include <cds/intrusive/michael_set_rcu.h>

            typedef cds::urcu::gc< general_buffered<> > rcu ;
            typedef cds::intrusive::MichaelList< rcu, Foo > rcu_michael_list ;
            typedef cds::intrusive::MichaelHashSet< rcu, rcu_michael_list, foo_traits > rcu_michael_set;

            rcu_michael_set theSet ;
            // ...

            rcu_michael_set::exempt_ptr p;
            {
                // first, we should lock RCU
                rcu_michael_set::rcu_lock lock;

                // Now, you can apply extract function
                // Note that you must not delete the item found inside the RCU lock
                if ( theSet.extract( p, 10 )) {
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
            if ( bucket( val ).extract( dest, val )) {
                --m_ItemCounter;
                return true;
            }
            return false;
        }

        /// Extracts an item from the set using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_rcu_extract "extract(exempt_ptr&, Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool extract_with( exempt_ptr& dest, Q const& val, Less pred )
        {
            if ( bucket( val ).extract_with( dest, val, pred )) {
                --m_ItemCounter;
                return true;
            }
            return false;
        }

        /// Finds the key \p val
        /** \anchor cds_intrusive_MichaelHashSet_rcu_find_val
            The function searches the item with key equal to \p val
            and returns \p true if \p val found or \p false otherwise.
        */
        template <typename Q>
        bool find( Q const& val ) const
        {
            return bucket( val ).find( val )  ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_rcu_find_val "find(Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool find_with( Q const& val, Less pred ) const
        {
            return bucket( val ).find_with( val, pred )  ;
        }

        /// Find the key \p val
        /** \anchor cds_intrusive_MichaelHashSet_rcu_find_func
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
            can modify both arguments.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q& val, Func f ) const
        {
            return bucket( val ).find( val, f )  ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_rcu_find_func "find(Q&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q& val, Less pred, Func f ) const
        {
            return bucket( val ).find_with( val, pred, f )  ;
        }

        /// Find the key \p val
        /** \anchor cds_intrusive_MichaelHashSet_rcu_find_cfunc
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
        bool find( Q const& val, Func f ) const
        {
            return bucket( val ).find( val, f )  ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_rcu_find_cfunc "find(Q const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q const& val, Less pred, Func f ) const
        {
            return bucket( val ).find_with( val, pred, f )  ;
        }

        /// Finds the key \p val and return the item found
        /** \anchor cds_intrusive_MichaelHashSet_rcu_get
            The function searches the item with key equal to \p val and returns the pointer to item found.
            If \p val is not found it returns \p NULL.

            Note the compare functor should accept a parameter of type \p Q that can be not the same as \p value_type.

            RCU should be locked before call of this function.
            Returned item is valid only while RCU is locked:
            \code
            typedef cds::intrusive::MichaelHashSet< your_template_parameters > hash_set;
            hash_set theSet ;
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
        value_type * get( Q const& val ) const
        {
            return bucket( val ).get( val )  ;
        }

        /// Finds the key \p val and return the item found
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_rcu_get "get(Q const&)"
            but \p pred is used for comparing the keys.

            \p Less functor has the semantics like \p std::less but should take arguments of type \ref value_type and \p Q
            in any order.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        value_type * get_with( Q const& val, Less pred ) const
        {
            return bucket( val ).get_with( val, pred )  ;
        }

        /// Clears the set (non-atomic)
        /**
            The function unlink all items from the set.
            The function is not atomic. It cleans up each bucket and then resets the item counter to zero.
            If there are a thread that performs insertion while \p clear is working the result is undefined in general case:
            <tt> empty() </tt> may return \p true but the set may contain item(s).
            Therefore, \p clear may be used only for debugging purposes.

            For each item the \p disposer is called after unlinking.
        */
        void clear()
        {
            for ( size_t i = 0; i < bucket_count(); ++i )
                m_Buckets[i].clear()  ;
            m_ItemCounter.reset()   ;
        }


        /// Checks if the set is empty
        /**
            Emptiness is checked by item counting: if item count is zero then the set is empty.
            Thus, the correct item counting feature is an important part of Michael's set implementation.
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

        /// Returns the size of hash table
        /**
            Since %MichaelHashSet cannot dynamically extend the hash table size,
            the value returned is an constant depending on object initialization parameters;
            see \ref cds_intrusive_MichaelHashSet_hp "MichaelHashSet" for explanation.
        */
        size_t bucket_count() const
        {
            return m_nHashBitmask + 1   ;
        }

    };

}} // namespace cds::intrusive

#endif // #ifndef __CDS_INTRUSIVE_MICHAEL_SET_NOGC_H

