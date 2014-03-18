//$$CDS-header$$

#ifndef __CDS_CONTAINER_SPLIT_LIST_SET_RCU_H
#define __CDS_CONTAINER_SPLIT_LIST_SET_RCU_H

#include <cds/intrusive/split_list_rcu.h>
#include <cds/container/details/make_split_list_set.h>
#include <cds/details/functor_wrapper.h>

namespace cds { namespace container {

    /// Split-ordered list set (template specialization for \ref cds_urcu_desc "RCU")
    /** @ingroup cds_nonintrusive_set
        \anchor cds_nonintrusive_SplitListSet_rcu

        Hash table implementation based on split-ordered list algorithm discovered by Ori Shalev and Nir Shavit, see
        - [2003] Ori Shalev, Nir Shavit "Split-Ordered Lists - Lock-free Resizable Hash Tables"
        - [2008] Nir Shavit "The Art of Multiprocessor Programming"

        See intrusive::SplitListSet for a brief description of the split-list algorithm.

        Template parameters:
        - \p RCU - one of \ref cds_urcu_gc "RCU type"
        - \p T - type stored in the split-list. The type must be default- and copy-constructible.
        - \p Traits - type traits, default is split_list::type_traits. Instead of declaring split_list::type_traits -based
            struct you may apply option-based notation with split_list::make_traits metafunction.

        <b>Iterators</b>

        The class supports a forward iterator (\ref iterator and \ref const_iterator).
        The iteration is ordered.

        You may iterate over split-list set items only under RCU lock.
        Only in this case the iterator is thread-safe since
        while RCU is locked any set's item cannot be reclaimed.

        The requirement of RCU lock during iterating means that deletion of the elements (i.e. \ref erase)
        is not possible.

        @warning The iterator object cannot be passed between threads

        \warning Due to concurrent nature of skip-list set it is not guarantee that you can iterate
        all elements in the set: any concurrent deletion can exclude the element
        pointed by the iterator from the set, and your iteration can be terminated
        before end of the set. Therefore, such iteration is more suitable for debugging purposes

        The iterator class supports the following minimalistic interface:
        \code
        struct iterator {
            // Default ctor
            iterator();

            // Copy ctor
            iterator( iterator const& s) ;

            value_type * operator ->() const ;
            value_type& operator *() const ;

            // Pre-increment
            iterator& operator ++() ;

            // Copy assignment
            iterator& operator = (const iterator& src) ;

            bool operator ==(iterator const& i ) const ;
            bool operator !=(iterator const& i ) const ;
        };
        \endcode
        Note, the iterator object returned by \ref end, \p cend member functions points to \p NULL and should not be dereferenced.

        \par Usage

        You should decide what garbage collector you want, and what ordered list you want to use. Split-ordered list
        is an original data structure based on an ordered list. Suppose, you want construct split-list set based on cds::urcu::general_buffered<> GC
        and LazyList as ordered list implementation. So, you beginning your program with following include:
        \code
        #include <cds/urcu/general_buffered.h>
        #include <cds/container/lazy_list_rcu.h>
        #include <cds/container/split_list_set_rcu.h>

        namespace cc = cds::container ;

        // The data belonged to split-ordered list
        sturuct foo {
            int     nKey;   // key field
            std::string strValue    ;   // value field
        };
        \endcode
        The inclusion order is important:
        - first, include one of \ref cds_urcu_gc "RCU implementation" (<tt>cds/urcu/general_buffered.h</tt> in our case)
        - second, include file for ordered-list implementation (for this example, <tt>cds/container/lazy_list_rcu.h</tt>),
        - then, the header for RCU-based split-list set <tt>cds/container/split_list_set_rcu.h</tt>.

        Now, you should declare traits for split-list set. The main parts of traits are a hash functor for the set and a comparing functor for ordered list.
        Note that we define several function in <tt>foo_hash</tt> and <tt>foo_less</tt> functors for different argument types since we want call our \p %SplitListSet
        object by the key of type <tt>int</tt> and by the value of type <tt>foo</tt>.

        The second attention: instead of using LazyList in SplitListSet traits we use a tag <tt>cds::contaner::lazy_list_tag</tt> for the lazy list.
        The split-list requires significant support from underlying ordered list class and it is not good idea to dive you
        into deep implementation details of split-list and ordered list interrelations. The tag paradigm simplifies split-list interface.

        \code
        // foo hash functor
        struct foo_hash {
            size_t operator()( int key ) const { return std::hash( key ) ; }
            size_t operator()( foo const& item ) const { return std::hash( item.nKey ) ; }
        };

        // foo comparator
        struct foo_less {
            bool operator()(int i, foo const& f ) const { return i < f.nKey ; }
            bool operator()(foo const& f, int i ) const { return f.nKey < i ; }
            bool operator()(foo const& f1, foo const& f2) const { return f1.nKey < f2.nKey; }
        };

        // SplitListSet traits
        struct foo_set_traits: public cc::split_list::type_traits
        {
            typedef cc::lazy_list_tag   ordered_list    ;   // what type of ordered list we want to use
            typedef foo_hash            hash            ;   // hash functor for our data stored in split-list set

            // Type traits for our LazyList class
            struct ordered_list_traits: public cc::lazy_list::type_traits
            {
                typedef foo_less less   ;   // use our foo_less as comparator to order list nodes
            } ;
        };
        \endcode

        Now you are ready to declare our set class based on \p %SplitListSet:
        \code
        typedef cc::SplitListSet< cds::urcu::gc<cds::urcu::general_buffered<> >, foo, foo_set_traits > foo_set   ;
        \endcode

        You may use the modern option-based declaration instead of classic type-traits-based one:
        \code
        typedef cc:SplitListSet<
            cds::urcu::gc<cds::urcu::general_buffered<> >   // RCU type used
            ,foo                                            // type of data stored
            ,cc::split_list::make_traits<      // metafunction to build split-list traits
                cc::split_list::ordered_list<cc::lazy_list_tag>     // tag for underlying ordered list implementation
                ,cc::opt::hash< foo_hash >              // hash functor
                ,cc::split_list::ordered_list_traits<   // ordered list traits desired
                    cc::lazy_list::make_traits<         // metafunction to build lazy list traits
                        cc::opt::less< foo_less >           // less-based compare functor
                    >::type
                >
            >::type
        >  foo_set ;
        \endcode
        In case of option-based declaration using split_list::make_traits metafunction
        the struct \p foo_set_traits is not required.

        Now, the set of type \p foo_set is ready to use in your program.

        Note that in this example we show only mandatory type_traits parts, optional ones is the default and they are inherited
        from cds::container::split_list::type_traits.
        The <b>cds</b> library contains many other options for deep tuning of behavior of the split-list and
        ordered-list containers.
    */
    template <
        class RCU,
        class T,
#ifdef CDS_DOXYGEN_INVOKED
        class Traits = split_list::type_traits
#else
        class Traits
#endif
    >
    class SplitListSet< cds::urcu::gc< RCU >, T, Traits >:
#ifdef CDS_DOXYGEN_INVOKED
        protected intrusive::SplitListSet< cds::urcu::gc< RCU >, typename Traits::ordered_list, Traits >
#else
        protected details::make_split_list_set< cds::urcu::gc< RCU >, T, typename Traits::ordered_list, split_list::details::wrap_set_traits<T, Traits> >::type
#endif
    {
    protected:
        //@cond
        typedef details::make_split_list_set< cds::urcu::gc< RCU >, T, typename Traits::ordered_list, split_list::details::wrap_set_traits<T, Traits> > maker ;
        typedef typename maker::type  base_class   ;
        //@endcond

    public:
        typedef Traits                            options         ; ///< \p Traits template argument
        typedef typename maker::gc                gc              ; ///< Garbage collector
        typedef typename maker::value_type        value_type      ; ///< type of value stored in the list
        typedef typename maker::ordered_list      ordered_list    ; ///< Underlying ordered list class
        typedef typename base_class::key_comparator key_comparator; ///< key compare functor

        /// Hash functor for \ref value_type and all its derivatives that you use
        typedef typename base_class::hash           hash            ;
        typedef typename base_class::item_counter   item_counter    ;   ///< Item counter type

        typedef typename base_class::rcu_lock      rcu_lock   ; ///< RCU scoped lock
        typedef typename base_class::exempt_ptr    exempt_ptr ; ///< pointer to extracted node

    protected:
        //@cond
        typedef typename maker::cxx_node_allocator    cxx_node_allocator  ;
        typedef typename maker::node_type             node_type           ;

        template <typename Q>
        static node_type * alloc_node(Q const& v )
        {
            return cxx_node_allocator().New( v )    ;
        }

        template <typename Q, typename Func>
        bool find_( Q& val, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find( val, [&f]( node_type& item, Q& val ) { cds::unref(f)(item.m_Value, val) ; } );
#       else
            find_functor_wrapper<Func> fw(f) ;
            return base_class::find( val, cds::ref(fw) )   ;
#       endif
        }

        template <typename Q, typename Less, typename Func>
        bool find_with_( Q& val, Less pred, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find_with( val, typename maker::template predicate_wrapper<Less>::type(),
                [&f]( node_type& item, Q& val ) { cds::unref(f)(item.m_Value, val) ; } );
#       else
            find_functor_wrapper<Func> fw(f) ;
            return base_class::find_with( val, typename maker::template predicate_wrapper<Less>::type(), cds::ref(fw) )   ;
#       endif
        }


#   ifdef CDS_EMPLACE_SUPPORT
        template <typename... Args>
        static node_type * alloc_node( Args&&... args )
        {
            return cxx_node_allocator().MoveNew( std::forward<Args>(args)...) ;
        }
#   endif

        static void free_node( node_type * pNode )
        {
            cxx_node_allocator().Delete( pNode ) ;
        }

        struct node_disposer {
            void operator()( node_type * pNode )
            {
                free_node( pNode )  ;
            }
        };
        typedef std::unique_ptr< node_type, node_disposer >     scoped_node_ptr ;

        bool insert_node( node_type * pNode )
        {
            assert( pNode != null_ptr<node_type *>() ) ;
            scoped_node_ptr p(pNode) ;

            if ( base_class::insert( *pNode ) ) {
                p.release() ;
                return true ;
            }

            return false        ;
        }

        //@endcond

    protected:
        //@cond
#   ifndef CDS_CXX11_LAMBDA_SUPPORT
        template <typename Func>
        class insert_functor_wrapper: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            insert_functor_wrapper( Func f ): base_class(f) {}

            void operator()(node_type& node)
            {
                base_class::get()( node.m_Value ) ;
            }
        };

        template <typename Func, typename Q>
        class ensure_functor_wrapper: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
            Q const&    m_val ;
        public:
            ensure_functor_wrapper( Func f, Q const& v ): base_class(f), m_val(v) {}

            void operator()( bool bNew, node_type& item, node_type const& /*val*/ )
            {
                base_class::get()( bNew, item.m_Value, m_val ) ;
            }
        };

        template <typename Func>
        class find_functor_wrapper: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            find_functor_wrapper( Func f ): base_class(f) {}

            template <typename Q>
            void operator()( node_type& item, Q& val )
            {
                base_class::get()( item.m_Value, val ) ;
            }
        };

        struct empty_find_functor
        {
            template <typename Q>
            void operator()( node_type&, Q& )
            {}
        };

        template <typename Func>
        class erase_functor_wrapper: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            erase_functor_wrapper( Func f ): base_class( f ) {}

            void operator()(node_type& node)
            {
                base_class::get()( node.m_Value ) ;
            }
        };
#   endif // ifndef CDS_CXX11_LAMBDA_SUPPORT
        //@endcond

    protected:
        /// Forward iterator
        /**
            \p IsConst - constness boolean flag

            The forward iterator for a split-list has the following features:
            - it has no post-increment operator
            - it depends on underlying ordered list iterator
            - it is safe to iterate only inside RCU critical section
            - deleting an item pointed by the iterator can cause to deadlock

            Therefore, the use of iterators in concurrent environment is not good idea. 
            Use it for debug purpose only.
        */
        template <bool IsConst>
        class iterator_type: protected base_class::template iterator_type<IsConst>
        {
            //@cond
            typedef typename base_class::template iterator_type<IsConst> iterator_base_class ;
            friend class SplitListSet   ;
            //@endcond
        public:
            /// Value pointer type (const for const iterator)
            typedef typename cds::details::make_const_type<value_type, IsConst>::pointer   value_ptr;
            /// Value reference type (const for const iterator)
            typedef typename cds::details::make_const_type<value_type, IsConst>::reference value_ref;

        public:
            /// Default ctor
            iterator_type()
            {}

            /// Copy ctor
            iterator_type( iterator_type const& src )
                : iterator_base_class( src )
            {}

        protected:
            //@cond
            explicit iterator_type( iterator_base_class const& src )
                : iterator_base_class( src )
            {}
            //@endcond

        public:
            /// Dereference operator
            value_ptr operator ->() const
            {
                return &(iterator_base_class::operator->()->m_Value) ;
            }

            /// Dereference operator
            value_ref operator *() const
            {
                return iterator_base_class::operator*().m_Value  ;
            }

            /// Pre-increment
            iterator_type& operator ++()
            {
                iterator_base_class::operator++() ;
                return *this    ;
            }

            /// Assignment operator
            iterator_type& operator = (iterator_type const& src)
            {
                iterator_base_class::operator=(src)  ;
                return *this    ;
            }

            /// Equality operator
            template <bool C>
            bool operator ==(iterator_type<C> const& i ) const
            {
                return iterator_base_class::operator==(i)  ;
            }

            /// Equality operator
            template <bool C>
            bool operator !=(iterator_type<C> const& i ) const
            {
                return iterator_base_class::operator!=(i)  ;
            }
        };

    public:
        /// Initializes split-ordered list of default capacity
        /**
            The default capacity is defined in bucket table constructor.
            See intrusive::split_list::expandable_bucket_table, intrusive::split_list::static_bucket_table
            which selects by intrusive::split_list::dynamic_bucket_table option.
        */
        SplitListSet()
            : base_class()
        {}

        /// Initializes split-ordered list
        SplitListSet(
            size_t nItemCount           ///< estimate average of item count
            , size_t nLoadFactor = 1    ///< load factor - average item count per bucket. Small integer up to 8, default is 1.
            )
            : base_class( nItemCount, nLoadFactor )
        {}

    public:
        typedef iterator_type<false>  iterator        ; ///< Forward iterator
        typedef iterator_type<true>   const_iterator  ; ///< Forward const iterator

        /// Returns a forward iterator addressing the first element in a set
        /**
            For empty set \code begin() == end() \endcode
        */
        iterator begin()
        {
            return iterator( base_class::begin() )   ;
        }

        /// Returns an iterator that addresses the location succeeding the last element in a set
        /**
            Do not use the value returned by <tt>end</tt> function to access any item.
            The returned value can be used only to control reaching the end of the set.
            For empty set \code begin() == end() \endcode
        */
        iterator end()
        {
            return iterator( base_class::end() )    ;
        }

        /// Returns a forward const iterator addressing the first element in a set
        const_iterator begin() const
        {
            return const_iterator( base_class::begin() )    ;
        }

        /// Returns an const iterator that addresses the location succeeding the last element in a set
        const_iterator end() const
        {
            return const_iterator( base_class::end() )  ;
        }

    public:
        /// Inserts new node
        /**
            The function creates a node with copy of \p val value
            and then inserts the node created into the set.

            The type \p Q should contain as minimum the complete key for the node.
            The object of \p value_type should be constructible from a value of type \p Q.
            In trivial case, \p Q is equal to \p value_type.

            The function applies RCU lock internally.

            Returns \p true if \p val is inserted into the set, \p false otherwise.
        */
        template <typename Q>
        bool insert( Q const& val )
        {
            return insert_node( alloc_node( val ) ) ;
        }

        /// Inserts new node
        /**
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
            The user-defined functor is called only if the inserting is success. It may be passed by reference
            using <tt>boost::ref</tt>

            The function applies RCU lock internally.
        */
        template <typename Q, typename Func>
        bool insert( Q const& val, Func f )
        {
            scoped_node_ptr pNode( alloc_node( val )) ;

#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            if ( base_class::insert( *pNode, [&f](node_type& node) { cds::unref(f)( node.m_Value ) ; } ))
#       else
            insert_functor_wrapper<Func> fw(f) ;
            if ( base_class::insert( *pNode, cds::ref(fw) ) )
#       endif
            {
                pNode.release() ;
                return true ;
            }
            return false        ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        /// Inserts data of type \p value_type constructed with <tt>std::forward<Args>(args)...</tt>
        /**
            Returns \p true if inserting successful, \p false otherwise.

            The function applies RCU lock internally.

            @note This function is available only for compiler that supports
            variadic template and move semantics.
        */
        template <typename... Args>
        bool emplace( Args&&... args )
        {
            return insert_node( alloc_node( std::forward<Args>(args)...)) ;
        }
#   endif

        /// Ensures that the \p item exists in the set
        /**
            The operation performs inserting or changing data with lock-free manner.

            If the \p val key not found in the set, then the new item created from \p val
            is inserted into the set. Otherwise, the functor \p func is called with the item found.
            The functor \p Func should be a function with signature:
            \code
                void func( bool bNew, value_type& item, const Q& val ) ;
            \endcode
            or a functor:
            \code
                struct my_functor {
                    void operator()( bool bNew, value_type& item, const Q& val ) ;
                };
            \endcode

            with arguments:
            - \p bNew - \p true if the item has been inserted, \p false otherwise
            - \p item - item of the set
            - \p val - argument \p val passed into the \p ensure function

            The functor may change non-key fields of the \p item; however, \p func must guarantee
            that during changing no any other modifications could be made on this item by concurrent threads.

            You may pass \p func argument by reference using <tt>boost::ref</tt>.

            The function applies RCU lock internally.

            Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
            \p second is true if new item has been added or \p false if the item with \p key
            already is in the set.
        */
        template <typename Q, typename Func>
        std::pair<bool, bool> ensure( Q const& val, Func func )
        {
            scoped_node_ptr pNode( alloc_node( val )) ;

#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            std::pair<bool, bool> bRet = base_class::ensure( *pNode,
                [&func, &val]( bool bNew, node_type& item,  node_type const& /*val*/ ) {
                    cds::unref(func)( bNew, item.m_Value, val ) ;
                } );
#       else
            ensure_functor_wrapper<Func, Q> fw( func, val ) ;
            std::pair<bool, bool> bRet = base_class::ensure( *pNode, cds::ref(fw) ) ;
#       endif

            if ( bRet.first && bRet.second )
                pNode.release() ;
            return bRet ;
        }

        /// Deletes \p key from the set
        /** \anchor cds_nonintrusive_SplitListSet_rcu_erase_val

            Since the key of SplitListSet's item type \p value_type is not explicitly specified,
            template parameter \p Q defines the key type searching in the list.
            The set item comparator should be able to compare the values of type \p value_type
            and the type \p Q.

            RCU \p synchronize method can be called. RCU should not be locked.

            Return \p true if key is found and deleted, \p false otherwise
        */
        template <typename Q>
        bool erase( Q const& key )
        {
            return base_class::erase( key ) ;
        }

        /// Deletes the item from the set using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SplitListSet_rcu_erase_val "erase(Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool erase_with( Q const& key, Less pred )
        {
             return base_class::erase_with( key, typename maker::template predicate_wrapper<Less>::type() ) ;
        }

        /// Deletes \p key from the set
        /** \anchor cds_nonintrusive_SplitListSet_rcu_erase_func

            The function searches an item with key \p key, calls \p f functor
            and deletes the item. If \p key is not found, the functor is not called.

            The functor \p Func interface:
            \code
            struct extractor {
                void operator()(value_type const& val) ;
            };
            \endcode
            The functor may be passed by reference using <tt>boost:ref</tt>

            Since the key of SplitListSet's \p value_type is not explicitly specified,
            template parameter \p Q defines the key type searching in the list.
            The list item comparator should be able to compare the values of the type \p value_type
            and the type \p Q.

            RCU \p synchronize method can be called. RCU should not be locked.

            Return \p true if key is found and deleted, \p false otherwise
        */
        template <typename Q, typename Func>
        bool erase( Q const& key, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::erase( key, [&f](node_type& node) { cds::unref(f)( node.m_Value ); } ) ;
#       else
            erase_functor_wrapper<Func> fw( f ) ;
            return base_class::erase( key, cds::ref(fw) ) ;
#       endif
        }

        /// Deletes the item from the set using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SplitListSet_rcu_erase_func "erase(Q const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool erase_with( Q const& key, Less pred, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::erase_with( key, typename maker::template predicate_wrapper<Less>::type(),
                [&f](node_type& node) { cds::unref(f)( node.m_Value ); } ) ;
#       else
            erase_functor_wrapper<Func> fw( f ) ;
            return base_class::erase_with( key, typename maker::template predicate_wrapper<Less>::type(), cds::ref(fw) ) ;
#       endif
        }

        /// Extracts an item from the set
        /** \anchor cds_nonintrusive_SplitListSet_rcu_extract
            The function searches an item with key equal to \p val in the set,
            unlinks it from the set, places item pointer into \p dest argument, and returns \p true.
            If the item with the key equal to \p val is not found the function return \p false.

            @note The function does NOT call RCU read-side lock or synchronization,
            and does NOT dispose the item found. It just excludes the item from the set
            and returns a pointer to item found.
            You should lock RCU before calling of the function, and you should synchronize RCU
            outside the RCU lock to free extracted item

            \code
            typedef cds::urcu::gc< general_buffered<> > rcu ;
            typedef cds::container::SplitListSet< rcu, Foo > splitlist_set;

            splitlist_set theSet ;
            // ...

            splitlist_set::exempt_ptr p;
            {
                // first, we should lock RCU
                splitlist_set::rcu_lock lock;

                // Now, you can apply extract function
                // Note that you must not delete the item found inside the RCU lock
                if ( theSet.extract( p, 10 )) {
                    // do something with p
                    ...
                }
            }

            // We may safely release p here
            // release() passes the pointer to RCU reclamation cycle
            p.release();
            \endcode
        */
        template <typename Q>
        bool extract( exempt_ptr& dest, Q const& val )
        {
        }

        /// Extracts an item from the set using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SplitListSet_rcu_extract "extract(exempt_ptr&, Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool extract_with( exempt_ptr& dest, Q const& val, Less pred )
        {
        }


        /// Finds the key \p val
        /** \anchor cds_nonintrusive_SplitListSet_rcu_find_func

            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You may pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change non-key fields of \p item. Note that the functor is only guarantee
            that \p item cannot be disposed during functor is executing.
            The functor does not serialize simultaneous access to the set's \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            The \p val argument is non-const since it can be used as \p f functor destination i.e., the functor
            may modify both arguments.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function makes RCU lock internally.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q& val, Func f )
        {
            return find_( val, f ) ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SplitListSet_rcu_find_func "find(Q&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q& val, Less pred, Func f )
        {
            return find_with_( val, pred, f ) ;
        }

        /// Find the key \p val
        /** \anchor cds_nonintrusive_SplitListSet_rcu_find_cfunc

            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q const& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You may pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change non-key fields of \p item. Note that the functor is only guarantee
            that \p item cannot be disposed during functor is executing.
            The functor does not serialize simultaneous access to the set's \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function makes RCU lock internally.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q const& val, Func f )
        {
            return find_( val, f ) ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SplitListSet_rcu_find_cfunc "find(Q const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q const& val, Less pred, Func f )
        {
            return find_with_( val, pred, f ) ;
        }

        /// Finds the key \p val
        /** \anchor cds_nonintrusive_SplitListSet_rcu_find_val

            The function searches the item with key equal to \p val
            and returns \p true if it is found, and \p false otherwise.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function makes RCU lock internally.
        */
        template <typename Q>
        bool find( Q const& val )
        {
            return base_class::find( val ) ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SplitListSet_rcu_find_val "find(Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool find_with( Q const& val, Less pred )
        {
            return base_class::find_with( val, typename maker::template predicate_wrapper<Less>::type() )   ;
        }

        /// Clears the set (non-atomic)
        /**
            The function unlink all items from the set.
            The function is not atomic and not lock-free and should be used for debugging only.

            RCU \p synchronize method can be called. RCU should not be locked.
        */
        void clear()
        {
            base_class::clear()   ;
        }

        /// Checks if the set is empty
        /**
            Emptiness is checked by item counting: if item count is zero then assume that the set is empty.
            Thus, the correct item counting feature is an important part of split-list set implementation.
        */
        bool empty() const
        {
            return base_class::empty()  ;
        }

        /// Returns item count in the set
        size_t size() const
        {
            return base_class::size()   ;
        }
    };


}}  // namespace cds::container

#endif // #ifndef __CDS_CONTAINER_SPLIT_LIST_SET_RCU_H
