//$$CDS-header$$

#ifndef __CDS_CONTAINER_SPLIT_LIST_MAP_H
#define __CDS_CONTAINER_SPLIT_LIST_MAP_H

#include <cds/container/split_list_set.h>
#include <cds/details/binary_functor_wrapper.h>

namespace cds { namespace container {

    /// Split-ordered list map
    /** @ingroup cds_nonintrusive_map
        \anchor cds_nonintrusive_SplitListMap_hp

        Hash table implementation based on split-ordered list algorithm discovered by Ori Shalev and Nir Shavit, see
        - [2003] Ori Shalev, Nir Shavit "Split-Ordered Lists - Lock-free Resizable Hash Tables"
        - [2008] Nir Shavit "The Art of Multiprocessor Programming"

        See intrusive::SplitListSet for a brief description of the split-list algorithm.

        Template parameters:
        - \p GC - Garbage collector used
        - \p Key - key type of an item stored in the map. It should be copy-constructible
        - \p Value - value type stored in the map
        - \p Traits - type traits, default is split_list::type_traits. Instead of declaring split_list::type_traits -based
            struct you may apply option-based notation with split_list::make_traits metafunction.

        There are the specializations:
        - for \ref cds_urcu_desc "RCU" - declared in <tt>cd/container/split_list_map_rcu.h</tt>,
            see \ref cds_nonintrusive_SplitListMap_rcu "SplitListMap<RCU>".
        - for \ref cds::gc::nogc declared in <tt>cds/container/split_list_map_nogc.h</tt>,
            see \ref cds_nonintrusive_SplitListMap_nogc "SplitListMap<gc::nogc>".

        \par Usage

        You should decide what garbage collector you want, and what ordered list you want to use. Split-ordered list
        is original data structure based on an ordered list. Suppose, you want construct split-list map based on gc::HP GC
        and MichaelList as ordered list implementation. Your map should map \p int key to <tt>std::string</tt> value.
        So, you beginning your program with following include:
        \code
        #include <cds/container/michael_list_hp.h>
        #include <cds/container/split_list_map.h>

        namespace cc = cds::container ;
        \endcode
        The inclusion order is important: first, include file for ordered-list implementation (for this example, <tt>cds/container/michael_list_hp.h</tt>),
        then the header for split-list map <tt>cds/container/split_list_map.h</tt>.

        Now, you should declare traits for split-list map. The main parts of traits are a hash functor for the map key and a comparing functor for ordered list.
        We use <tt>std::hash<int></tt> as hash functor and <tt>std::less<int></tt> predicate as comparing functor.

        The second attention: instead of using \p %MichaelList in \p %SplitListMap traits we use a tag <tt>cds::contaner::michael_list_tag</tt> for the Michael's list.
        The split-list requires significant support from underlying ordered list class and it is not good idea to dive you
        into deep implementation details of split-list and ordered list interrelations. The tag paradigm simplifies split-list interface.

        \code
        // SplitListMap traits
        struct foo_set_traits: public cc::split_list::type_traits
        {
            typedef cc::michael_list_tag   ordered_list    ;   // what type of ordered list we want to use
            typedef std::hash<int>         hash            ;   // hash functor for the key stored in split-list map

            // Type traits for our MichaelList class
            struct ordered_list_traits: public cc::michael_list::type_traits
            {
            typedef std::less<int> less   ;   // use our std::less predicate as comparator to order list nodes
            } ;
        };
        \endcode

        Now you are ready to declare our map class based on SplitListMap:
        \code
        typedef cc::SplitListMap< cds::gc::PTB, int, std::string, foo_set_traits > int_string_map   ;
        \endcode

        You may use the modern option-based declaration instead of classic type-traits-based one:
        \code
        typedef cc:SplitListMap<
            cs::gc::PTB             // GC used
            ,int                    // key type
            ,std::string            // value type
            ,cc::split_list::make_traits<      // metafunction to build split-list traits
                cc::split_list::ordered_list<cc::michael_list_tag>     // tag for underlying ordered list implementation
                ,cc::opt::hash< std::hash<int> >        // hash functor
                ,cc::split_list::ordered_list_traits<    // ordered list traits desired
                    cc::michael_list::make_traits<    // metafunction to build lazy list traits
                        cc::opt::less< std::less<int> >         // less-based compare functor
                    >::type
                >
            >::type
        >  int_string_map ;
        \endcode
        In case of option-based declaration using split_list::make_traits metafunction the struct \p foo_set_traits is not required.

        Now, the map of type \p int_string_map is ready to use in your program.

        Note that in this example we show only mandatory type_traits parts, optional ones is the default and they are inherited
        from cds::container::split_list::type_traits.
        The <b>cds</b> library contains many other options for deep tuning of behavior of the split-list and
        ordered-list containers.
    */
    template <
        class GC,
        typename Key,
        typename Value,
#ifdef CDS_DOXYGEN_INVOKED
        class Traits = split_list::type_traits
#else
        class Traits
#endif
    >
    class SplitListMap:
        protected container::SplitListSet<
            GC,
            std::pair<Key const, Value>,
            split_list::details::wrap_map_traits<Key, Value, Traits>
        >
    {
        //@cond
        typedef container::SplitListSet<
            GC,
            std::pair<Key const, Value>,
            split_list::details::wrap_map_traits<Key, Value, Traits>
        >  base_class ;
        //@endcond

    public:
        typedef typename base_class::gc gc              ;   ///< Garbage collector
        typedef Key                     key_type        ;   ///< key type
        typedef Value                   mapped_type     ;   ///< type of value stored in the map
        typedef Traits                  options         ;   ///< \p Traits template argument

        typedef std::pair<key_type const, mapped_type>  value_type  ;   ///< key-value pair type
        typedef typename base_class::ordered_list       ordered_list;   ///< Underlying ordered list class
        typedef typename base_class::key_comparator     key_comparator  ;   ///< key compare functor

        typedef typename base_class::hash           hash            ;   ///< Hash functor for \ref key_type
        typedef typename base_class::item_counter   item_counter    ;   ///< Item counter type

    protected:
        //@cond
        typedef typename base_class::maker::type_traits::key_accessor key_accessor ;

#   ifndef CDS_CXX11_LAMBDA_SUPPORT
        template <typename Func>
        class ensure_functor_wrapper: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            ensure_functor_wrapper() {}
            ensure_functor_wrapper( Func f ): base_class(f) {}

            template <typename Q>
            void operator()( bool bNew, value_type& item, const Q& /*val*/ )
            {
                base_class::get()( bNew, item ) ;
            }
        };

        template <typename Func>
        class find_functor_wrapper: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            find_functor_wrapper() {}
            find_functor_wrapper( Func f ): base_class(f) {}

            template <typename Q>
            void operator()( value_type& pair, Q const& /*val*/ )
            {
                base_class::get()( pair ) ;
            }
        };
#   endif   // ifndef CDS_CXX11_LAMBDA_SUPPORT
        //@endcond

    public:
        /// Forward iterator (see SplitListSet::iterator)
        /**
            Remember, the iterator <tt>operator -> </tt> and <tt>operator *</tt> returns \ref value_type pointer and reference.
            To access item key and value use <tt>it->first</tt> and <tt>it->second</tt> respectively.
        */
        typedef typename base_class::iterator iterator    ;

        /// Const forward iterator (see SplitListSet::const_iterator)
        typedef typename base_class::const_iterator const_iterator    ;

        /// Returns a forward iterator addressing the first element in a map
        /**
            For empty map \code begin() == end() \endcode
        */
        iterator begin()
        {
            return base_class::begin() ;
        }

        /// Returns an iterator that addresses the location succeeding the last element in a map
        /**
            Do not use the value returned by <tt>end</tt> function to access any item.
            The returned value can be used only to control reaching the end of the map.
            For empty map \code begin() == end() \endcode
        */
        iterator end()
        {
            return base_class::end() ;
        }

        /// Returns a forward const iterator addressing the first element in a map
        //@{
        const_iterator begin() const
        {
            return base_class::begin() ;
        }
        const_iterator cbegin()
        {
            return base_class::cbegin() ;
        }
        //@}

        /// Returns an const iterator that addresses the location succeeding the last element in a map
        //@{
        const_iterator end() const
        {
            return base_class::end()  ;
        }
        const_iterator cend()
        {
            return base_class::cend()  ;
        }
        //@}

    public:
        /// Initializes split-ordered map of default capacity
        /**
            The default capacity is defined in bucket table constructor.
            See intrusive::split_list::expandable_bucket_table, intrusive::split_list::static_bucket_table
            which selects by intrusive::split_list::dynamic_bucket_table option.
        */
        SplitListMap()
            : base_class()
        {}

        /// Initializes split-ordered map
        SplitListMap(
            size_t nItemCount           ///< estimate average item count
            , size_t nLoadFactor = 1    ///< load factor - average item count per bucket. Small integer up to 10, default is 1.
            )
            : base_class( nItemCount, nLoadFactor )
        {}

    public:
        /// Inserts new node with key and default value
        /**
            The function creates a node with \p key and default value, and then inserts the node created into the map.

            Preconditions:
            - The \ref key_type should be constructible from value of type \p K.
                In trivial case, \p K is equal to \ref key_type.
            - The \ref mapped_type should be default-constructible.

            Returns \p true if inserting successful, \p false otherwise.
        */
        template <typename K>
        bool insert( K const& key )
        {
            //TODO: pass arguments by reference (make_pair makes copy)
            return base_class::insert( std::make_pair( key, mapped_type() ) ) ;
        }

        /// Inserts new node
        /**
            The function creates a node with copy of \p val value
            and then inserts the node created into the map.

            Preconditions:
            - The \ref key_type should be constructible from \p key of type \p K.
            - The \ref mapped_type should be constructible from \p val of type \p V.

            Returns \p true if \p val is inserted into the map, \p false otherwise.
        */
        template <typename K, typename V>
        bool insert( K const& key, V const& val )
        {
            //TODO: pass arguments by reference (make_pair makes copy)
            return base_class::insert( std::make_pair(key, val) ) ;
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

            It should be keep in mind that concurrent modifications of \p <tt>item.second</tt> may be possible.
            User-defined functor \p func should guarantee that during changing item's value no any other changes
            could be made on this \p item by concurrent threads.

            The user-defined functor can be passed by reference using <tt>boost::ref</tt>
            and it is called only if inserting is successful.

            The key_type should be constructible from value of type \p K.

            The function allows to split creating of new item into two part:
            - create item from \p key;
            - insert new item into the map;
            - if inserting is successful, initialize the value of item by calling \p func functor

            This can be useful if complete initialization of object of \p mapped_type is heavyweight and
            it is preferable that the initialization should be completed only if inserting is successful.
        */
        template <typename K, typename Func>
        bool insert_key( K const& key, Func func )
        {
            //TODO: pass arguments by reference (make_pair makes copy)
            return base_class::insert( std::make_pair( key, mapped_type() ), func ) ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        /// For key \p key inserts data of type \ref mapped_type constructed with <tt>std::forward<Args>(args)...</tt>
        /**
            \p key_type should be constructible from type \p K

            Returns \p true if inserting successful, \p false otherwise.

            This function is available only for compiler that supports
            variadic template and move semantics
        */
        template <typename K, typename... Args>
        bool emplace( K&& key, Args&&... args )
        {
            return base_class::emplace( std::forward<K>(key), std::move(mapped_type(std::forward<Args>(args)...))) ;
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

            The functor may change any fields of the \p item.second that is \ref mapped_type;
            however, \p func must guarantee that during changing no any other modifications
            could be made on this item by concurrent threads.

            You may pass \p func argument by reference using <tt>boost::ref</tt>.

            Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
            \p second is true if new item has been added or \p false if the item with \p key
            already is in the list.
        */
        template <typename K, typename Func>
        std::pair<bool, bool> ensure( K const& key, Func func )
        {
            //TODO: pass arguments by reference (make_pair makes copy)
#   ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::ensure( std::make_pair( key, mapped_type() ),
                [&func](bool bNew, value_type& item, value_type const& /*val*/) {
                    cds::unref(func)( bNew, item );
                } )  ;
#   else
            ensure_functor_wrapper<Func> fw( func ) ;
            return base_class::ensure( std::make_pair( key, mapped_type() ), cds::ref(fw) )  ;
#   endif
        }

        /// Deletes \p key from the map
        /** \anchor cds_nonintrusive_SplitListMap_erase_val

            Return \p true if \p key is found and deleted, \p false otherwise
        */
        template <typename K>
        bool erase( K const& key )
        {
            return base_class::erase( key ) ;
        }

        /// Deletes the item from the map using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SplitListMap_erase_val "erase(K const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less>
        bool erase_with( K const& key, Less pred )
        {
            return base_class::erase_with( key, cds::details::predicate_wrapper<value_type, Less, key_accessor>() ) ;
        }

        /// Deletes \p key from the map
        /** \anchor cds_nonintrusive_SplitListMap_erase_func

            The function searches an item with key \p key, calls \p f functor
            and deletes the item. If \p key is not found, the functor is not called.

            The functor \p Func interface is:
            \code
            struct extractor {
                void operator()(value_type& item) { ... }
            };
            \endcode
            The functor may be passed by reference using <tt>boost:ref</tt>

            Return \p true if key is found and deleted, \p false otherwise
        */
        template <typename K, typename Func>
        bool erase( K const& key, Func f )
        {
            return base_class::erase( key, f )  ;
        }

        /// Deletes the item from the map using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SplitListMap_erase_func "erase(K const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less, typename Func>
        bool erase_with( K const& key, Less pred, Func f )
        {
            return base_class::erase_with( key, cds::details::predicate_wrapper<value_type, Less, key_accessor>(), f )  ;
        }

        /// Finds the key \p key
        /** \anchor cds_nonintrusive_SplitListMap_find_cfunc

            The function searches the item with key equal to \p key and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item ) ;
            };
            \endcode
            where \p item is the item found.

            You may pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change \p item.second. Note that the functor is only guarantee
            that \p item cannot be disposed during functor is executing.
            The functor does not serialize simultaneous access to the map's \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            The function returns \p true if \p key is found, \p false otherwise.
        */
        template <typename K, typename Func>
        bool find( K const& key, Func f )
        {
#   ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find( key, [&f](value_type& pair, K const&){ cds::unref(f)( pair ); } ) ;
#   else
            find_functor_wrapper<Func> fw(f)    ;
            return base_class::find( key, cds::ref(fw) )   ;
#   endif
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SplitListMap_find_cfunc "find(K const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less, typename Func>
        bool find_with( K const& key, Less pred, Func f )
        {
#   ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find_with( key,
                cds::details::predicate_wrapper<value_type, Less, key_accessor>(),
                [&f](value_type& pair, K const&){ cds::unref(f)( pair ); } ) ;
#   else
            find_functor_wrapper<Func> fw(f)    ;
            return base_class::find_with( key, cds::details::predicate_wrapper<value_type, Less, key_accessor>(), cds::ref(fw) )   ;
#   endif
        }

        /// Finds the key \p key
        /** \anchor cds_nonintrusive_SplitListMap_find_val

            The function searches the item with key equal to \p key
            and returns \p true if it is found, and \p false otherwise.
        */
        template <typename K>
        bool find( K const& key )
        {
            return base_class::find( key )    ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SplitListMap_find_val "find(K const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less>
        bool find_with( K const& key, Less pred )
        {
            return base_class::find( key, cds::details::predicate_wrapper<value_type, Less, key_accessor>() )    ;
        }

        /// Clears the map (non-atomic)
        /**
            The function unlink all items from the map.
            The function is not atomic and not lock-free and should be used for debugging only.
        */
        void clear()
        {
            base_class::clear()   ;
        }

        /// Checks if the map is empty
        /**
            Emptiness is checked by item counting: if item count is zero then the map is empty.
            Thus, the correct item counting is an important part of the map implementation.
        */
        bool empty() const
        {
            return base_class::empty() ;
        }

        /// Returns item count in the map
        size_t size() const
        {
            return base_class::size() ;
        }
    };


}} // namespace cds::container

#endif // #ifndef __CDS_CONTAINER_SPLIT_LIST_MAP_H
