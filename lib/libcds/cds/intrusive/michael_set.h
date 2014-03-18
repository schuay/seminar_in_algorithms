//$$CDS-header$$

#ifndef __CDS_INTRUSIVE_MICHAEL_SET_H
#define __CDS_INTRUSIVE_MICHAEL_SET_H

#include <cds/intrusive/michael_set_base.h>
#include <cds/details/allocator.h>

namespace cds { namespace intrusive {

    /// Michael's hash set
    /** @ingroup cds_intrusive_map
        \anchor cds_intrusive_MichaelHashSet_hp

        Source:
            - [2002] Maged Michael "High performance dynamic lock-free hash tables and list-based sets"

        Michael's hash table algorithm is based on lock-free ordered list and it is very simple.
        The main structure is an array \p T of size \p M. Each element in \p T is basically a pointer
        to a hash bucket, implemented as a singly linked list. The array of buckets cannot be dynamically expanded.
        However, each bucket may contain unbounded number of items.

        Template parameters are:
        - \p GC - Garbage collector used. Note the \p GC must be the same as the GC used for \p OrderedList
        - \p OrderedList - ordered list implementation used as bucket for hash set, for example, MichaelList, LazyList.
            The intrusive ordered list implementation specifies the type \p T stored in the hash-set, the reclamation
            schema \p GC used by hash-set, the comparison functor for the type \p T and other features specific for
            the ordered list.
        - \p Traits - type traits. See michael_set::type_traits for explanation.
            Instead of defining \p Traits struct you can use option-based syntax with michael_set::make_traits metafunction.

        There are several specializations of \p %MichaelHashSet for each GC. You should include:
        - <tt><cds/intrusive/michael_set_rcu.h></tt> for \ref cds_intrusive_MichaelHashSet_rcu "RCU type"
        - <tt><cds/intrusive/michael_set_nogc.h></tt> for \ref cds_intrusive_MichaelHashSet_nogc for persistent set
        - <tt><cds/intrusive/michael_set.h></tt> for other GC (gc::HP, gc::HRC, gc::PTB)

        <b>Hash functor</b>

        Some member functions of Michael's hash set accept the key parameter of type \p Q which differs from \p value_type.
        It is expected that type \p Q contains full key of \p value_type, and for equal keys of type \p Q and \p value_type
        the hash values of these keys must be equal too.
        The hash functor <tt>Traits::hash</tt> should accept parameters of both type:
        \code
        // Our node type
        struct Foo {
            std::string     key_    ;   // key field
            // ... other fields
        }   ;

        // Hash functor
        struct fooHash {
            size_t operator()( const std::string& s ) const
            {
                return std::hash( s )   ;
            }

            size_t operator()( const Foo& f ) const
            {
                return (*this)( f.key_ )    ;
            }
        };
        \endcode


        <b>How to use</b>

        First, you should define ordered list type to use in your hash set:
        \code
        // For gc::HP-based MichaelList implementation
        #include <cds/intrusive/michael_list_hp.h>

        // cds::intrusive::MichaelHashSet declaration
        #include <cds/intrusive/michael_set.h>

        // Type of hash-set items
        struct Foo: public cds::intrusive::michael_list::node< cds::gc::HP >
        {
            std::string     key_    ;   // key field
            unsigned        val_    ;   // value field
            // ...  other value fields
        };

        // Declare comparator for the item
        struct FooCmp
        {
            int operator()( const Foo& f1, const Foo& f2 ) const
            {
                return f1.key_.compare( f2.key_ ) ;
            }
        };

        // Declare bucket type for Michael's hash set
        // The bucket type is any ordered list type like MichaelList, LazyList
        typedef cds::intrusive::MichaelList< cds::gc::HP, Foo,
            typename cds::intrusive::michael_list::make_traits<
                // hook option
                cds::intrusive::opt::hook< cds::intrusive::michael_list::base_hook< cds::opt::gc< cds::gc::HP > > >
                // item comparator option
                ,cds::opt::compare< FooCmp >
            >::type
        >  Foo_bucket   ;
        \endcode

        Second, you should declare Michael's hash set container:
        \code

        // Declare hash functor
        // Note, the hash functor accepts parameter type Foo and std::string
        struct FooHash {
            size_t operator()( const Foo& f ) const
            {
                return cds::opt::v::hash<std::string>()( f.key_ ) ;
            }
            size_t operator()( const std::string& f ) const
            {
                return cds::opt::v::hash<std::string>()( f ) ;
            }
        };

        // Michael's set typedef
        typedef cds::intrusive::MichaelHashSet<
            cds::gc::HP
            ,Foo_bucket
            ,typename cds::intrusive::michael_set::make_traits<
                cds::opt::hash< FooHash >
            >::type
        > Foo_set   ;
        \endcode

        Now, you can use \p Foo_set in your application.

        Like other intrusive containers, you may build several containers on single item structure:
        \code
        #include <cds/intrusive/michael_list_hp.h>
        #include <cds/intrusive/michael_list_ptb.h>
        #include <cds/intrusive/michael_set.h>

        struct tag_key1_idx ;
        struct tag_key2_idx ;

        // Your two-key data
        // The first key is maintained by gc::HP, second key is maintained by gc::PTB garbage collectors
        struct Foo
            : public cds::intrusive::michael_list::node< cds::gc::HP, tag_key1_idx >
            , public cds::intrusive::michael_list::node< cds::gc::PTB, tag_key2_idx >
        {
            std::string     key1_   ;   // first key field
            unsigned int    key2_   ;   // second key field

            // ... value fields and fields for controlling item's lifetime
        };

        // Declare comparators for the item
        struct Key1Cmp
        {
            int operator()( const Foo& f1, const Foo& f2 ) const { return f1.key1_.compare( f2.key1_ ) ; }
        };
        struct Key2Less
        {
            bool operator()( const Foo& f1, const Foo& f2 ) const { return f1.key2_ < f2.key1_ ; }
        };

        // Declare bucket type for Michael's hash set indexed by key1_ field and maintained by gc::HP
        typedef cds::intrusive::MichaelList< cds::gc::HP, Foo,
            typename cds::intrusive::michael_list::make_traits<
                // hook option
                cds::intrusive::opt::hook< cds::intrusive::michael_list::base_hook< cds::opt::gc< cds::gc::HP >, tag_key1_idx > >
                // item comparator option
                ,cds::opt::compare< Key1Cmp >
            >::type
        >  Key1_bucket   ;

        // Declare bucket type for Michael's hash set indexed by key2_ field and maintained by gc::PTB
        typedef cds::intrusive::MichaelList< cds::gc::PTB, Foo,
            typename cds::intrusive::michael_list::make_traits<
                // hook option
                cds::intrusive::opt::hook< cds::intrusive::michael_list::base_hook< cds::opt::gc< cds::gc::PTB >, tag_key2_idx > >
                // item comparator option
                ,cds::opt::less< Key2Less >
            >::type
        >  Key2_bucket   ;

        // Declare hash functor
        struct Key1Hash {
            size_t operator()( const Foo& f ) const { return cds::opt::v::hash<std::string>()( f.key1_ ) ; }
            size_t operator()( const std::string& s ) const { return cds::opt::v::hash<std::string>()( s ) ; }
        };
        inline size_t Key2Hash( const Foo& f ) { return (size_t) f.key2_  ; }

        // Michael's set indexed by key1_ field
        typedef cds::intrusive::MichaelHashSet<
            cds::gc::HP
            ,Key1_bucket
            ,typename cds::intrusive::michael_set::make_traits<
                cds::opt::hash< Key1Hash >
            >::type
        > key1_set   ;

        // Michael's set indexed by key2_ field
        typedef cds::intrusive::MichaelHashSet<
            cds::gc::PTB
            ,Key2_bucket
            ,typename cds::intrusive::michael_set::make_traits<
                cds::opt::hash< Key2Hash >
            >::type
        > key2_set   ;
        \endcode
    */
    template <
        class GC,
        class OrderedList,
#ifdef CDS_DOXYGEN_INVOKED
        class Traits = michael_set::type_traits
#else
        class Traits
#endif
    >
    class MichaelHashSet
    {
    public:
        typedef OrderedList     ordered_list    ;   ///< type of ordered list used as a bucket implementation
        typedef ordered_list    bucket_type     ;   ///< bucket type
        typedef Traits          options         ;   ///< Traits template parameters

        typedef typename ordered_list::value_type       value_type      ;   ///< type of value stored in the list
        typedef GC                                      gc              ;   ///< Garbage collector
        typedef typename ordered_list::key_comparator   key_comparator  ;   ///< key comparison functor
        typedef typename ordered_list::disposer         disposer        ;   ///< Node disposer functor

        /// Hash functor for \ref value_type and all its derivatives that you use
        typedef typename cds::opt::v::hash_selector< typename options::hash >::type   hash ;
        typedef typename options::item_counter          item_counter    ;   ///< Item counter type

        typedef typename ordered_list::guarded_ptr      guarded_ptr; ///< Guarded pointer

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
        /// Calculates hash value of \p key
        template <typename Q>
        size_t hash_value( const Q& key ) const
        {
            return m_HashFunctor( key ) & m_nHashBitmask   ;
        }

        /// Returns the bucket (ordered list) for \p key
        template <typename Q>
        bucket_type&    bucket( const Q& key )
        {
            return m_Buckets[ hash_value( key ) ]  ;
        }

    public:
        /// Forward iterator
        /**
            The forward iterator for Michael's set is based on \p OrderedList forward iterator and has some features:
            - it has no post-increment operator
            - it iterates items in unordered fashion
            - The iterator cannot be moved across thread boundary since it may contain GC's guard that is thread-private GC data.
            - Iterator ensures thread-safety even if you delete the item that iterator points to. However, in case of concurrent
              deleting operations it is no guarantee that you iterate all item in the set.

            Therefore, the use of iterators in concurrent environment is not good idea. Use the iterator for the concurrent container
            for debug purpose only.
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
        //@{
        const_iterator get_const_begin() const
        {
            return const_iterator( m_Buckets[0].begin(), m_Buckets, m_Buckets + bucket_count() )    ;
        }
        const_iterator get_const_end() const
        {
            return const_iterator( m_Buckets[bucket_count() - 1].end(), m_Buckets + bucket_count() - 1, m_Buckets + bucket_count() )   ;
        }
        //@}

    public:
        /// Initializes hash set
        /**
            The Michael's hash set is an unbounded container, but its hash table is non-expandable.
            At construction time you should pass estimated maximum item count and a load factor.
            The load factor is average size of one bucket - a small number between 1 and 10.
            The bucket is an ordered single-linked list, searching in the bucket has linear complexity <tt>O(nLoadFactor)</tt>.
            The constructor defines hash table size as rounding <tt>nMaxItemCount / nLoadFactor</tt> up to nearest power of two.
        */
        MichaelHashSet(
            size_t nMaxItemCount,   ///< estimation of max item count in the hash set
            size_t nLoadFactor      ///< load factor: estimation of max number of items in the bucket. Small integer up to 10, default is 1.
        ) : m_nHashBitmask( michael_set::details::init_hash_bitmask( nMaxItemCount, nLoadFactor ))
        {
            // GC and OrderedList::gc must be the same
            static_assert(( std::is_same<gc, typename bucket_type::gc>::value ), "GC and OrderedList::gc must be the same")  ;

            // atomicity::empty_item_counter is not allowed as a item counter
            static_assert(( !std::is_same<item_counter, atomicity::empty_item_counter>::value ), "atomicity::empty_item_counter is not allowed as a item counter")  ;

            m_Buckets = bucket_table_allocator().NewArray( bucket_count() ) ;
        }

        /// Clears hash set object and destroys it
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

        /// Ensures that the \p val exists in the set
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

            The functor may change non-key fields of the \p item; however, \p func must guarantee
            that during changing no any other modifications could be made on this item by concurrent threads.

            You may pass \p func argument by reference using <tt>boost::ref</tt> or cds::ref.

            Returns <tt> std::pair<bool, bool> </tt> where \p first is \p true if operation is successfull,
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
        /** \anchor cds_intrusive_MichaelHashSet_hp_erase
            The function searches an item with key equal to \p val in the set,
            unlinks it from the set, and returns \p true.
            If the item with key equal to \p val is not found the function return \p false.

            Note the hash functor should accept a parameter of type \p Q that can be not the same as \p value_type.
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
            The function is an analog of \ref cds_intrusive_MichaelHashSet_hp_erase "erase(Q const&)"
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
        /** \anchor cds_intrusive_MichaelHashSet_hp_erase_func
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
            The function is an analog of \ref cds_intrusive_MichaelHashSet_hp_erase_func "erase(Q const&, Func)"
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

        /// Extracts the item with specified \p key
        /** \anchor cds_intrusive_MichaelHashSet_hp_extract
            The function searches an item with key equal to \p key,
            unlinks it from the set, and returns it in \p dest parameter.
            If the item with key equal to \p key is not found the function returns \p false.

            Note the compare functor should accept a parameter of type \p Q that may be not the same as \p value_type.

            The \ref disposer specified in \p OrderedList class' template parameter is called automatically
            by garbage collector \p GC when returned \ref guarded_ptr object will be destroyed or released.
            @note Each \p guarded_ptr object uses the GC's guard that can be limited resource.

            Usage:
            \code
            typedef cds::intrusive::MichaelHashSet< your_template_args > michael_set;
            michael_set theSet;
            // ...
            {
                michael_set::guarded_ptr gp;
                theSet.extract( gp, 5 );
                // Deal with gp
                // ...

                // Destructor of gp releases internal HP guard
            }
            \endcode
        */
        template <typename Q>
        bool extract( guarded_ptr& dest, Q const& key )
        {
            if ( bucket( key ).extract( dest, key )) {
                --m_ItemCounter     ;
                return true         ;
            }
            return false    ;
        }

        /// Extracts the item using compare functor \p pred
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_hp_extract "extract(guarded_ptr&, Q const&)"
            but \p pred predicate is used for key comparing.

            \p Less functor has the semantics like \p std::less but should take arguments of type \ref value_type and \p Q
            in any order.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename Q, typename Less>
        bool extract_with( guarded_ptr& dest, Q const& key, Less pred )
        {
            if ( bucket( key ).extract_with( dest, key, pred )) {
                --m_ItemCounter     ;
                return true         ;
            }
            return false    ;
        }

        /// Finds the key \p val
        /** \anchor cds_intrusive_MichaelHashSet_hp_find_func
            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You can pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change non-key fields of \p item. Note that the functor is only guarantee
            that \p item cannot be disposed during functor is executing.
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
            return bucket( val ).find( val, f )  ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_hp_find_func "find(Q&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q& val, Less pred, Func f )
        {
            return bucket( val ).find_with( val, pred, f )  ;
        }

        /// Finds the key \p val
        /** \anchor cds_intrusive_MichaelHashSet_hp_find_cfunc
            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You can pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change non-key fields of \p item. Note that the functor is only guarantee
            that \p item cannot be disposed during functor is executing.
            The functor does not serialize simultaneous access to the set \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q const& val, Func f )
        {
            return bucket( val ).find( val, f )  ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_hp_find_cfunc "find(Q const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q const& val, Less pred, Func f )
        {
            return bucket( val ).find_with( val, pred, f )  ;
        }

        /// Finds the key \p val
        /** \anchor cds_intrusive_MichaelHashSet_hp_find_val
            The function searches the item with key equal to \p val
            and returns \p true if it is found, and \p false otherwise.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q>
        bool find( Q const & val )
        {
            return bucket( val ).find( val )  ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_hp_find_val "find(Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool find_with( Q const & val, Less pred )
        {
            return bucket( val ).find_with( val, pred )  ;
        }

        /// Finds the key \p val and return the item found
        /** \anchor cds_intrusive_MichaelHashSet_hp_get
            The function searches the item with key equal to \p val
            and assigns the item found to guarded pointer \p ptr.
            The function returns \p true if \p val is found, and \p false otherwise.
            If \p val is not found the \p ptr parameter is not changed.

            @note Each \p guarded_ptr object uses one GC's guard which can be limited resource.

            Usage:
            \code
            typedef cds::intrusive::MichaeHashSet< your_template_params >  michael_set;
            michael_set theSet;
            // ...
            {
                michael_set::guarded_ptr gp;
                if ( theSet.get( gp, 5 )) {
                    // Deal with gp
                    //...
                }
                // Destructor of guarded_ptr releases internal HP guard
            }
            \endcode

            Note the compare functor specified for \p OrderedList template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q>
        bool get( guarded_ptr& ptr, Q const& val )
        {
            return bucket( val ).get( ptr, val )  ;
        }

        /// Finds the key \p val and return the item found
        /**
            The function is an analog of \ref cds_intrusive_MichaelHashSet_hp_get "get( guarded_ptr& ptr, Q const&)"
            but \p pred is used for comparing the keys.

            \p Less functor has the semantics like \p std::less but should take arguments of type \ref value_type and \p Q
            in any order.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool get_with( guarded_ptr& ptr, Q const& val, Less pred )
        {
            return bucket( val ).get_with( ptr, val, pred )  ;
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
            Since MichaelHashSet cannot dynamically extend the hash table size,
            the value returned is an constant depending on object initialization parameters;
            see MichaelHashSet::MichaelHashSet for explanation.
        */
        size_t bucket_count() const
        {
            return m_nHashBitmask + 1   ;
        }
    };

}}  // namespace cds::intrusive

#endif // ifndef __CDS_INTRUSIVE_MICHAEL_SET_H
