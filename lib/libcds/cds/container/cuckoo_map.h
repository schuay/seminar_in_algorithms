//$$CDS-header$$

#ifndef __CDS_CONTAINER_CUCKOO_MAP_H
#define __CDS_CONTAINER_CUCKOO_MAP_H

#include <cds/container/cuckoo_base.h>
#include <cds/details/binary_functor_wrapper.h>

namespace cds { namespace container {

    //@cond
    namespace details {
        template <typename Key, typename T, typename Traits>
        struct make_cuckoo_map
        {
            typedef Key key_type    ;   ///< key type
            typedef T   mapped_type ;   ///< type of value stored in the map
            typedef std::pair<key_type const, mapped_type>   value_type  ;   ///< Pair type

            typedef Traits original_type_traits ;
            typedef typename original_type_traits::probeset_type probeset_type   ;
            static bool const store_hash = original_type_traits::store_hash      ;
            static unsigned int const store_hash_count = store_hash ? ((unsigned int) std::tuple_size< typename original_type_traits::hash::hash_tuple_type >::value) : 0 ;

            struct node_type: public intrusive::cuckoo::node<probeset_type, store_hash_count>
            {
                value_type  m_val   ;

                template <typename K>
                node_type( K const& key )
                    : m_val( std::make_pair( key_type(key), mapped_type() ))
                {}

                template <typename K, typename Q>
                node_type( K const& key, Q const& v )
                    : m_val( std::make_pair( key_type(key), mapped_type(v) ))
                {}

#           ifdef CDS_EMPLACE_SUPPORT
                template <typename K, typename... Args>
                node_type( K&& key, Args&&... args )
                    : m_val( std::forward<K>(key), std::move( mapped_type(std::forward<Args>(args)...)) )
                {}
#           else
                node_type()
                {}
#           endif
            };

            /*
            template <typename Pred, typename ReturnValue>
            struct predicate_wrapper {
                typedef Pred native_predicate ;

                ReturnValue operator()( node_type const& n1, node_type const& n2) const
                {
                    return native_predicate()(n1.m_val.first, n2.m_val.first ) ;
                }
                template <typename Q>
                ReturnValue operator()( node_type const& n, Q const& v) const
                {
                    return native_predicate()(n.m_val.first, v) ;
                }
                template <typename Q>
                ReturnValue operator()( Q const& v, node_type const& n) const
                {
                    return native_predicate()(v, n.m_val.first) ;
                }

                template <typename Q1, typename Q2>
                ReturnValue operator()( Q1 const& v1, Q2 const& v2) const
                {
                    return native_predicate()(v1, v2) ;
                }
            };
            */

            struct key_accessor {
                key_type const& operator()( node_type const& node ) const
                {
                    return node.m_val.first ;
                }
            };

            struct intrusive_traits: public original_type_traits
            {
                typedef intrusive::cuckoo::base_hook<
                    cds::intrusive::cuckoo::probeset_type< probeset_type >
                    ,cds::intrusive::cuckoo::store_hash< store_hash_count >
                >  hook ;

                typedef cds::intrusive::cuckoo::type_traits::disposer   disposer ;

                typedef typename std::conditional<
                    std::is_same< typename original_type_traits::equal_to, opt::none >::value
                    , opt::none
                    , cds::details::predicate_wrapper< node_type, typename original_type_traits::equal_to, key_accessor >
                >::type equal_to ;

                typedef typename std::conditional<
                    std::is_same< typename original_type_traits::compare, opt::none >::value
                    , opt::none
                    , cds::details::compare_wrapper< node_type, typename original_type_traits::compare, key_accessor >
                >::type compare ;

                typedef typename std::conditional<
                    std::is_same< typename original_type_traits::less, opt::none >::value
                    ,opt::none
                    ,cds::details::predicate_wrapper< node_type, typename original_type_traits::less, key_accessor >
                >::type less ;

                typedef opt::details::hash_list_wrapper< typename original_type_traits::hash, node_type, key_accessor >    hash ;
            };

            typedef intrusive::CuckooSet< node_type, intrusive_traits > type    ;
        };
    }   // namespace details
    //@endcond

    /// Cuckoo hash map
    /** @ingroup cds_nonintrusive_map

        Source
            - [2007] M.Herlihy, N.Shavit, M.Tzafrir "Concurrent Cuckoo Hashing. Technical report"
            - [2008] Maurice Herlihy, Nir Shavit "The Art of Multiprocessor Programming"

        <b>About Cuckoo hashing</b>

            [From "The Art of Multiprocessor Programming"]
            Cuckoo hashing is a hashing algorithm in which a newly added item displaces any earlier item
            occupying the same slot. For brevity, a table is a k-entry array of items. For a hash set f size
            N = 2k we use a two-entry array of tables, and two independent hash functions,
            <tt> h0, h1: KeyRange -> 0,...,k-1</tt>
            mapping the set of possible keys to entries in he array. To test whether a value \p x is in the set,
            <tt>find(x)</tt> tests whether either <tt>table[0][h0(x)]</tt> or <tt>table[1][h1(x)]</tt> is
            equal to \p x. Similarly, <tt>erase(x)</tt>checks whether \p x is in either <tt>table[0][h0(x)]</tt>
            or <tt>table[1][h1(x)]</tt>, ad removes it if found.

            The <tt>insert(x)</tt> successively "kicks out" conflicting items until every key has a slot.
            To add \p x, the method swaps \p x with \p y, the current occupant of <tt>table[0][h0(x)]</tt>.
            If the prior value was \p NULL, it is done. Otherwise, it swaps the newly nest-less value \p y
            for the current occupant of <tt>table[1][h1(y)]</tt> in the same way. As before, if the prior value
            was \p NULL, it is done. Otherwise, the method continues swapping entries (alternating tables)
            until it finds an empty slot. We might not find an empty slot, either because the table is full,
            or because the sequence of displacement forms a cycle. We therefore need an upper limit on the
            number of successive displacements we are willing to undertake. When this limit is exceeded,
            we resize the hash table, choose new hash functions and start over.

            For concurrent cuckoo hashing, rather than organizing the set as a two-dimensional table of
            items, we use two-dimensional table of probe sets, where a probe set is a constant-sized set
            of items with the same hash code. Each probe set holds at most \p PROBE_SIZE items, but the algorithm
            tries to ensure that when the set is quiescent (i.e no method call in progress) each probe set
            holds no more than <tt>THRESHOLD < PROBE_SET</tt> items. While method calls are in-flight, a probe
            set may temporarily hold more than \p THRESHOLD but never more than \p PROBE_SET items.

            In current implementation, a probe set can be defined either as a (single-linked) list
            or as a fixed-sized vector, optionally ordered.

            In description above two-table cuckoo hashing (<tt>k = 2</tt>) has been considered.
            We can generalize this approach for <tt>k >= 2</tt> when we have \p k hash functions
            <tt>h[0], ... h[k-1]</tt> and \p k tables <tt>table[0], ... table[k-1]</tt>.

            The search in probe set is linear, the complexity is <tt> O(PROBE_SET) </tt>.
            The probe set may be ordered or not. Ordered probe set can be a little better since
            the average search complexity is <tt>O(PROBE_SET/2)</tt>.
            However, the overhead of sorting can eliminate a gain of ordered search.

            The probe set is ordered if opt::compare or opt::less is specified in \p %CuckooSet
            declaration. Otherwise, the probe set is unordered and \p %CuckooSet must contain
            opt::equal_to option.

        Template arguments:
        - \p Key - key type
        - \p T - the type stored in the map.
        - \p Traits - type traits. See cuckoo::type_traits for explanation.
            It is possible to declare option-based set with cuckoo::make_traits metafunction result as \p Traits template argument.

        Template argument list \p Options... of cuckoo::make_traits metafunction are:
        - opt::hash - hash functor tuple, mandatory option. At least, two hash functors should be provided. All hash functor
            should be orthogonal (different): for each <tt> i,j: i != j => h[i](x) != h[j](x) </tt>.
            The hash functors are passed as <tt> std::tuple< H1, H2, ... Hn > </tt>. The number of hash functors specifies
            the number \p k - the count of hash tables in cuckoo hashing. If the compiler supports variadic templates
            then k is unlimited, otherwise up to 10 different hash functors are supported.
        - opt::mutex_policy - concurrent access policy.
            Available policies: cuckoo::striping, cuckoo::refinable.
            Default is cuckoo::striping.
        - opt::equal_to - key equality functor like \p std::equal_to.
            If this functor is defined then the probe-set will be unordered.
            If opt::compare or opt::less option is specified too, then the probe-set will be ordered
            and opt::equal_to will be ignored.
        - opt::compare - key comparison functor. No default functor is provided.
            If the option is not specified, the opt::less is used.
            If opt::compare or opt::less option is specified, then the probe-set will be ordered.
        - opt::less - specifies binary predicate used for key comparison. Default is \p std::less<T>.
            If opt::compare or opt::less option is specified, then the probe-set will be ordered.
        - opt::item_counter - the type of item counting feature. Default is \ref opt::v::sequential_item_counter.
        - opt::allocator - the allocator type using for allocating bucket tables.
            Default is \p CDS_DEFAULT_ALLOCATOR
        - opt::node_allocator - the allocator type using for allocating map's items. If this option
            is not specified then the type defined in opt::allocator option is used.
        - cuckoo::store_hash - this option reserves additional space in the node to store the hash value
            of the object once it's introduced in the container. When this option is used,
            the map will store the calculated hash value in the node and rehashing operations won't need
            to recalculate the hash of the value. This option will improve the performance of maps
            when rehashing is frequent or hashing the value is a slow operation. Default value is \p false.
        - \ref intrusive::cuckoo::probeset_type "cuckoo::probeset_type" - type of probe set, may be \p cuckoo::list or <tt>cuckoo::vector<Capacity></tt>,
            Default is \p cuckoo::list.
        - opt::stat - internal statistics. Possibly types: cuckoo::stat, cuckoo::empty_stat.
            Default is cuckoo::empty_stat

       <b>Examples</b>

       Declares cuckoo mapping from \p std::string to struct \p foo.
       For cuckoo hashing we should provide at least two hash functions:
       \code
        struct hash1 {
            size_t operator()(std::string const& s) const
            {
                return cds::opt::v::hash<std::string>( s ) ;
            }
        };

        struct hash2: private hash1 {
            size_t operator()(std::string const& s) const
            {
                size_t h = ~( hash1::operator()(s))        ;
                return ~h + 0x9e3779b9 + (h << 6) + (h >> 2)    ;
            }
        };
       \endcode

        Cuckoo-map with list-based unordered probe set and storing hash values
        \code
        #include <cds/container/cuckoo_map.h>

        // Declare type traits
        struct my_traits: public cds::container::cuckoo::type_traits
        {
            typedef std::equal_to< std::string > equal_to ;
            typedef std::tuple< hash1, hash2 >  hash ;

            static bool const store_hash = true ;
        };

        // Declare CuckooMap type
        typedef cds::container::CuckooMap< std::string, foo, my_traits > my_cuckoo_map ;

        // Equal option-based declaration
        typedef cds::container::CuckooMap< std::string, foo,
            cds::container::cuckoo::make_traits<
                cds::opt::hash< std::tuple< hash1, hash2 > >
                ,cds::opt::equal_to< std::equal_to< std::string > >
                ,cds::container::cuckoo::store_hash< true >
            >::type
        > opt_cuckoo_map ;
        \endcode

        If we provide \p less functor instead of \p equal_to
        we get as a result a cuckoo map with ordered probe set that may improve
        performance.
        Example for ordered vector-based probe-set:

        \code
        #include <cds/container/cuckoo_map.h>

        // Declare type traits
        // We use a vector of capacity 4 as probe-set container and store hash values in the node
        struct my_traits: public cds::container::cuckoo::type_traits
        {
            typedef std::less< std::string > less ;
            typedef std::tuple< hash1, hash2 >  hash ;
            typedef cds::container::cuckoo::vector<4> probeset_type ;

            static bool const store_hash = true ;
        };

        // Declare CuckooMap type
        typedef cds::container::CuckooMap< std::string, foo, my_traits > my_cuckoo_map ;

        // Equal option-based declaration
        typedef cds::container::CuckooMap< std::string, foo,
            cds::container::cuckoo::make_traits<
                cds::opt::hash< std::tuple< hash1, hash2 > >
                ,cds::opt::less< std::less< std::string > >
                ,cds::container::cuckoo::probeset_type< cds::container::cuckoo::vector<4> >
                ,cds::container::cuckoo::store_hash< true >
            >::type
        > opt_cuckoo_map ;
        \endcode

    */
    template <typename Key, typename T, typename Traits = cuckoo::type_traits>
    class CuckooMap:
#ifdef CDS_DOXYGEN_INVOKED
        protected intrusive::CuckooSet< std::pair< Key const, T>, Traits>
#else
        protected details::make_cuckoo_map<Key, T, Traits>::type
#endif
    {
        //@cond
        typedef details::make_cuckoo_map<Key, T, Traits>    maker ;
        typedef typename maker::type  base_class ;
        //@endcond
    public:
        typedef Key     key_type    ;   ///< key type
        typedef T       mapped_type ;   ///< value type stored in the container
        typedef std::pair<key_type const, mapped_type>   value_type  ;   ///< Key-value pair type stored in the map

        typedef Traits  options     ;   ///< traits

        typedef typename options::hash                  hash             ; ///< hash functor tuple wrapped for internal use
        typedef typename base_class::hash_tuple_type    hash_tuple_type  ; ///< hash tuple type

        typedef typename base_class::mutex_policy       mutex_policy     ;  ///< Concurrent access policy, see cuckoo::type_traits::mutex_policy
        typedef typename base_class::stat               stat             ;  ///< internal statistics type

        static bool const c_isSorted = base_class::c_isSorted   ; ///< whether the probe set should be ordered
        static size_t const c_nArity = base_class::c_nArity     ; ///< the arity of cuckoo hashing: the number of hash functors provided; minimum 2.

        typedef typename base_class::key_equal_to key_equal_to  ; ///< Key equality functor; used only for unordered probe-set

        typedef typename base_class::key_comparator  key_comparator ; ///< key comparing functor based on opt::compare and opt::less option setter. Used only for ordered probe set

        typedef typename base_class::allocator     allocator   ; ///< allocator type used for internal bucket table allocations

        /// Node allocator type
        typedef typename std::conditional<
            std::is_same< typename options::node_allocator, opt::none >::value,
            allocator,
            typename options::node_allocator
        >::type node_allocator ;

        /// item counter type
        typedef typename options::item_counter  item_counter ;

    protected:
        //@cond
        typedef typename base_class::scoped_cell_lock   scoped_cell_lock    ;
        typedef typename base_class::scoped_full_lock   scoped_full_lock    ;
        typedef typename base_class::scoped_resize_lock scoped_resize_lock  ;
        typedef typename maker::key_accessor            key_accessor        ;

        typedef typename base_class::value_type         node_type   ;
        typedef cds::details::Allocator< node_type, node_allocator >    cxx_node_allocator  ;
        //@endcond

    public:
        static unsigned int const   c_nDefaultProbesetSize = base_class::c_nDefaultProbesetSize ;   ///< default probeset size
        static size_t const         c_nDefaultInitialSize = base_class::c_nDefaultInitialSize   ;   ///< default initial size
        static unsigned int const   c_nRelocateLimit = base_class::c_nRelocateLimit             ;   ///< Count of attempts to relocate before giving up

    protected:
        //@cond
        template <typename K>
        static node_type * alloc_node( K const& key )
        {
            return cxx_node_allocator().New( key ) ;
        }
#   ifdef CDS_EMPLACE_SUPPORT
        template <typename K, typename... Args>
        static node_type * alloc_node( K&& key, Args&&... args )
        {
            return cxx_node_allocator().MoveNew( std::forward<K>( key ), std::forward<Args>(args)... ) ;
        }
#   endif

        static void free_node( node_type * pNode )
        {
            cxx_node_allocator().Delete( pNode ) ;
        }
        //@endcond

    protected:
        //@cond
        struct node_disposer {
            void operator()( node_type * pNode )
            {
                free_node( pNode )  ;
            }
        };

        typedef std::unique_ptr< node_type, node_disposer >     scoped_node_ptr ;

#ifndef CDS_CXX11_LAMBDA_SUPPORT
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
            insert_value_functor( Q const & v)
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

            void operator()( node_type& item )
            {
                base_class::get()( item.m_val ) ;
            }
        };

        template <typename Func>
        class ensure_wrapper: protected cds::details::functor_wrapper<Func>
        {
            typedef cds::details::functor_wrapper<Func> base_class ;
        public:
            ensure_wrapper( Func f) : base_class(f) {}

            void operator()( bool bNew, node_type& item, node_type const& )
            {
                base_class::get()( bNew, item.m_val ) ;
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
            void operator()( node_type& item, Q& val )
            {
                base_class::get()( item.m_val, val ) ;
            }
        };
#endif  // #ifndef CDS_CXX11_LAMBDA_SUPPORT

        //@endcond

    public:
        /// Default constructor
        /**
            Initial size = \ref c_nDefaultInitialSize

            Probe set size:
            - \ref c_nDefaultProbesetSize if \p probeset_type is \p cuckoo::list
            - \p Capacity if \p probeset_type is <tt> cuckoo::vector<Capacity> </tt>

            Probe set threshold = probe set size - 1
        */
        CuckooMap()
        {}

        /// Constructs an object with given probe set size and threshold
        /**
            If probe set type is <tt> cuckoo::vector<Capacity> </tt> vector
            then \p nProbesetSize should be equal to vector's \p Capacity.
        */
        CuckooMap(
            size_t nInitialSize                     ///< Initial map size; if 0 - use default initial size \ref c_nDefaultInitialSize
            , unsigned int nProbesetSize            ///< probe set size
            , unsigned int nProbesetThreshold = 0   ///< probe set threshold, <tt>nProbesetThreshold < nProbesetSize</tt>. If 0, nProbesetThreshold = nProbesetSize - 1
        )
        : base_class( nInitialSize, nProbesetSize, nProbesetThreshold )
        {}

        /// Constructs an object with given hash functor tuple
        /**
            The probe set size and threshold are set as default, see CuckooSet()
        */
        CuckooMap(
            hash_tuple_type const& h    ///< hash functor tuple of type <tt>std::tuple<H1, H2, ... Hn></tt> where <tt> n == \ref c_nArity </tt>
        )
        : base_class( h )
        {}

        /// Constructs a map with given probe set properties and hash functor tuple
        /**
            If probe set type is <tt> cuckoo::vector<Capacity> </tt> vector
            then \p nProbesetSize should be equal to vector's \p Capacity.
        */
        CuckooMap(
            size_t nInitialSize                 ///< Initial map size; if 0 - use default initial size \ref c_nDefaultInitialSize
            , unsigned int nProbesetSize        ///< probe set size
            , unsigned int nProbesetThreshold   ///< probe set threshold, <tt>nProbesetThreshold < nProbesetSize</tt>. If 0, nProbesetThreshold = nProbesetSize - 1
            , hash_tuple_type const& h    ///< hash functor tuple of type <tt>std::tuple<H1, H2, ... Hn></tt> where <tt> n == \ref c_nArity </tt>
        )
        : base_class( nInitialSize, nProbesetSize, nProbesetThreshold, h )
        {}

#   ifdef CDS_MOVE_SEMANTICS_SUPPORT
        /// Constructs a map with given hash functor tuple (move semantics)
        /**
            The probe set size and threshold are set as default, see CuckooSet()
        */
        CuckooMap(
            hash_tuple_type&& h     ///< hash functor tuple of type <tt>std::tuple<H1, H2, ... Hn></tt> where <tt> n == \ref c_nArity </tt>
        )
        : base_class( std::forward<hash_tuple_type>(h) )
        {}

        /// Constructs a map with given probe set properties and hash functor tuple (move semantics)
        /**
            If probe set type is <tt> cuckoo::vector<Capacity> </tt> vector
            then \p nProbesetSize should be equal to vector's \p Capacity.
        */
        CuckooMap(
            size_t nInitialSize                 ///< Initial map size; if 0 - use default initial size \ref c_nDefaultInitialSize
            , unsigned int nProbesetSize        ///< probe set size
            , unsigned int nProbesetThreshold   ///< probe set threshold, <tt>nProbesetThreshold < nProbesetSize</tt>. If 0, nProbesetThreshold = nProbesetSize - 1
            , hash_tuple_type&& h    ///< hash functor tuple of type <tt>std::tuple<H1, H2, ... Hn></tt> where <tt> n == \ref c_nArity </tt>
        )
        : base_class( nInitialSize, nProbesetSize, nProbesetThreshold, std::forward<hash_tuple_type>(h) )
        {}
#   endif   // ifdef CDS_MOVE_SEMANTICS_SUPPORT

        /// Destructor clears the map
        ~CuckooMap()
        {
            clear() ;
        }

    public:
        /// Inserts new node with key and default value
        /**
            The function creates a node with \p key and default value, and then inserts the node created into the map.

            Preconditions:
            - The \ref key_type should be constructible from a value of type \p K.
                In trivial case, \p K is equal to \ref key_type.
            - The \ref mapped_type should be default-constructible.

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

            Returns \p true if \p val is inserted into the set, \p false otherwise.
        */
        template <typename K, typename V>
        bool insert( K const& key, V const& val )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return insert_key( key, [&val](value_type& item) { item.second = val ; } )   ;
#       else
            insert_value_functor<V> f(val) ;
            return insert_key( key, cds::ref(f) )   ;
#       endif
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
        */
        template <typename K, typename Func>
        bool insert_key( const K& key, Func func )
        {
            scoped_node_ptr pNode( alloc_node( key )) ;
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            if ( base_class::insert( *pNode, [&func]( node_type& item ) { cds::unref(func)( item.m_val ); } ))
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

            This function is available only for compiler that supports
            variadic template and move semantics
        */
        template <typename K, typename... Args>
        bool emplace( K&& key, Args&&... args )
        {
            scoped_node_ptr pNode( alloc_node( std::forward<K>(key), std::forward<Args>(args)... )) ;
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

            Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
            \p second is true if new item has been added or \p false if the item with \p key
            already is in the list.
        */
        template <typename K, typename Func>
        std::pair<bool, bool> ensure( K const& key, Func func )
        {
            scoped_node_ptr pNode( alloc_node( key )) ;
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            std::pair<bool, bool> res = base_class::ensure( *pNode,
                [&func](bool bNew, node_type& item, node_type const& ){ cds::unref(func)( bNew, item.m_val ); }
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
        /** \anchor cds_nonintrusive_CuckooMap_erase_val

            Return \p true if \p key is found and deleted, \p false otherwise
        */
        template <typename K>
        bool erase( K const& key )
        {
            node_type * pNode = base_class::erase(key) ;
            if ( pNode ) {
                free_node( pNode )  ;
                return true     ;
            }
            return false ;
        }

        /// Deletes the item from the list using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_CuckooMap_erase_val "erase(Q const&)"
            but \p pred is used for key comparing.
            If cuckoo map is ordered, then \p Predicate should have the interface and semantics like \p std::less.
            If cuckoo map is unordered, then \p Predicate should have the interface and semantics like \p std::equal_to.
            \p Predicate must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Predicate>
        bool erase_with( K const& key, Predicate pred )
        {
            node_type * pNode = base_class::erase_with(key, cds::details::predicate_wrapper<node_type, Predicate, key_accessor>()) ;
            if ( pNode ) {
                free_node( pNode )  ;
                return true     ;
            }
            return false ;
        }

        /// Delete \p key from the map
        /** \anchor cds_nonintrusive_CuckooMap_erase_func

            The function searches an item with key \p key, calls \p f functor
            and deletes the item. If \p key is not found, the functor is not called.

            The functor \p Func interface:
            \code
            struct extractor {
                void operator()(value_type& item) { ... }
            };
            \endcode
            The functor may be passed by reference using <tt>boost:ref</tt>

            Return \p true if key is found and deleted, \p false otherwise

            See also: \ref erase
        */
        template <typename K, typename Func>
        bool erase( K const& key, Func f )
        {
            node_type * pNode = base_class::erase( key ) ;
            if ( pNode ) {
                cds::unref(f)( pNode->m_val ) ;
                free_node( pNode )  ;
                return true ;
            }
            return false ;
        }

        /// Deletes the item from the list using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_CuckooMap_erase_func "erase(Q const&, Func)"
            but \p pred is used for key comparing.
            If cuckoo map is ordered, then \p Predicate should have the interface and semantics like \p std::less.
            If cuckoo map is unordered, then \p Predicate should have the interface and semantics like \p std::equal_to.
            \p Predicate must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Predicate, typename Func>
        bool erase_with( K const& key, Predicate pred, Func f )
        {
            node_type * pNode = base_class::erase_with( key, cds::details::predicate_wrapper<node_type, Predicate, key_accessor>() ) ;
            if ( pNode ) {
                cds::unref(f)( pNode->m_val ) ;
                free_node( pNode )  ;
                return true ;
            }
            return false ;
        }

        /// Find the key \p key
        /** \anchor cds_nonintrusive_CuckooMap_find_func

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

            The function returns \p true if \p key is found, \p false otherwise.
        */
        template <typename K, typename Func>
        bool find( K const& key, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find( key, [&f](node_type& item, K const& ) { cds::unref(f)( item.m_val );}) ;
#       else
            find_wrapper<Func> wrapper(f)       ;
            return base_class::find( key, cds::ref(wrapper) )   ;
#       endif
        }

        /// Find the key \p val using \p pred predicate for comparing
        /**
            The function is an analog of \ref cds_nonintrusive_CuckooMap_find_func "find(K const&, Func)"
            but \p pred is used for key comparison.
            If you use ordered cuckoo map, then \p Predicate should have the interface and semantics like \p std::less.
            If you use unordered cuckoo map, then \p Predicate should have the interface and semantics like \p std::equal_to.
            \p pred must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Predicate, typename Func>
        bool find_with( K const& key, Predicate pred, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return base_class::find_with( key, cds::details::predicate_wrapper<node_type, Predicate, key_accessor>(),
                [&f](node_type& item, K const& ) { cds::unref(f)( item.m_val );}) ;
#       else
            find_wrapper<Func> wrapper(f)       ;
            return base_class::find_with( key, cds::details::predicate_wrapper<node_type, Predicate, key_accessor>(), cds::ref(wrapper) )   ;
#       endif
        }

        /// Find the key \p key
        /** \anchor cds_nonintrusive_CuckooSet_find_val

            The function searches the item with key equal to \p key
            and returns \p true if it is found, and \p false otherwise.
        */
        template <typename K>
        bool find( K const& key )
        {
            return base_class::find( key ) ;
        }

        /// Find the key \p val using \p pred predicate for comparing
        /**
            The function is an analog of \ref cds_nonintrusive_CuckooSet_find_val "find(K const&)"
            but \p pred is used for key comparison.
            If you use ordered cuckoo map, then \p Predicate should have the interface and semantics like \p std::less.
            If you use unordered cuckoo map, then \p Predicate should have the interface and semantics like \p std::equal_to.
            \p pred must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Predicate>
        bool find_with( K const& key, Predicate pred )
        {
            return base_class::find_with( key, cds::details::predicate_wrapper<node_type, Predicate, key_accessor>() ) ;
        }

        /// Clears the map
        void clear()
        {
            base_class::clear_and_dispose( node_disposer() ) ;
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

        /// Returns the size of hash table
        /**
            The hash table size is non-constant and can be increased via resizing.
        */
        size_t bucket_count() const
        {
            return base_class::bucket_count() ;
        }

        /// Returns lock array size
        /**
            The lock array size is constant.
        */
        size_t lock_count() const
        {
            return base_class::lock_count() ;
        }

        /// Returns const reference to internal statistics
        stat const& statistics() const
        {
            return base_class::statistics() ;
        }

        /// Returns const reference to mutex policy internal statistics
        typename mutex_policy::statistics_type const& mutex_policy_statistics() const
        {
            return base_class::mutex_policy_statistics() ;
        }

    };
}}  // namespace cds::container

#endif //#ifndef __CDS_CONTAINER_CUCKOO_MAP_H
