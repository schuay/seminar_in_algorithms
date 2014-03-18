//$$CDS-header$$

#ifndef __CDS_CONTAINER_MICHAEL_KVLIST_IMPL_H
#define __CDS_CONTAINER_MICHAEL_KVLIST_IMPL_H

#include <cds/ref.h>
#include <cds/details/functor_wrapper.h>
#include <cds/details/std/memory.h>
#include <cds/container/details/guarded_ptr_cast.h>

namespace cds { namespace container {

    /// Michael's ordered list (key-value pair)
    /** @ingroup cds_nonintrusive_list
        \anchor cds_nonintrusive_MichaelKVList_gc

        This is key-value variation of non-intrusive MichaelList.
        Like standard container, this implementation split a value stored into two part -
        constant key and alterable value.

        Usually, ordered single-linked list is used as a building block for the hash table implementation.
        The complexity of searching is <tt>O(N)</tt>.

        Template arguments:
        - \p GC - garbage collector used
        - \p Key - key type of an item stored in the list. It should be copy-constructible
        - \p Value - value type stored in a list
        - \p Traits - type traits, default is michael_list::type_traits

        It is possible to declare option-based list with cds::container::michael_list::make_traits metafunction istead of \p Traits template
        argument. For example, the following traits-based declaration of gc::HP Michael's list
        \code
        #include <cds/container/michael_kvlist_hp.h>
        // Declare comparator for the item
        struct my_compare {
            int operator ()( int i1, int i2 )
            {
                return i1 - i2  ;
            }
        };

        // Declare type_traits
        struct my_traits: public cds::container::michael_list::type_traits
        {
            typedef my_compare compare ;
        };

        // Declare traits-based list
        typedef cds::container::MichaelKVList< cds::gc::HP, int, int, my_traits >     traits_based_list   ;
        \endcode

        is equivalent for the following option-based list
        \code
        #include <cds/container/michael_kvlist_hp.h>

        // my_compare is the same

        // Declare option-based list
        typedef cds::container::MichaelKVList< cds::gc::HP, int, int,
            typename cds::container::michael_list::make_traits<
                cds::container::opt::compare< my_compare >     // item comparator option
            >::type
        >     option_based_list   ;
        \endcode

        Template argument list \p Options of cds::container::michael_list::make_traits metafunction are:
        - opt::compare - key comparison functor. No default functor is provided.
            If the option is not specified, the opt::less is used.
        - opt::less - specifies binary predicate used for key comparison. Default is \p std::less<T>.
        - opt::back_off - back-off strategy used. If the option is not specified, the cds::backoff::empty is used.
        - opt::item_counter - the type of item counting feature. Default is \ref atomicity::empty_item_counter that is no item counting.
        - opt::allocator - the allocator used for creating and freeing list's item. Default is \ref CDS_DEFAULT_ALLOCATOR macro.
        - opt::memory_model - C++ memory ordering model. Can be opt::v::relaxed_ordering (relaxed memory model, the default)
            or opt::v::sequential_consistent (sequentially consisnent memory model).

        \par Usage
        There are different specializations of this template for each garbage collecting schema used.
        You should include appropriate .h-file depending on GC you are using:
        - for gc::HP: \code #include <cds/container/michael_kvlist_hp.h> \endcode
        - for gc::PTB: \code #include <cds/container/michael_kvlist_ptb.h> \endcode
        - for gc::HRC: \code #include <cds/container/michael_kvlist_hrc.h> \endcode
        - for \ref cds_urcu_desc "RCU": \code #include <cds/container/michael_kvlist_rcu.h> \endcode
        - for gc::nogc: \code #include <cds/container/michael_kvlist_nogc.h> \endcode
    */
    template <
        typename GC,
        typename Key,
        typename Value,
#ifdef CDS_DOXYGEN_INVOKED
        typename Traits = michael_list::type_traits
#else
        typename Traits
#endif
    >
    class MichaelKVList:
#ifdef CDS_DOXYGEN_INVOKED
        protected intrusive::MichaelList< GC, implementation_defined, Traits >
#else
        protected details::make_michael_kvlist< GC, Key, Value, Traits >::type
#endif
    {
        //@cond
        typedef details::make_michael_kvlist< GC, Key, Value, Traits > options   ;
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

    protected:
        //@cond
        typedef typename base_class::value_type     node_type       ;
        typedef typename options::cxx_allocator     cxx_allocator   ;
        typedef typename options::node_deallocator  node_deallocator;
        typedef typename options::type_traits::compare  intrusive_key_comparator    ;

        typedef typename base_class::atomic_node_ptr head_type      ;
        //@endcond

    public:
        /// Guarded pointer
        typedef cds::gc::guarded_ptr< gc, node_type, value_type, details::guarded_ptr_cast_map<node_type, value_type> > guarded_ptr ;

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

#   ifdef CDS_EMPLACE_SUPPORT
        template <typename K, typename... Args>
        static node_type * alloc_node( K&& key, Args&&... args )
        {
            return cxx_allocator().MoveNew( std::forward<K>(key), std::forward<Args>(args)...) ;
        }
#   endif

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
            return base_class::m_pHead  ;
        }

        head_type const& head() const
        {
            return base_class::m_pHead  ;
        }
        //@endcond

    protected:
        //@cond
        template <bool IsConst>
        class iterator_type: protected base_class::template iterator_type<IsConst>
        {
            typedef typename base_class::template iterator_type<IsConst>    iterator_base ;

            iterator_type( head_type const& pNode )
                : iterator_base( pNode )
            {}

            friend class MichaelKVList ;

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
                assert( p != null_ptr<typename iterator_base::value_ptr>() ) ;
                return p->m_Data.first  ;
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

            value_ref val() const
            {
                typename iterator_base::value_ptr p = iterator_base::operator ->()      ;
                assert( p != null_ptr<typename iterator_base::value_ptr>() ) ;
                return p->m_Data.second ;
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
        /**
            The forward iterator for Michael's list has some features:
            - it has no post-increment operator
            - to protect the value, the iterator contains a GC-specific guard + another guard is required locally for increment operator.
              For some GC (gc::HP, gc::HRC), a guard is limited resource per thread, so an exception (or assertion) "no free guard"
              may be thrown if a limit of guard count per thread is exceeded.
            - The iterator cannot be moved across thread boundary since it contains GC's guard that is thread-private GC data.
            - Iterator ensures thread-safety even if you delete the item that iterator points to. However, in case of concurrent
              deleting operations it is no guarantee that you iterate all item in the list.

            Therefore, the use of iterators in concurrent environment is not good idea. Use the iterator on the concurrent container
            for debug purpose only.

            The iterator interface to access item data:
            - <tt> operator -> </tt> - returns a pointer to \ref value_type for iterator
            - <tt> operator *</tt> - returns a reference (a const reference for \p const_iterator) to \ref value_type for iterator
            - <tt> const key_type& key() </tt> - returns a key reference for iterator
            - <tt> mapped_type& val() </tt> - retuns a value reference for iterator (const reference for \p const_iterator)

            For both functions the iterator should not be equal to <tt> end() </tt>
        */
        typedef iterator_type<false>    iterator        ;

        /// Const forward iterator
        /**
            For iterator's features and requirements see \ref iterator
        */
        typedef iterator_type<true>     const_iterator  ;

        /// Returns a forward iterator addressing the first element in a list
        /**
            For empty list \code begin() == end() \endcode
        */
        iterator begin()
        {
            return iterator( head() )    ;
        }

        /// Returns an iterator that addresses the location succeeding the last element in a list
        /**
            Do not use the value returned by <tt>end</tt> function to access any item.
            Internally, <tt>end</tt> returning value equals to <tt>NULL</tt>.

            The returned value can be used only to control reaching the end of the list.
            For empty list \code begin() == end() \endcode
        */
        iterator end()
        {
            return iterator()   ;
        }

        /// Returns a forward const iterator addressing the first element in a list
        //@{
        const_iterator begin() const
        {
            return const_iterator( head() )    ;
        }
        const_iterator cbegin()
        {
            return const_iterator( head() )    ;
        }
        //@}

        /// Returns an const iterator that addresses the location succeeding the last element in a list
        //@{
        const_iterator end() const
        {
            return const_iterator()   ;
        }
        const_iterator cend()
        {
            return const_iterator()   ;
        }
        //@}

    public:
        /// Default constructor
        /**
            Initializes empty list
        */
        MichaelKVList()
        {}

        /// List desctructor
        /**
            Clears the list
        */
        ~MichaelKVList()
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

            Returns \p true if inserting successful, \p false otherwise.
        */
        template <typename K, typename V>
        bool insert( const K& key, const V& val )
        {
            // We cannot use insert with functor here
            // because we cannot lock inserted node for updating
            // Therefore, we use separate function
            return insert_at( head(), key, val )    ;
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
        */
        template <typename K, typename Func>
        bool insert_key( const K& key, Func func )
        {
            return insert_key_at( head(), key, func )    ;
        }

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

            Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
            \p second is true if new item has been added or \p false if the item with \p key
            already is in the list.
        */
        template <typename K, typename Func>
        std::pair<bool, bool> ensure( const K& key, Func f )
        {
            return ensure_at( head(), key, f ) ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        /// Inserts data of type \ref mapped_type constructed with <tt>std::forward<Args>(args)...</tt>
        /**
            Returns \p true if inserting successful, \p false otherwise.

            @note This function is available only for compiler that supports
            variadic template and move semantics
        */
        template <typename K, typename... Args>
        bool emplace( K&& key, Args&&... args )
        {
            return emplace_at( head(), std::forward<K>(key), std::forward<Args>(args)... ) ;
        }
#   endif

        /// Deletes \p key from the list
        /** \anchor cds_nonintrusive_MichaelKVList_hp_erase_val

            Returns \p true if \p key is found and has been deleted, \p false otherwise
        */
        template <typename K>
        bool erase( K const& key )
        {
            return erase_at( head(), key, intrusive_key_comparator() ) ;
        }

        /// Deletes the item from the list using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_MichaelKVList_hp_erase_val "erase(K const&)"
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
        /** \anchor cds_nonintrusive_MichaelKVList_hp_erase_func
            The function searches an item with key \p key, calls \p f functor
            and deletes the item. If \p key is not found, the functor is not called.

            The functor \p Func interface:
            \code
            struct extractor {
                void operator()(value_type& val) { ... }
            };
            \endcode
            The functor may be passed by reference with <tt>boost:ref</tt>

            Return \p true if key is found and deleted, \p false otherwise

            See also: \ref erase
        */
        template <typename K, typename Func>
        bool erase( K const& key, Func f )
        {
            return erase_at( head(), key, intrusive_key_comparator(), f ) ;
        }

        /// Deletes the item from the list using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_MichaelKVList_hp_erase_func "erase(K const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename K, typename Less, typename Func>
        bool erase_with( K const& key, Less pred, Func f )
        {
            return erase_at( head(), key, typename options::template less_wrapper<Less>::type(), f ) ;
        }

        /// Extracts the item from the list with specified \p key
        /** \anchor cds_nonintrusive_MichaelKVList_hp_extract
            The function searches an item with key equal to \p key,
            unlinks it from the list, and returns it in \p dest parameter.
            If the item with key equal to \p key is not found the function returns \p false.

            Note the compare functor should accept a parameter of type \p K that can be not the same as \p key_type.

            The \ref disposer specified in \p Traits class template parameter is called automatically
            by garbage collector \p GC specified in class' template parameters when returned \ref guarded_ptr object
            will be destroyed or released.
            @note Each \p guarded_ptr object uses the GC's guard that can be limited resource.

            Usage:
            \code
            typedef cds::container::MichaelKVList< cds::gc::HP, int, foo, my_traits >  ord_list;
            ord_list theList;
            // ...
            {
                ord_list::guarded_ptr gp;
                theList.extract( gp, 5 );
                // Deal with gp
                // ...

                // Destructor of gp releases internal HP guard
            }
            \endcode
        */
        template <typename K>
        bool extract( guarded_ptr& dest, K const& key )
        {
            return extract_at( head(), dest.guard(), key, intrusive_key_comparator() );
        }

        /// Extracts the item from the list with comparing functor \p pred
        /**
            The function is an analog of \ref cds_nonintrusive_MichaelKVList_hp_extract "extract(guarded_ptr&, K const&)"
            but \p pred predicate is used for key comparing.

            \p Less functor has the semantics like \p std::less but should take arguments of type \ref key_type and \p K
            in any order.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename K, typename Less>
        bool extract_with( guarded_ptr& dest, K const& key, Less pred )
        {
            return extract_at( head(), dest.guard(), key, typename options::template less_wrapper<Less>::type() );
        }

        /// Finds the key \p key
        /** \anchor cds_nonintrusive_MichaelKVList_hp_find_val
            The function searches the item with key equal to \p key
            and returns \p true if it is found, and \p false otherwise
        */
        template <typename Q>
        bool find( Q const& key )
        {
            return find_at( head(), key, intrusive_key_comparator() )  ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_MichaelKVList_hp_find_val "find(Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename Q, typename Less>
        bool find_with( Q const& key, Less pred )
        {
            return find_at( head(), key, typename options::template less_wrapper<Less>::type() )  ;
        }

        /// Finds the key \p key and performs an action with it
        /** \anchor cds_nonintrusive_MichaelKVList_hp_find_func
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

            The function returns \p true if \p key is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q const& key, Func f )
        {
            return find_at( head(), key, intrusive_key_comparator(), f )  ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_MichaelKVList_hp_find_func "find(Q&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q const& key, Less pred, Func f )
        {
            return find_at( head(), key, typename options::template less_wrapper<Less>::type(), f )  ;
        }

        /// Finds the \p key and return the item found
        /** \anchor cds_nonintrusive_MichaelKVList_hp_get
            The function searches the item with key equal to \p key
            and assigns the item found to guarded pointer \p ptr.
            The function returns \p true if \p key is found, and \p false otherwise.
            If \p key is not found the \p ptr parameter is not changed.

            The \ref disposer specified in \p Traits class template parameter is called
            by garbage collector \p GC automatically when returned \ref guarded_ptr object
            will be destroyed or released.
            @note Each \p guarded_ptr object uses one GC's guard which can be limited resource.

            Usage:
            \code
            typedef cds::container::MichaelKVList< cds::gc::HP, int, foo, my_traits >  ord_list;
            ord_list theList;
            // ...
            {
                ord_list::guarded_ptr gp;
                if ( theList.get( gp, 5 )) {
                    // Deal with gp
                    //...
                }
                // Destructor of guarded_ptr releases internal HP guard
            }
            \endcode

            Note the compare functor specified for class \p Traits template parameter
            should accept a parameter of type \p K that can be not the same as \p key_type.
        */
        template <typename K>
        bool get( guarded_ptr& ptr, K const& key )
        {
            return get_at( head(), ptr.guard(), key, intrusive_key_comparator() );
        }

        /// Finds the \p key and return the item found
        /**
            The function is an analog of \ref cds_nonintrusive_MichaelKVList_hp_get "get( guarded_ptr& ptr, K const&)"
            but \p pred is used for comparing the keys.

            \p Less functor has the semantics like \p std::less but should take arguments of type \ref key_type and \p K
            in any order.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename K, typename Less>
        bool get_with( guarded_ptr& ptr, K const& key, Less pred )
        {
            return get_at( head(), ptr.guard(), key, typename options::template less_wrapper<Less>::type() ) ;
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
            scoped_node_ptr p( pNode ) ;
            if ( base_class::insert_at( refHead, *pNode )) {
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
            scoped_node_ptr pNode( alloc_node( key ))  ;

#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            if ( base_class::insert_at( refHead, *pNode, [&f](node_type& node){ cds::unref(f)( node.m_Data ); }))
#       else
            insert_functor<Func>  wrapper( f ) ;
            if ( base_class::insert_at( refHead, *pNode, cds::ref(wrapper) ))
#       endif
            {
                pNode.release() ;
                return true ;
            }
            return false    ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        template <typename K, typename... Args>
        bool emplace_at( head_type& refHead, K&& key, Args&&... args )
        {
            return insert_node_at( refHead, alloc_node( std::forward<K>(key), std::forward<Args>(args)... )) ;
        }
#   endif

        template <typename K, typename Func>
        std::pair<bool, bool> ensure_at( head_type& refHead, const K& key, Func f )
        {
            scoped_node_ptr pNode( alloc_node( key ))  ;

#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            std::pair<bool, bool> ret = base_class::ensure_at( refHead, *pNode,
                [&f]( bool bNew, node_type& node, node_type& ){ cds::unref(f)( bNew, node.m_Data ); }) ;
#       else
            ensure_functor<Func> wrapper( f )    ;
            std::pair<bool, bool> ret = base_class::ensure_at( refHead, *pNode, cds::ref(wrapper)) ;
#       endif
            if ( ret.first && ret.second )
                pNode.release() ;

            return ret  ;
        }

        template <typename K, typename Compare>
        bool erase_at( head_type& refHead, K const& key, Compare cmp )
        {
            return base_class::erase_at( refHead, key, cmp )  ;
        }

        template <typename K, typename Compare, typename Func>
        bool erase_at( head_type& refHead, K const& key, Compare cmp, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::erase_at( refHead, key, cmp, [&f]( node_type const & node ){ cds::unref(f)( const_cast<value_type&>(node.m_Data)); })  ;
#       else
            erase_functor<Func> wrapper( f )  ;
            return base_class::erase_at( refHead, key, cmp, cds::ref(wrapper) )  ;
#       endif
        }
        template <typename K, typename Compare>
        bool extract_at( head_type& refHead, typename gc::Guard& dest, K const& key, Compare cmp )
        {
            return base_class::extract_at( refHead, dest, key, cmp );
        }

        template <typename K, typename Compare>
        bool find_at( head_type& refHead, K const& key, Compare cmp )
        {
            return base_class::find_at( refHead, key, cmp ) ;
        }

        template <typename K, typename Compare, typename Func>
        bool find_at( head_type& refHead, K& key, Compare cmp, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find_at( refHead, key, cmp, [&f](node_type& node, K const&){ cds::unref(f)( node.m_Data ); })  ;
#       else
            find_functor<Func>  wrapper( f ) ;
            return base_class::find_at( refHead, key, cmp, cds::ref(wrapper) )  ;
#       endif
        }

        template <typename K, typename Compare>
        bool get_at( head_type& refHead, typename gc::Guard& guard, K const& key, Compare cmp )
        {
            return base_class::get_at( refHead, guard, key, cmp ) ;
        }

        //@endcond
    };

}}  // namespace cds::container

#endif  // #ifndef __CDS_CONTAINER_MICHAEL_KVLIST_IMPL_H
