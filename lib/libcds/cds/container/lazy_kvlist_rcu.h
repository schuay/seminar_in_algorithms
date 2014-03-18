//$$CDS-header$$

#ifndef __CDS_CONTAINER_LAZY_KVLIST_RCU_H
#define __CDS_CONTAINER_LAZY_KVLIST_RCU_H

#include <cds/container/lazy_list_base.h>
#include <cds/intrusive/lazy_list_rcu.h>
#include <cds/container/details/make_lazy_kvlist.h>
#include <cds/ref.h>
#include <cds/details/functor_wrapper.h>
#include <cds/details/std/memory.h>

namespace cds { namespace container {

    /// Lazy ordered list (key-value pair), template specialization for \ref cds_urcu_desc "RCU"
    /** @ingroup cds_nonintrusive_list
        \anchor cds_nonintrusive_LazyKVList_rcu

        This is key-value variation of non-intrusive LazyList.
        Like standard container, this implementation split a value stored into two part -
        constant key and alterable value.

        Usually, ordered single-linked list is used as a building block for the hash table implementation.
        The complexity of searching is <tt>O(N)</tt>.

        Template arguments:
        - \p RCU - one of \ref cds_urcu_gc "RCU type"
        - \p Key - key type of an item stored in the list. It should be copy-constructible
        - \p Value - value type stored in the list
        - \p Traits - type traits, default is lazy_list::type_traits

        It is possible to declare option-based list with cds::container::lazy_list::make_traits metafunction istead of \p Traits template
        argument. For example, the following traits-based declaration of gc::HP lazy list
        @note Before including <tt><cds/container/lazy_kvlist_rcu.h></tt> you should include appropriate RCU header file,
        see \ref cds_urcu_gc "RCU type" for list of existing RCU class and corresponding header files.
        \code
        #include <cds/urcu/general_threaded.h>
        #include <cds/container/lazy_kvlist_rcu.h>
        // Declare comparator for the item
        struct my_compare {
            int operator ()( int i1, int i2 )
            {
                return i1 - i2  ;
            }
        };

        // Declare type_traits
        struct my_traits: public cds::container::lazy_list::type_traits
        {
            typedef my_compare compare ;
        };

        // Declare traits-based list
        typedef cds::container::LazyKVList< cds::urcu::gc< cds::urcu::general_threaded<> >, int, int, my_traits >     traits_based_list   ;
        \endcode

        is equivalent for the following option-based list
        \code
        #include <cds/urcu/general_threaded.h>
        #include <cds/container/lazy_kvlist_rcu.h>

        // my_compare is the same

        // Declare option-based list
        typedef cds::container::LazyKVList< cds::urcu::gc< cds::urcu::general_threaded<> >, int, int,
            typename cds::container::lazy_list::make_traits<
                cds::container::opt::compare< my_compare >     // item comparator option
            >::type
        >     option_based_list   ;
        \endcode

        Template argument list \p Options of cds::container::lazy_list::make_traits metafunction are:
        - opt::compare - key comparison functor. No default functor is provided.
            If the option is not specified, the opt::less is used.
        - opt::less - specifies binary predicate used for key comparison. Default is \p std::less<T>.
        - opt::back_off - back-off strategy used. If the option is not specified, the cds::backoff::empty is used.
        - opt::item_counter - the type of item counting feature. Default is \ref atomicity::empty_item_counter that is no item counting.
        - opt::allocator - the allocator used for creating and freeing list's item. Default is \ref CDS_DEFAULT_ALLOCATOR macro.
        - opt::memory_model - C++ memory ordering model. Can be opt::v::relaxed_ordering (relaxed memory model, the default)
            or opt::v::sequential_consistent (sequentially consisnent memory model).
        - opt::rcu_check_deadlock - a deadlock checking policy. Default is opt::v::rcu_throw_deadlock
    */
    template <
        typename RCU,
        typename Key,
        typename Value,
#ifdef CDS_DOXYGEN_INVOKED
        typename Traits = lazy_list::type_traits
#else
        typename Traits
#endif
    >
    class LazyKVList< cds::urcu::gc<RCU>, Key, Value, Traits >:
#ifdef CDS_DOXYGEN_INVOKED
        protected intrusive::LazyList< cds::urcu::gc<RCU>, implementation_defined, Traits >
#else
        protected details::make_lazy_kvlist< cds::urcu::gc<RCU>, Key, Value, Traits >::type
#endif
    {
        //@cond
        typedef details::make_lazy_kvlist< cds::urcu::gc<RCU>, Key, Value, Traits > options   ;
        typedef typename options::type  base_class                      ;
        //@endcond

    public:
#ifdef CDS_DOXYGEN_INVOKED
        typedef Key                                 key_type        ;   ///< Key type
        typedef Value                               mapped_type     ;   ///< Type of value stored in the list
        typedef std::pair<key_type const, mapped_type> value_type   ;   ///< key/value pair stored in the list
#else
        typedef typename options::key_type          key_type        ;
        typedef typename options::value_type        mapped_type     ;
        typedef typename options::pair_type         value_type      ;
#endif

        typedef typename base_class::gc             gc              ;   ///< Garbage collector used
        typedef typename base_class::back_off       back_off        ;   ///< Back-off strategy used
        typedef typename options::allocator_type    allocator_type  ;   ///< Allocator type used for allocate/deallocate the nodes
        typedef typename base_class::item_counter   item_counter    ;   ///< Item counting policy used
        typedef typename options::key_comparator    key_comparator  ;   ///< key comparison functor
        typedef typename base_class::memory_model   memory_model    ;   ///< Memory ordering. See cds::opt::memory_model option
        typedef typename base_class::rcu_check_deadlock rcu_check_deadlock ; ///< RCU deadlock checking policy

        typedef typename gc::scoped_lock    rcu_lock ;  ///< RCU scoped lock

    protected:
        //@cond
        typedef typename base_class::value_type     node_type       ;
        typedef typename options::cxx_allocator     cxx_allocator   ;
        typedef typename options::node_deallocator  node_deallocator;
        typedef typename options::type_traits::compare  intrusive_key_comparator    ;

        typedef typename base_class::node_type      head_type      ;
        //@endcond

    public:
        /// pointer to extracted node
        typedef cds::urcu::exempt_ptr< gc, node_type, value_type, typename options::type_traits::disposer, 
            cds::urcu::details::conventional_exempt_pair_cast<node_type, value_type> 
        > exempt_ptr; 

    private:
        //@cond
#   ifndef CDS_CXX11_LAMBDA_SUPPORT
        template <typename Func>
        class insert_functor: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            insert_functor ( Func f )
                : base_class( f )
            {}

            void operator()( node_type& node )
            {
                base_class::get()( node.m_Data )  ;
            }
        };

        template <typename Func>
        class ensure_functor: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            ensure_functor( Func f )
                : base_class(f)
            {}

            void operator ()( bool bNew, node_type& node, node_type& )
            {
                base_class::get()( bNew, node.m_Data ) ;
            }
        };

        template <typename Func>
        class find_functor: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            find_functor( Func f )
                : base_class(f)
            {}

            template <typename Q>
            void operator ()( node_type& node, Q&  )
            {
                base_class::get()( node.m_Data ) ;
            }
        };

        struct empty_find_functor
        {
            template <typename Q>
            void operator ()( node_type& node, Q& val ) const
            {}
        };

        template <typename Func>
        struct erase_functor
        {
            Func m_func ;

            erase_functor( Func f )
                : m_func( f )
            {}

            void operator ()( node_type const & node )
            {
                cds::unref(m_func)( const_cast<value_type&>(node.m_Data) )    ;
            }
        };
#   endif // ifndef CDS_CXX11_LAMBDA_SUPPORT
        //@endcond

    protected:
        //@cond
        template <typename K>
        static node_type * alloc_node(const K& key)
        {
            return cxx_allocator().New( key )    ;
        }

        template <typename K, typename V>
        static node_type * alloc_node( const K& key, const V& val )
        {
            return cxx_allocator().New( key, val ) ;
        }

#ifdef CDS_EMPLACE_SUPPORT
        template <typename... Args>
        static node_type * alloc_node( Args&&... args )
        {
            return cxx_allocator().MoveNew( std::forward<Args>(args)... )   ;
        }
#endif

        static void free_node( node_type * pNode )
        {
            cxx_allocator().Delete( pNode ) ;
        }

        struct node_disposer {
            void operator()( node_type * pNode )
            {
                free_node( pNode )  ;
            }
        };
        typedef std::unique_ptr< node_type, node_disposer >     scoped_node_ptr ;

        head_type& head()
        {
            return base_class::m_Head  ;
        }

        head_type& head() const
        {
            return const_cast<head_type&>( base_class::m_Head ) ;
        }

        head_type& tail()
        {
            return base_class::m_Tail   ;
        }

        head_type& tail() const
        {
            return const_cast<head_type&>( base_class::m_Tail ) ;
        }

        //@endcond

    protected:
        //@cond
        template <bool IsConst>
        class iterator_type: protected base_class::template iterator_type<IsConst>
        {
            typedef typename base_class::template iterator_type<IsConst>    iterator_base ;

            iterator_type( head_type const& pNode )
                : iterator_base( const_cast<head_type *>(&pNode) )
            {}
            iterator_type( head_type const * pNode )
                : iterator_base( const_cast<head_type *>(pNode) )
            {}

            friend class LazyKVList ;

        public:
            typedef typename cds::details::make_const_type<mapped_type, IsConst>::reference  value_ref;
            typedef typename cds::details::make_const_type<mapped_type, IsConst>::pointer    value_ptr;

            typedef typename cds::details::make_const_type<value_type,  IsConst>::reference  pair_ref;
            typedef typename cds::details::make_const_type<value_type,  IsConst>::pointer    pair_ptr;

            iterator_type()
            {}

            iterator_type( iterator_type const& src )
                : iterator_base( src )
            {}

            key_type const& key() const
            {
                typename iterator_base::value_ptr p = iterator_base::operator ->()      ;
                assert( p != null_ptr<typename iterator_base::value_ptr>() )     ;
                return p->m_Data.first  ;
            }

            value_ref val() const
            {
                typename iterator_base::value_ptr p = iterator_base::operator ->()      ;
                assert( p != null_ptr<typename iterator_base::value_ptr>() )     ;
                return p->m_Data.second ;
            }

            pair_ptr operator ->() const
            {
                typename iterator_base::value_ptr p = iterator_base::operator ->()      ;
                return p ? &(p->m_Data) : null_ptr<pair_ptr>() ;
            }

            pair_ref operator *() const
            {
                typename iterator_base::value_ref p = iterator_base::operator *()      ;
                return p.m_Data ;
            }

            /// Pre-increment
            iterator_type& operator ++()
            {
                iterator_base::operator ++()    ;
                return *this;
            }

            template <bool C>
            bool operator ==(iterator_type<C> const& i ) const
            {
                return iterator_base::operator ==(i)    ;
            }
            template <bool C>
            bool operator !=(iterator_type<C> const& i ) const
            {
                return iterator_base::operator !=(i)    ;
            }
        };
        //@endcond

    public:
        /// Forward iterator
        typedef iterator_type<false>    iterator        ;

        /// Const forward iterator
        typedef iterator_type<true>     const_iterator  ;

        /// Returns a forward iterator addressing the first element in a list
        /**
            For empty list \code begin() == end() \endcode
        */
        iterator begin()
        {
            iterator it( head() )    ;
            ++it ;  // skip dummy head
            return it;
        }

        /// Returns an iterator that addresses the location succeeding the last element in a list
        /**
            Do not use the value returned by <tt>end</tt> function to access any item.
            Internally, <tt>end</tt> returning value pointing to dummy tail node.

            The returned value can be used only to control reaching the end of the list.
            For empty list \code begin() == end() \endcode
        */
        iterator end()
        {
            return iterator( tail() )   ;
        }

        /// Returns a forward const iterator addressing the first element in a list
        //@{
        const_iterator begin() const
        {
            const_iterator it( head() )    ;
            ++it;   // skip dummy head
            return it;
        }
        const_iterator cbegin()
        {
            const_iterator it( head() )    ;
            ++it;   // skip dummy head
            return it;
        }
        //@}

        /// Returns an const iterator that addresses the location succeeding the last element in a list
        //@{
        const_iterator end() const
        {
            return const_iterator( tail())   ;
        }
        const_iterator cend()
        {
            return const_iterator( tail())   ;
        }
        //@}

    public:
        /// Default constructor
        /**
            Initializes empty list
        */
        LazyKVList()
        {}

        /// List destructor
        /**
            Clears the list
        */
        ~LazyKVList()
        {
            clear() ;
        }

        /// Inserts new node with key and default value
        /**
            The function creates a node with \p key and default value, and then inserts the node created into the list.

            Preconditions:
            - The \ref key_type should be constructible from value of type \p K.
                In trivial case, \p K is equal to \ref key_type.
            - The \ref mapped_type should be default-constructible.

            The function makes RCU lock internally.

            Returns \p true if inserting successful, \p false otherwise.
        */
        template <typename K>
        bool insert( const K& key )
        {
            return insert_at( head(), key )    ;
        }

        /// Inserts new node with a key and a value
        /**
            The function creates a node with \p key and value \p val, and then inserts the node created into the list.

            Preconditions:
            - The \ref key_type should be constructible from \p key of type \p K.
            - The \ref mapped_type should be constructible from \p val of type \p V.

            The function makes RCU lock internally.

            Returns \p true if inserting successful, \p false otherwise.
        */
        template <typename K, typename V>
        bool insert( const K& key, const V& val )
        {
            return insert_at( head(), key, val )    ;
        }

        /// Inserts new node and initializes it by a functor
        /**
            This function inserts new node with key \p key and if inserting is successful then it calls
            \p func functor with signature
            \code
                struct functor {
                    void operator()( value_type& item )  ;
                };
            \endcode

            The argument \p item of user-defined functor \p func is the reference
            to the list's item inserted. <tt>item.second</tt> is a reference to item's value that may be changed.
            User-defined functor \p func should guarantee that during changing item's value no any other changes
            could be made on this list's item by concurrent threads.
            The user-defined functor can be passed by reference using <tt>boost::ref</tt>
            and it is called only if inserting is successful.

            The key_type should be constructible from value of type \p K.

            The function allows to split creating of new item into two part:
            - create item from \p key;
            - insert new item into the list;
            - if inserting is successful, initialize the value of item by calling \p func functor

            This can be useful if complete initialization of object of \p mapped_type is heavyweight and
            it is preferable that the initialization should be completed only if inserting is successful.

            The function makes RCU lock internally.
        */
        template <typename K, typename Func>
        bool insert_key( const K& key, Func func )
        {
            return insert_key_at( head(), key, func )    ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        /// Inserts data of type \ref mapped_type constructed with <tt>std::forward<Args>(args)...</tt>
        /**
            Returns \p true if inserting successful, \p false otherwise.

            The function makes RCU lock internally.

            @note This function is available only for compiler that supports
            variadic template and move semantics
        */
        template <typename... Args>
        bool emplace( Args&&... args )
        {
            return emplace_at( head(), std::forward<Args>(args)... )  ;
        }
#   endif

        /// Ensures that the \p key exists in the list
        /**
            The operation performs inserting or changing data with lock-free manner.

            If the \p key not found in the list, then the new item created from \p key
            is inserted into the list (note that in this case the \ref key_type should be
            copy-constructible from type \p K).
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

            The functor may change any fields of the \p item.second that is \ref mapped_type;
            however, \p func must guarantee that during changing no any other modifications
            could be made on this item by concurrent threads.

            You may pass \p func argument by reference using <tt>boost::ref</tt>.

            The function makes RCU lock internally.

            Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
            \p second is true if new item has been added or \p false if the item with \p key
            already is in the list.
        */
        template <typename K, typename Func>
        std::pair<bool, bool> ensure( const K& key, Func f )
        {
            return ensure_at( head(), key, f ) ;
        }

        /// Deletes \p key from the list
        /** \anchor cds_nonintrusive_LazyKVList_rcu_erase

            RCU \p synchronize method can be called. RCU should not be locked.

            Returns \p true if \p key is found and has been deleted, \p false otherwise
        */
        template <typename K>
        bool erase( K const& key )
        {
            return erase_at( head(), key, intrusive_key_comparator() ) ;
        }

        /// Deletes the item from the list using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_LazyKVList_rcu_erase "erase(K const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename K, typename Less>
        bool erase_with( K const& key, Less pred )
        {
            return erase_at( head(), key, typename options::template less_wrapper<Less>::type() ) ;
        }

        /// Deletes \p key from the list
        /** \anchor cds_nonintrusive_LazyKVList_rcu_erase_func
            The function searches an item with key \p key, calls \p f functor
            and deletes the item. If \p key is not found, the functor is not called.

            The functor \p Func interface:
            \code
            struct extractor {
                void operator()(value_type& val) { ... }
            };
            \endcode
            The functor may be passed by reference with <tt>boost:ref</tt>

            RCU \p synchronize method can be called. RCU should not be locked.

            Returns \p true if key is found and deleted, \p false otherwise
        */
        template <typename K, typename Func>
        bool erase( K const& key, Func f )
        {
            return erase_at( head(), key, intrusive_key_comparator(), f ) ;
        }

        /// Deletes the item from the list using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_LazyKVList_rcu_erase_func "erase(K const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename K, typename Less, typename Func>
        bool erase_with( K const& key, Less pred, Func f )
        {
            return erase_at( head(), key, typename options::template less_wrapper<Less>::type(), f ) ;
        }

        /// Extracts an item from the list
        /**
        @anchor cds_nonintrusive_LazyKVList_rcu_extract
            The function searches an item with key equal to \p key in the list,
            unlinks it from the list, and returns pointer to an item found in \p dest argument.
            If \p key is not found the function returns \p false.

            @note The function does NOT call RCU read-side lock or synchronization,
            and does NOT dispose the item found. It just excludes the item from the list
            and returns a pointer to item found.
            You should lock RCU before calling this function.

            \code
            #include <cds/urcu/general_buffered.h>
            #include <cds/container/lazy_kvlist_rcu.h>

            typedef cds::urcu::gc< general_buffered<> > rcu ;
            typedef cds::container::LazyKVList< rcu, int, Foo > rcu_lazy_list ;

            rcu_lazy_list theList ;
            // ...

            rcu_lazy_list::exempt_ptr p;
            {
                // first, we should lock RCU
                rcu_lazy_list::rcu_lock sl;

                // Now, you can apply extract function
                // Note that you must not delete the item found inside the RCU lock
                if ( theList.extract( p, 10 )) {
                    // do something with p
                    ...
                }
            }
            // Outside RCU lock section we may safely release extracted pointer.
            // release() passes the pointer to RCU reclamation cycle.
            p.release();
            \endcode
        */
        template <typename K>
        bool extract( exempt_ptr& dest, K const& key )
        {
            dest = extract_at( head(), key, intrusive_key_comparator() ) ;
            return !dest.empty();
        }

        /// Extracts an item from the list using \p pred predicate for searching
        /**
            This function is the analog for \ref cds_nonintrusive_LazyKVList_rcu_extract "extract(exempt_ptr&, K const&)".
            The \p pred is a predicate used for key comparing.
            \p Less has the interface like \p std::less.
            \p pred must imply the same element order as \ref key_comparator.
        */
        template <typename K, typename Less>
        bool extract_with( exempt_ptr& dest, K const& key, Less pred )
        {
            dest = extract_at( head(), key, typename options::template less_wrapper<Less>::type() );
            return !dest.empty();
        }

        /// Finds the key \p key
        /** \anchor cds_nonintrusive_LazyKVList_rcu_find_val
            The function searches the item with key equal to \p key
            and returns \p true if it is found, and \p false otherwise

            The function applies RCU lock internally.
        */
        template <typename Q>
        bool find( Q const& key ) const
        {
            return find_at( head(), key, intrusive_key_comparator() )  ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_LazyKVList_rcu_find_val "find(Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename Q, typename Less>
        bool find_with( Q const& key, Less pred ) const
        {
            return find_at( head(), key, typename options::template less_wrapper<Less>::type() )  ;
        }

        /// Finds the key \p key and performs an action with it
        /** \anchor cds_nonintrusive_LazyKVList_rcu_find_func
            The function searches an item with key equal to \p key and calls the functor \p f for the item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item ) ;
            };
            \endcode
            where \p item is the item found.

            You may pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change <tt>item.second</tt> that is reference to value of node.
            Note that the function is only guarantee that \p item cannot be deleted during functor is executing.
            The function does not serialize simultaneous access to the list \p item. If such access is
            possible you must provide your own synchronization schema to exclude unsafe item modifications.

            The function applies RCU lock internally.

            The function returns \p true if \p key is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q const& key, Func f ) const
        {
            return find_at( head(), key, intrusive_key_comparator(), f )  ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_LazyKVList_rcu_find_func "find(Q const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q const& key, Less pred, Func f ) const
        {
            return find_at( head(), key, typename options::template less_wrapper<Less>::type(), f )  ;
        }

        /// Finds \p key and return the item found
        /** \anchor cds_nonintrusive_LazyKVList_rcu_get
            The function searches the item with \p key and returns the pointer to item found.
            If \p key is not found it returns \p NULL.

            Note the compare functor should accept a parameter of type \p K that can be not the same as \p key_type.

            RCU should be locked before call of this function.
            Returned item is valid only while RCU is locked:
            \code
            typedef cds::container::LazyKVList< cds::urcu::gc< cds::urcu::general_buffered<> >, int, foo, my_traits > ord_list;
            ord_list theList ;
            // ...
            {
                // Lock RCU
                ord_list::rcu_lock lock;

                ord_list::value_type * pVal = theList.get( 5 );
                if ( pVal ) {
                    // Deal with pVal
                    //...
                }
                // Unlock RCU by rcu_lock destructor
                // pVal can be freed at any time after RCU has been unlocked
            }
            \endcode
        */
        template <typename K>
        value_type * get( K const& key ) const
        {
            return get_at( head(), key, intrusive_key_comparator());
        }

        /// Finds \p key and return the item found
        /**
            The function is an analog of \ref cds_nonintrusive_LazyKVList_rcu_get "get(K const&)"
            but \p pred is used for comparing the keys.

            \p Less functor has the semantics like \p std::less but should take arguments of type \ref key_type and \p K
            in any order.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename K, typename Less>
        value_type * get_with( K const& key, Less pred ) const
        {
            return get_at( head(), key, typename options::template less_wrapper<Less>::type());
        }

        /// Checks if the list is empty
        bool empty() const
        {
            return base_class::empty()  ;
        }

        /// Returns list's item count
        /**
            The value returned depends on opt::item_counter option. For atomicity::empty_item_counter,
            this function always returns 0.

            <b>Warning</b>: even if you use real item counter and it returns 0, this fact is not mean that the list
            is empty. To check list emptyness use \ref empty() method.
        */
        size_t size() const
        {
            return base_class::size()   ;
        }

        /// Clears the list
        /**
            Post-condition: the list is empty
        */
        void clear()
        {
            base_class::clear() ;
        }

    protected:
        //@cond
        bool insert_node_at( head_type& refHead, node_type * pNode )
        {
            assert( pNode != null_ptr<node_type *>() ) ;
            scoped_node_ptr p( pNode )  ;

            if ( base_class::insert_at( &refHead, *p )) {
                p.release() ;
                return true ;
            }

            return false    ;
        }

        template <typename K>
        bool insert_at( head_type& refHead, const K& key )
        {
            return insert_node_at( refHead, alloc_node( key )) ;
        }

        template <typename K, typename V>
        bool insert_at( head_type& refHead, const K& key, const V& val )
        {
            return insert_node_at( refHead, alloc_node( key, val )) ;
        }

        template <typename K, typename Func>
        bool insert_key_at( head_type& refHead, const K& key, Func f )
        {
            scoped_node_ptr pNode( alloc_node( key )) ;

#   ifdef CDS_CXX11_LAMBDA_SUPPORT
            if ( base_class::insert_at( &refHead, *pNode, [&f](node_type& node){ cds::unref(f)( node.m_Data ); } ))
#   else
            insert_functor<Func>  wrapper( f ) ;
            if ( base_class::insert_at( &refHead, *pNode, cds::ref(wrapper) ))
#   endif
            {
                pNode.release() ;
                return true ;
            }
            return false    ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        template <typename... Args>
        bool emplace_at( head_type& refHead, Args&&... args )
        {
            return insert_node_at( refHead, alloc_node( std::forward<Args>(args)... )) ;
        }
#   endif

        template <typename K, typename Compare>
        bool erase_at( head_type& refHead, K const& key, Compare cmp )
        {
            return base_class::erase_at( &refHead, key, cmp )  ;
        }

        template <typename K, typename Compare, typename Func>
        bool erase_at( head_type& refHead, K const & key, Compare cmp, Func f )
        {
#   ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::erase_at( &refHead, key, cmp, [&f](node_type const & node){cds::unref(f)( const_cast<value_type&>(node.m_Data)); })  ;
#   else
            erase_functor<Func> wrapper( f )  ;
            return base_class::erase_at( &refHead, key, cmp, cds::ref(wrapper) )  ;
#   endif
        }

        template <typename K, typename Func>
        std::pair<bool, bool> ensure_at( head_type& refHead, const K& key, Func f )
        {
            scoped_node_ptr pNode( alloc_node( key )) ;

#   ifdef CDS_CXX11_LAMBDA_SUPPORT
            std::pair<bool, bool> ret = base_class::ensure_at( &refHead, *pNode,
                [&f]( bool bNew, node_type& node, node_type& ){ cds::unref(f)( bNew, node.m_Data ); }) ;
#   else
            ensure_functor<Func> wrapper( f )    ;
            std::pair<bool, bool> ret = base_class::ensure_at( &refHead, *pNode, cds::ref(wrapper)) ;
#   endif
            if ( ret.first && ret.second )
                pNode.release() ;

            return ret  ;
        }

        template <typename K, typename Compare>
        node_type * extract_at( head_type& refHead, K const& key, Compare cmp )
        {
            return base_class::extract_at( &refHead, key, cmp );
        }

        template <typename K, typename Compare>
        bool find_at( head_type& refHead, K const& key, Compare cmp ) const
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find_at( &refHead, key, cmp, [](node_type&, K const&) {} ) ;
#       else
            return base_class::find_at( &refHead, key, cmp, empty_find_functor() )  ;
#       endif
        }

        template <typename K, typename Compare, typename Func>
        bool find_at( head_type& refHead, K& key, Compare cmp, Func f ) const
        {
#   ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find_at( &refHead, key, cmp, [&f]( node_type& node, K& ){ cds::unref(f)( node.m_Data ); })  ;
#   else
            find_functor<Func>  wrapper( f ) ;
            return base_class::find_at( &refHead, key, cmp, cds::ref(wrapper) )  ;
#   endif
        }

        template <typename K, typename Compare>
        value_type * get_at( head_type& refHead, K const& val, Compare cmp ) const
        {
            node_type * pNode = base_class::get_at( &refHead, val, cmp );
            return pNode ? &pNode->m_Data : null_ptr<value_type *>();
        }

        //@endcond
    };

}}  // namespace cds::container

#endif  // #ifndef __CDS_CONTAINER_LAZY_KVLIST_RCU_H
