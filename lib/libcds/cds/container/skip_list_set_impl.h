//$$CDS-header$$

#ifndef __CDS_CONTAINER_SKIP_LIST_SET_IMPL_H
#define __CDS_CONTAINER_SKIP_LIST_SET_IMPL_H

#include <cds/details/binary_functor_wrapper.h>
#include <cds/gc/guarded_ptr.h>
#include <cds/container/details/guarded_ptr_cast.h>

namespace cds { namespace container {

    /// Lock-free skip-list set
    /** @ingroup cds_nonintrusive_set
        \anchor cds_nonintrusive_SkipListSet_hp

        The implementation of well-known probabilistic data structure called skip-list
        invented by W.Pugh in his papers:
            - [1989] W.Pugh Skip Lists: A Probabilistic Alternative to Balanced Trees
            - [1990] W.Pugh A Skip List Cookbook

        A skip-list is a probabilistic data structure that provides expected logarithmic
        time search without the need of rebalance. The skip-list is a collection of sorted
        linked list. Nodes are ordered by key. Each node is linked into a subset of the lists.
        Each list has a level, ranging from 0 to 32. The bottom-level list contains
        all the nodes, and each higher-level list is a sublist of the lower-level lists.
        Each node is created with a random top level (with a random height), and belongs
        to all lists up to that level. The probability that a node has the height 1 is 1/2.
        The probability that a node has the height N is 1/2 ** N (more precisely,
        the distribution depends on an random generator provided, but our generators
        have this property).

        The lock-free variant of skip-list is implemented according to book
            - [2008] M.Herlihy, N.Shavit "The Art of Multiprocessor Programming",
                chapter 14.4 "A Lock-Free Concurrent Skiplist"

        Template arguments:
        - \p GC - Garbage collector used.
        - \p T - type to be stored in the list.
        - \p Traits - type traits. See skip_list::type_traits for explanation.

        It is possible to declare option-based list with cds::container::skip_list::make_traits metafunction istead of \p Traits template
        argument.
        Template argument list \p Options of cds::container::skip_list::make_traits metafunction are:
        - opt::compare - key comparison functor. No default functor is provided.
            If the option is not specified, the opt::less is used.
        - opt::less - specifies binary predicate used for key comparison. Default is \p std::less<T>.
        - opt::item_counter - the type of item counting feature. Default is \ref atomicity::empty_item_counter that is no item counting.
        - opt::memory_model - C++ memory ordering model. Can be opt::v::relaxed_ordering (relaxed memory model, the default)
            or opt::v::sequential_consistent (sequentially consisnent memory model).
        - skip_list::random_level_generator - random level generator. Can be skip_list::xorshift, skip_list::turbo_pascal or
            user-provided one. See skip_list::random_level_generator option description for explanation.
            Default is \p %skip_list::turbo_pascal.
        - opt::allocator - allocator for skip-list node. Default is \ref CDS_DEFAULT_ALLOCATOR.
        - opt::back_off - back-off strategy used. If the option is not specified, the cds::backoff::Default is used.
        - opt::stat - internal statistics. Available types: skip_list::stat, skip_list::empty_stat (the default)

        \warning The skip-list requires up to 67 hazard pointers that may be critical for some GCs for which
            the guard count is limited (like as gc::HP, gc::HRC). Those GCs should be explicitly initialized with
            hazard pointer enough: \code cds::gc::HP myhp( 67 ) \endcode. Otherwise an run-time exception may be raised
            when you try to create skip-list object.

        \note There are several specializations of \p %SkipListSet for each \p GC. You should include:
        - <tt><cds/container/skip_list_set_hp.h></tt> for gc::HP garbage collector
        - <tt><cds/container/skip_list_set_hrc.h></tt> for gc::HRC garbage collector
        - <tt><cds/container/skip_list_set_ptb.h></tt> for gc::PTB garbage collector
        - <tt><cds/container/skip_list_set_rcu.h></tt> for \ref cds_nonintrusive_SkipListSet_rcu "RCU type"
        - <tt><cds/container/skip_list_set_nogc.h></tt> for \ref cds_nonintrusive_SkipListSet_nogc "non-deletable SkipListSet"

        <b>Iterators</b>

        The class supports a forward iterator (\ref iterator and \ref const_iterator).
        The iteration is ordered.
        The iterator object is thread-safe: the element pointed by the iterator object is guarded,
        so, the element cannot be reclaimed while the iterator object is alive.
        However, passing an iterator object between threads is dangerous.

        \warning Due to concurrent nature of skip-list set it is not guarantee that you can iterate
        all elements in the set: any concurrent deletion can exclude the element
        pointed by the iterator from the set, and your iteration can be terminated
        before end of the set. Therefore, such iteration is more suitable for debugging purpose only

        Remember, each iterator object requires 2 additional hazard pointers, that may be
        a limited resource for \p GC like as gc::HP and gc::HRC (for gc::PTB the count of
        guards is unlimited).

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

    */
    template <
        typename GC,
        typename T,
#ifdef CDS_DOXYGEN_INVOKED
        typename Traits = skip_list::type_traits
#else
        typename Traits
#endif
    >
    class SkipListSet:
#ifdef CDS_DOXYGEN_INVOKED
        protected intrusive::SkipListSet< GC, T, Traits >
#else
        protected details::make_skip_list_set< GC, T, Traits >::type
#endif
    {
        //@cond
        typedef details::make_skip_list_set< GC, T, Traits >    maker ;
        typedef typename maker::type base_class ;
        //@endcond
    public:
        typedef typename base_class::gc          gc  ; ///< Garbage collector used
        typedef T       value_type  ;   ///< @anchor cds_containewr_SkipListSet_value_type Value type stored in the set
        typedef Traits  options     ;   ///< Options specified

        typedef typename base_class::back_off       back_off        ;   ///< Back-off strategy used
        typedef typename options::allocator         allocator_type  ;   ///< Allocator type used for allocate/deallocate the skip-list nodes
        typedef typename base_class::item_counter   item_counter    ;   ///< Item counting policy used
        typedef typename maker::key_comparator      key_comparator  ;   ///< key comparison functor
        typedef typename base_class::memory_model   memory_model    ;   ///< Memory ordering. See cds::opt::memory_model option
        typedef typename options::random_level_generator random_level_generator ; ///< random level generator
        typedef typename options::stat              stat            ;   ///< internal statistics type

    protected:
        //@cond
        typedef typename maker::node_type           node_type       ;
        typedef typename maker::node_allocator      node_allocator  ;

        typedef std::unique_ptr< node_type, typename maker::node_deallocator >    scoped_node_ptr ;
        //@endcond

    public:
        /// Guarded pointer
        typedef cds::gc::guarded_ptr< gc, node_type, value_type, details::guarded_ptr_cast_set<node_type, value_type> > guarded_ptr ;

    protected:
        //@cond
        unsigned int random_level()
        {
            return base_class::random_level() ;
        }
        //@endcond

    protected:
        //@cond
#   ifndef CDS_CXX11_LAMBDA_SUPPORT
        template <typename Func>
        struct insert_functor
        {
            Func        m_func  ;

            insert_functor ( Func f )
                : m_func(f)
            {}

            void operator()( node_type& node )
            {
                cds::unref(m_func)( node.m_Value )  ;
            }
        };

        template <typename Q, typename Func>
        struct ensure_functor
        {
            Func        m_func  ;
            Q const&    m_arg   ;

            ensure_functor( Q const& arg, Func f )
                : m_func(f)
                , m_arg( arg )
            {}

            void operator ()( bool bNew, node_type& node, node_type& )
            {
                cds::unref(m_func)( bNew, node.m_Value, m_arg ) ;
            }
        };

        template <typename Func>
        struct find_functor
        {
            Func    m_func  ;

            find_functor( Func f )
                : m_func(f)
            {}

            template <typename Q>
            void operator ()( node_type& node, Q& val )
            {
                cds::unref(m_func)( node.m_Value, val ) ;
            }
        };

        struct copy_value_functor {
            template <typename Q>
            void operator()( Q& dest, value_type const& src ) const
            {
                dest = src;
            }
        };

        template <typename Func>
        struct erase_functor
        {
            Func        m_func  ;

            erase_functor( Func f )
                : m_func(f)
            {}

            void operator()( node_type const& node )
            {
                cds::unref(m_func)( node.m_Value )  ;
            }
        };
#   endif  // ifndef CDS_CXX11_LAMBDA_SUPPORT

        //@endcond

    public:
        /// Default ctor
        SkipListSet()
            : base_class()
        {}

        /// Destructor destroys the set object
        ~SkipListSet()
        {}

    public:
        /// Iterator type
        typedef skip_list::details::iterator< typename base_class::iterator >  iterator        ;

        /// Const iterator type
        typedef skip_list::details::iterator< typename base_class::const_iterator >   const_iterator  ;

        /// Returns a forward iterator addressing the first element in a set
        iterator begin()
        {
            return iterator( base_class::begin() )    ;
        }

        /// Returns a forward const iterator addressing the first element in a set
        const_iterator begin() const
        {
            return const_iterator( base_class::begin() ) ;
        }

        /// Returns a forward const iterator addressing the first element in a set
        const_iterator cbegin()
        {
            return const_iterator( base_class::cbegin() ) ;
        }

        /// Returns a forward iterator that addresses the location succeeding the last element in a set.
        iterator end()
        {
            return iterator( base_class::end() )   ;
        }

        /// Returns a forward const iterator that addresses the location succeeding the last element in a set.
        const_iterator end() const
        {
            return const_iterator( base_class::end() ) ;
        }

        /// Returns a forward const iterator that addresses the location succeeding the last element in a set.
        const_iterator cend()
        {
            return const_iterator( base_class::cend() ) ;
        }

    public:
        /// Inserts new node
        /**
            The function creates a node with copy of \p val value
            and then inserts the node created into the set.

            The type \p Q should contain as minimum the complete key for the node.
            The object of \ref value_type should be constructible from a value of type \p Q.
            In trivial case, \p Q is equal to \ref value_type.

            Returns \p true if \p val is inserted into the set, \p false otherwise.
        */
        template <typename Q>
        bool insert( Q const& val )
        {
            scoped_node_ptr sp( node_allocator().New( random_level(), val )) ;
            if ( base_class::insert( *sp.get() )) {
                sp.release()    ;
                return true     ;
            }
            return false ;
        }

        /// Inserts new node
        /**
            The function allows to split creating of new item into two part:
            - create item with key only
            - insert new item into the set
            - if inserting is success, calls  \p f functor to initialize value-fields of \p val.

            The functor signature is:
            \code
                void func( value_type& val ) ;
            \endcode
            where \p val is the item inserted. User-defined functor \p f should guarantee that during changing
            \p val no any other changes could be made on this set's item by concurrent threads.
            The user-defined functor is called only if the inserting is success. It may be passed by reference
            using <tt>boost::ref</tt>
        */
        template <typename Q, typename Func>
        bool insert( Q const& val, Func f )
        {
            scoped_node_ptr sp( node_allocator().New( random_level(), val )) ;
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            if ( base_class::insert( *sp.get(), [&f]( node_type& val ) { cds::unref(f)( val.m_Value ); } ))
#       else
            insert_functor<Func> wrapper(f) ;
            if ( base_class::insert( *sp, cds::ref(wrapper) ))
#       endif
            {
                sp.release()    ;
                return true     ;
            }
            return false ;
        }

        /// Ensures that the item exists in the set
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
            - \p val - argument \p key passed into the \p ensure function

            The functor may change non-key fields of the \p item; however, \p func must guarantee
            that during changing no any other modifications could be made on this item by concurrent threads.

            You may pass \p func argument by reference using <tt>boost::ref</tt>.

            Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
            \p second is true if new item has been added or \p false if the item with \p key
            already is in the set.
        */
        template <typename Q, typename Func>
        std::pair<bool, bool> ensure( const Q& val, Func func )
        {
            scoped_node_ptr sp( node_allocator().New( random_level(), val )) ;
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            std::pair<bool, bool> bRes = base_class::ensure( *sp,
                [&func, &val](bool bNew, node_type& node, node_type&){ cds::unref(func)( bNew, node.m_Value, val ); }) ;
#       else
            ensure_functor<Q, Func> wrapper( val, func )    ;
            std::pair<bool, bool> bRes = base_class::ensure( *sp, cds::ref(wrapper)) ;
#       endif
            if ( bRes.first && bRes.second )
                sp.release() ;
            return bRes ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        /// Inserts data of type \ref cds_containewr_SkipListSet_value_type "value_type" constructed with <tt>std::forward<Args>(args)...</tt>
        /**
            Returns \p true if inserting successful, \p false otherwise.

            @note This function is available only for compiler that supports
            variadic template and move semantics
        */
        template <typename... Args>
        bool emplace( Args&&... args )
        {
            scoped_node_ptr sp( node_allocator().New( random_level(), std::forward<Args>(args)... )) ;
            if ( base_class::insert( *sp.get() )) {
                sp.release()    ;
                return true     ;
            }
            return false ;
        }
#   endif

        /// Delete \p key from the set
        /** \anchor cds_nonintrusive_SkipListSet_erase_val

            The set item comparator should be able to compare the type \p value_type
            and the type \p Q.

            Return \p true if key is found and deleted, \p false otherwise
        */
        template <typename Q>
        bool erase( Q const& key )
        {
            return base_class::erase( key ) ;
        }

        /// Deletes the item from the set using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListSet_erase_val "erase(Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool erase_with( Q const& key, Less pred )
        {
            return base_class::erase_with( key, cds::details::predicate_wrapper< node_type, Less, typename maker::value_accessor >() ) ;
        }

        /// Delete \p key from the set
        /** \anchor cds_nonintrusive_SkipListSet_erase_func

            The function searches an item with key \p key, calls \p f functor
            and deletes the item. If \p key is not found, the functor is not called.

            The functor \p Func interface:
            \code
            struct extractor {
                void operator()(value_type const& val) ;
            };
            \endcode
            The functor may be passed by reference using <tt>boost:ref</tt>

            Since the key of \p value_type is not explicitly specified,
            template parameter \p Q defines the key type to search in the list.
            The list item comparator should be able to compare the type \p T of list item
            and the type \p Q.

            Return \p true if key is found and deleted, \p false otherwise

            See also: \ref erase
        */
        template <typename Q, typename Func>
        bool erase( Q const& key, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::erase( key, [&f]( node_type const& node) { cds::unref(f)( node.m_Value ); } ) ;
#       else
            erase_functor<Func> wrapper(f) ;
            return base_class::erase( key, cds::ref(wrapper)) ;
#       endif
        }

        /// Deletes the item from the set using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListSet_erase_func "erase(Q const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool erase_with( Q const& key, Less pred, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::erase_with( key, cds::details::predicate_wrapper< node_type, Less, typename maker::value_accessor >(),
                [&f]( node_type const& node) { cds::unref(f)( node.m_Value ); } ) ;
#       else
            erase_functor<Func> wrapper(f) ;
            return base_class::erase_with( key, cds::details::predicate_wrapper< node_type, Less, typename maker::value_accessor >(),
                cds::ref(wrapper)) ;
#       endif
        }

        /// Extracts the item from the set with specified \p key
        /** \anchor cds_nonintrusive_SkipListSet_hp_extract
            The function searches an item with key equal to \p key in the set,
            unlinks it from the set, and returns it in \p ptr parameter.
            If the item with key equal to \p key is not found the function returns \p false.

            Note the compare functor should accept a parameter of type \p Q that can be not the same as \p value_type.

            The item extracted is freed automatically
            by garbage collector \p GC when returned \ref guarded_ptr object
            will be destroyed or released.
            @note Each \p guarded_ptr object uses the GC's guard that can be limited resource.

            Usage:
            \code
            typedef cds::container::SkipListSet< cds::gc::HP, foo, my_traits >  skip_list;
            skip_list theList;
            // ...
            {
                skip_list::guarded_ptr gp;
                if ( theList.extract( gp, 5 ) ) {
                    // Deal with gp
                    // ...
                }
                // Destructor of gp releases internal HP guard and frees the pointer
            }
            \endcode
        */
        template <typename Q>
        bool extract( guarded_ptr& ptr, Q const& key )
        {
            return base_class::extract_( ptr.guard(), key, typename base_class::key_comparator() );
        }

        /// Extracts the item from the set with comparing functor \p pred
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListSet_hp_extract "extract(Q const&)"
            but \p pred predicate is used for key comparing.

            \p Less functor has the semantics like \p std::less but should take arguments of type \ref value_type and \p Q
            in any order.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool extract_with( guarded_ptr& ptr, Q const& key, Less pred )
        {
            typedef cds::details::predicate_wrapper< node_type, Less, typename maker::value_accessor >  wrapped_less ;
            return base_class::extract_( ptr.guard(), key, cds::opt::details::make_comparator_from_less<wrapped_less>() ) ;
        }

        /// Extracts an item with minimal key from the set
        /** 
            The function searches an item with minimal key, unlinks it, and returns the item found in \p ptr parameter.
            If the skip-list is empty the function returns \p false.

            The item extracted is freed automatically by garbage collector \p GC 
            when returned \ref guarded_ptr object will be destroyed or released.
            @note Each \p guarded_ptr object uses the GC's guard that can be limited resource.

            Usage:
            \code
            typedef cds::continer::SkipListSet< cds::gc::HP, foo, my_traits >  skip_list;
            skip_list theList;
            // ...
            {
                skip_list::guarded_ptr gp;
                if ( theList.extract_min( gp )) {
                    // Deal with gp
                    //...
                }
                // Destructor of gp releases internal HP guard and then frees the pointer
            }
            \endcode
        */
        bool extract_min( guarded_ptr& ptr)
        {
            return base_class::extract_min_( ptr.guard() ) ;
        }

        /// Extracts an item with minimal key from the map
        /** \anchor cds_container_SkipListSet_hp_extract_min
            The function searches an item with minimal key, unlinks it, 
            and copy it to \p dest by \p fnCopy functor. Then, minimal item is freed by \p GC
            The functor \p Func has the following interface:
            \code
            struct copy_functor {
                void operator()( Q& dest, value_type& src ) ;
            };
            \endcode
            The functor can be passed by reference with \p %cds::ref or \p boost::ref.

            If the skip-list is empty the function returns \p false, otherwise - \p true.
        */
        template <typename Q, typename Func >
        bool extract_min( Q& dest, Func fnCopy )
        {
            typename gc::Guard gDel ;
            if ( base_class::extract_min_( gDel )) {
                cds::unref(fnCopy)( dest, gDel.template get<node_type>()->m_Value ) ;
                return true ;
            }
            return false ;
        }

        /// Extracts an item with minimal key from the map
        /** 
            The function is similar to \ref cds_container_SkipListSet_hp_extract_min "extract_min(Q& dest, Func fnCopy)"
            but an assignment operator is used to copy value found to \p dest.
            So, the value of type \p value_type should be assignable to the value of type \p Q.
        */
        template <typename Q >
        bool extract_min( Q& dest )
        {
            return extract_min( dest,
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
                [](Q& d, value_type const& src) { d = src; }
#       else
                copy_value_functor()
#       endif
                );
        }

        /// Extracts an item with maximal key from the set
        /** 
            The function searches an item with maximal key, unlinks it, and returns the pointer to item found in \p ptr parameter.
            If the skip-list is empty the function returns empty \p guarded_ptr.

            The item found is freed by garbage collector \p GC automatically 
            when returned \ref guarded_ptr object will be destroyed or released.
            @note Each \p guarded_ptr object uses the GC's guard that can be limited resource.

            Usage:
            \code
            typedef cds::container::SkipListSet< cds::gc::HP, foo, my_traits >  skip_list;
            skip_list theList;
            // ...
            {
                skip_list::guarded_ptr gp;
                if ( theList.extract_max( gp )) {
                    // Deal with gp
                    //...
                }
                // Destructor of gp releases internal HP guard and then frees the pointer
            }
            \endcode
        */
        bool extract_max( guarded_ptr& dest )
        {
            return base_class::extract_max_( dest.guard() ) ;
        }

        /// Extracts an item with maximal key from the set
        /** \anchor cds_nonintrusive_SkipListSet_hp_extract_max
            The function searches an item with maximal key, unlinks it, 
            and copy it to \p dest by \p fnCopy functor.
            The functor \p Func has the following interface:
            \code
            struct copy_functor {
                void operator()( Q& dest, value_type& src ) ;
            };
            \endcode
            The functor can be passed by reference with \p %cds::ref or \p boost::ref if it is necessary

            If the skip-list is empty the function returns \p false, otherwise - \p true.
        */
        template <typename Q, typename Func >
        bool extract_max( Q& dest, Func fnCopy )
        {
            typename gc::Guard gDel ;
            if ( base_class::extract_max_( gDel )) {
                cds::unref(fnCopy)( dest, gDel.template get<node_type>()->m_Value ) ;
                return true ;
            }
            return false ;
        }

        /// Extracts an item with maximal key from the list
        /** 
            The function is similar to \ref cds_nonintrusive_SkipListSet_hp_extract_max "extract_max(Q& dest, Func fnCopy)"
            but an assignment operator is used to copy value found to \p dest.
            So, the value of type \p value_type should be assignable to the value of type \p Q.

            The \ref disposer specified in \p Traits class template parameter is called
            by garbage collector \p GC automatically.
        */
        template <typename Q >
        bool extract_max( Q& dest )
        {
            return extract_max( dest,
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
                [](Q& dest, value_type const& src) { dest = src; }
#       else
                copy_value_functor()
#       endif
                );
        }

        /// Find the key \p val
        /** \anchor cds_nonintrusive_SkipListSet_find_func

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
            can modify both arguments.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that may be not the same as \p value_type.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q& val, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find( val, [&f]( node_type& node, Q& v ) { cds::unref(f)( node.m_Value, v ); });
#       else
            find_functor<Func> wrapper(f) ;
            return base_class::find( val, cds::ref(wrapper)) ;
#       endif
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListSet_find_func "find(Q&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q& val, Less pred, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find_with( val, cds::details::predicate_wrapper< node_type, Less, typename maker::value_accessor >(),
                [&f]( node_type& node, Q& v ) { cds::unref(f)( node.m_Value, v ); } );
#       else
            find_functor<Func> wrapper(f) ;
            return base_class::find_with( val, cds::details::predicate_wrapper< node_type, Less, typename maker::value_accessor >(), cds::ref(wrapper)) ;
#       endif
        }

        /// Find the key \p val
        /** \anchor cds_nonintrusive_SkipListSet_find_cfunc

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
            should accept a parameter of type \p Q that may be not the same as \p value_type.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q const& val, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find( val, [&f]( node_type& node, Q const& v ) { cds::unref(f)( node.m_Value, v ); });
#       else
            find_functor<Func> wrapper(f) ;
            return base_class::find( val, cds::ref(wrapper)) ;
#       endif
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListSet_find_cfunc "find(Q const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q const& val, Less cmp, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find_with( val, cds::details::predicate_wrapper< node_type, Less, typename maker::value_accessor >(),
                [&f]( node_type& node, Q const& v ) { cds::unref(f)( node.m_Value, v ); } );
#       else
            find_functor<Func> wrapper(f) ;
            return base_class::find_with( val, cds::details::predicate_wrapper< node_type, Less, typename maker::value_accessor >(),
                cds::ref(wrapper)) ;
#       endif
        }

        /// Find the key \p val
        /** \anchor cds_nonintrusive_SkipListSet_find_val

            The function searches the item with key equal to \p val
            and returns \p true if it is found, and \p false otherwise.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that may be not the same as \ref value_type.
        */
        template <typename Q>
        bool find( Q const& val )
        {
            return base_class::find( val )  ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListSet_find_val "find(Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool find_with( Q const& val, Less pred )
        {
            return base_class::find_with( val, cds::details::predicate_wrapper< node_type, Less, typename maker::value_accessor >());
        }

        /// Finds the key \p val and return the item found
        /** \anchor cds_nonintrusive_SkipListSet_hp_get
            The function searches the item with key equal to \p val
            and assigns the item found to guarded pointer \p ptr.
            The function returns \p true if \p val is found, and \p false otherwise.
            If \p val is not found the \p ptr parameter is not changed.

            It is safe when a concurrent thread erases the item returned in \p ptr guarded pointer.
            In this case the item will be freed later by garbage collector \p GC automatically
            when \p guarded_ptr object will be destroyed or released.
            @note Each \p guarded_ptr object uses one GC's guard which can be limited resource.

            Usage:
            \code
            typedef cds::container::SkipListSet< cds::gc::HP, foo, my_traits >  skip_list;
            skip_list theList;
            // ...
            {
                skip_list::guarded_ptr gp;
                if ( theList.get( gp, 5 ) ) {
                    // Deal with gp
                    //...
                }
                // Destructor of guarded_ptr releases internal HP guard
            }
            \endcode

            Note the compare functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q>
        bool get( guarded_ptr& ptr, Q const& val )
        {
            return base_class::get_with_( ptr.guard(), val, typename base_class::key_comparator() );
        }

        /// Finds the key \p val and return the item found
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListSet_hp_get "get( guarded_ptr& ptr, Q const&)"
            but \p pred is used for comparing the keys.

            \p Less functor has the semantics like \p std::less but should take arguments of type \ref value_type and \p Q
            in any order.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool get_with( guarded_ptr& ptr, Q const& val, Less pred )
        {
            typedef cds::details::predicate_wrapper< node_type, Less, typename maker::value_accessor >  wrapped_less ;
            return base_class::get_with_( ptr.guard(), val, cds::opt::details::make_comparator_from_less< wrapped_less >());
        }

        /// Gets minimum key from the set
        /** 
            The function searches an item with minimal key and returns the item found in \p ptr parameter.
            If the skip-list is empty the function returns \p false.
            @note Each \p guarded_ptr object uses the GC's guard that can be limited resource.

            Usage:
            \code
            typedef cds::container::SkipListSet< cds::gc::HP, foo, my_traits >  skip_list;
            skip_list theList;
            // ...
            {
                skip_list::guarded_ptr gp;
                if ( theList.get_min( gp )) {
                    // Deal with gp
                    //...
                }
                // Destructor of guarded_ptr releases internal HP guard
            }
            \endcode
        */
        bool get_min( guarded_ptr& ptr )
        {
            return base_class::get_min_( ptr.guard() );
        }

        /// Gets maximum key from the set
        /** 
            The function searches an item with maximal key and returns the item found in \p ptr parameter.
            If the skip-list is empty the function returns \p false.
            @note Each \p guarded_ptr object uses the GC's guard that can be limited resource.

            Usage:
            \code
            typedef cds::container::SkipListSet< cds::gc::HP, foo, my_traits >  skip_list;
            skip_list theList;
            // ...
            {
                skip_list::guarded_ptr gp;
                if ( theList.get_max( gp )) {
                    // Deal with gp
                    //...
                }
                // Destructor of guarded_ptr releases internal HP guard
            }
            \endcode
        */
        bool get_max( guarded_ptr& ptr )
        {
            return base_class::get_max_( ptr.guard() );
        }

        /// Clears the set (non-atomic).
        /**
            The function deletes all items from the set.
            The function is not atomic, thus, in multi-threaded environment with parallel insertions
            this sequence
            \code
            set.clear() ;
            assert( set.empty() ) ;
            \endcode
            the assertion could be raised.

            For each item the \ref disposer provided by \p Traits template parameter will be called.
        */
        void clear()
        {
            base_class::clear() ;
        }

        /// Checks if the set is empty
        bool empty() const
        {
            return base_class::empty() ;
        }

        /// Returns item count in the set
        /**
            The value returned depends on item counter type provided by \p Traits template parameter.
            If it is atomicity::empty_item_counter this function always returns 0.
            Therefore, the function is not suitable for checking the set emptiness, use \ref empty
            member function for this purpose.
        */
        size_t size() const
        {
            return base_class::size() ;
        }

        /// Returns const reference to internal statistics
        stat const& statistics() const
        {
            return base_class::statistics() ;
        }

    };

}} // namespace cds::container

#endif // #ifndef __CDS_CONTAINER_SKIP_LIST_SET_IMPL_H
