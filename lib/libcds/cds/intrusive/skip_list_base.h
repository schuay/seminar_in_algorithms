//$$CDS-header$$

#ifndef __CDS_INTRUSIVE_SKIP_LIST_BASE_H
#define __CDS_INTRUSIVE_SKIP_LIST_BASE_H

#include <cds/intrusive/base.h>
#include <cds/details/marked_ptr.h>
#include <cds/bitop.h>
#include <cds/os/timer.h>
#include <cds/urcu/options.h>


namespace cds { namespace intrusive {
    /// SkipListSet related definitions
    /** @ingroup cds_intrusive_helper
    */
    namespace skip_list {

        /// The maximum possible height of any skip-list
        static unsigned int const c_nHeightLimit = 32 ;

        /// Skip list node
        /**
            Template parameters:
            - GC - garbage collector
            - Tag - a tag used to distinguish between different implementation. An incomplete type may be used as a tag.
        */
        template <class GC, typename Tag = opt::none>
        class node {
        public:
            typedef GC      gc          ;   ///< Garbage collector
            typedef Tag     tag         ;   ///< tag

            typedef cds::details::marked_ptr<node, 1>                       marked_ptr          ;   ///< marked pointer
            typedef typename gc::template atomic_marked_ptr< marked_ptr>    atomic_marked_ptr   ;   ///< atomic marked pointer specific for GC
            //@cond
            typedef atomic_marked_ptr tower_item_type ;
            //@endcond

        protected:
            atomic_marked_ptr       m_pNext     ;   ///< Next item in bottom-list (list at level 0)
            unsigned int            m_nHeight   ;   ///< Node height (size of m_arrNext array). For node at level 0 the height is 1.
            atomic_marked_ptr *     m_arrNext   ;   ///< Array of next items for levels 1 .. m_nHeight - 1. For node at level 0 \p m_arrNext is \p NULL

        public:
            /// Constructs a node of height 1 (a bottom-list node)
            node()
                : m_pNext( null_ptr<node *>())
                , m_nHeight(1)
                , m_arrNext( null_ptr<atomic_marked_ptr *>())
            {}

            /// Constructs a node of height \p nHeight
            void make_tower( unsigned int nHeight, atomic_marked_ptr * nextTower )
            {
                assert( nHeight > 0 )   ;
                assert( ( nHeight == 1 && nextTower == null_ptr<atomic_marked_ptr *>() )  // bottom-list node
                     || ( nHeight > 1  && nextTower != null_ptr<atomic_marked_ptr *>() )   // node at level of more than 0
                ) ;

                m_arrNext = nextTower   ;
                m_nHeight = nHeight     ;
            }

            //@cond
            atomic_marked_ptr * release_tower()
            {
                atomic_marked_ptr * pTower = m_arrNext  ;
                m_arrNext = null_ptr<atomic_marked_ptr *>() ;
                m_nHeight = 1   ;
                return pTower   ;
            }

            atomic_marked_ptr * get_tower() const
            {
                return m_arrNext ;
            }
            //@endcond

            /// Access to element of next pointer array
            atomic_marked_ptr& next( unsigned int nLevel )
            {
                assert( nLevel < height() ) ;
                assert( nLevel == 0 || (nLevel > 0 && m_arrNext != null_ptr<atomic_marked_ptr *>() )) ;

                return nLevel ? m_arrNext[ nLevel - 1] : m_pNext ;
            }

            /// Access to element of next pointer array (const version)
            atomic_marked_ptr const& next( unsigned int nLevel ) const
            {
                assert( nLevel < height() ) ;
                assert( nLevel == 0 || nLevel > 0 && m_arrNext != null_ptr<atomic_marked_ptr *>() ) ;

                return nLevel ? m_arrNext[ nLevel - 1] : m_pNext ;
            }

            /// Access to element of next pointer array (same as \ref next function)
            atomic_marked_ptr& operator[]( unsigned int nLevel )
            {
                return next( nLevel ) ;
            }

            /// Access to element of next pointer array (same as \ref next function)
            atomic_marked_ptr const& operator[]( unsigned int nLevel ) const
            {
                return next( nLevel ) ;
            }

            /// Height of the node
            unsigned int height() const
            {
                return m_nHeight    ;
            }

            /// Clears internal links
            void clear()
            {
                assert( m_arrNext == null_ptr<atomic_marked_ptr *>()) ;
                m_pNext.store( marked_ptr(), CDS_ATOMIC::memory_order_release ) ;
            }

            //@cond
            bool is_cleared() const
            {
                return m_pNext == atomic_marked_ptr()
                    && m_arrNext == null_ptr<atomic_marked_ptr *>()
                    && m_nHeight <= 1
                    ;
            }
            //@endcond
        };

        //@cond
        struct undefined_gc ;
        struct default_hook {
            typedef undefined_gc    gc  ;
            typedef opt::none       tag ;
        };
        //@endcond

        //@cond
        template < typename HookType, CDS_DECL_OPTIONS2>
        struct hook
        {
            typedef typename opt::make_options< default_hook, CDS_OPTIONS2>::type  options ;
            typedef typename options::gc    gc  ;
            typedef typename options::tag   tag ;
            typedef node<gc, tag>           node_type   ;
            typedef HookType                hook_type   ;
        };
        //@endcond

        /// Base hook
        /**
            \p Options are:
            - opt::gc - garbage collector used.
            - opt::tag - a tag
        */
        template < CDS_DECL_OPTIONS2 >
        struct base_hook: public hook< opt::base_hook_tag, CDS_OPTIONS2 >
        {};

        /// Member hook
        /**
            \p MemberOffset defines offset in bytes of \ref node member into your structure.
            Use \p offsetof macro to define \p MemberOffset

            \p Options are:
            - opt::gc - garbage collector used.
            - opt::tag - a tag
        */
        template < size_t MemberOffset, CDS_DECL_OPTIONS2 >
        struct member_hook: public hook< opt::member_hook_tag, CDS_OPTIONS2 >
        {
            //@cond
            static const size_t c_nMemberOffset = MemberOffset ;
            //@endcond
        };

        /// Traits hook
        /**
            \p NodeTraits defines type traits for node.
            See \ref node_traits for \p NodeTraits interface description

            \p Options are:
            - opt::gc - garbage collector used.
            - opt::tag - a tag
        */
        template <typename NodeTraits, CDS_DECL_OPTIONS2 >
        struct traits_hook: public hook< opt::traits_hook_tag, CDS_OPTIONS2 >
        {
            //@cond
            typedef NodeTraits node_traits ;
            //@endcond
        };

        /// Option specifying random level generator
        /**
            The random level generator is an important part of skip-list algorithm.
            The node height in the skip-list have a probabilistic distribution
            where half of the nodes that have level \p i pointers also have level <tt>i+1</tt> pointers
            (i = 0..30).
            The random level generator should provide such distribution.

            The \p Type functor interface is:
            \code
            struct random_generator {
                static unsigned int const c_nUpperBound = 32 ;
                random_generator() ;
                unsigned int operator()() ;
            };
            \endcode

            where
            - \p c_nUpperBound - constant that specifies the upper bound of random number generated.
                The generator produces a number from range <tt>[0 .. c_nUpperBound)</tt> (upper bound excluded).
                \p c_nUpperBound must be no more than 32.
            - <tt>random_generator()</tt> - the constructor of generator object initialises the generator instance (its internal state).
            - <tt>unsigned int operator()()</tt> - the main generating function. Returns random level from range 0..31.

            Stateful generators are supported.

            Available \p Type implementations:
            - \ref xorshift
            - \ref turbo_pascal
        */
        template <typename Type>
        struct random_level_generator {
            //@cond
            template <typename Base>
            struct pack: public Base
            {
                typedef Type random_level_generator ;
            };
            //@endcond
        };

        /// Xor-shift random level generator
        /**
            The simplest of the generators described in George
            Marsaglia's "Xorshift RNGs" paper.  This is not a high-quality
            generator but is acceptable for skip-list.

            The random generator should return numbers from range [0..31].

            From Doug Lea's ConcurrentSkipListMap.java.
        */
        class xorshift {
            //@cond
            CDS_ATOMIC::atomic<unsigned int>    m_nSeed ;
            //@endcond
        public:
            /// The upper bound of generator's return value. The generator produces random number in range <tt>[0..c_nUpperBound)</tt>
            static unsigned int const c_nUpperBound = c_nHeightLimit ;

            /// Initializes the generator instance
            xorshift()
            {
                m_nSeed.store( (unsigned int) cds::OS::Timer::random_seed(), CDS_ATOMIC::memory_order_relaxed ) ;
            }

            /// Main generator function
            unsigned int operator()()
            {
                /* ConcurrentSkipListMap.java
                private int randomLevel() {
                    int x = randomSeed;
                    x ^= x << 13;
                    x ^= x >>> 17;
                    randomSeed = x ^= x << 5;
                    if ((x & 0x80000001) != 0) // test highest and lowest bits
                        return 0;
                    int level = 1;
                    while (((x >>>= 1) & 1) != 0) ++level;
                    return level;
                }
                */
                unsigned int x = m_nSeed.load( CDS_ATOMIC::memory_order_relaxed ) ;
                x ^= x << 13;
                x ^= x >> 17;
                x ^= x << 5;
                m_nSeed.store( x, CDS_ATOMIC::memory_order_relaxed ) ;
                unsigned int nLevel = ((x & 0x00000001) != 0) ? 0 : cds::bitop::LSB( (~(x >> 1)) & 0x7FFFFFFF ) ;
                assert( nLevel < c_nUpperBound )  ;
                return nLevel ;
            }
        };

        /// Turbo-pascal random level generator
        /**
            This uses a cheap pseudo-random function that was used in Turbo Pascal.

            The random generator should return numbers from range [0..31].

            From Doug Lea's ConcurrentSkipListMap.java.
        */
        class turbo_pascal
        {
            //@cond
            CDS_ATOMIC::atomic<unsigned int>    m_nSeed ;
            //@endcond
        public:
            /// The upper bound of generator's return value. The generator produces random number in range <tt>[0..c_nUpperBound)</tt>
            static unsigned int const c_nUpperBound = c_nHeightLimit ;

            /// Initializes the generator instance
            turbo_pascal()
            {
                m_nSeed.store( (unsigned int) cds::OS::Timer::random_seed(), CDS_ATOMIC::memory_order_relaxed ) ;
            }

            /// Main generator function
            unsigned int operator()()
            {
                /*
                private int randomLevel() {
                    int level = 0;
                    int r = randomSeed;
                    randomSeed = r * 134775813 + 1;
                    if (r < 0) {
                        while ((r <<= 1) > 0)
                            ++level;
                    }
                return level;
                }
                */
                /*
                    The low bits are apparently not very random (the original used only
                    upper 16 bits) so we traverse from highest bit down (i.e., test
                    sign), thus hardly ever use lower bits.
                */
                unsigned int x = m_nSeed.load( CDS_ATOMIC::memory_order_relaxed ) * 134775813 + 1;
                m_nSeed.store( x, CDS_ATOMIC::memory_order_relaxed ) ;
                unsigned int nLevel = ( x & 0x80000000 ) ? (31 - cds::bitop::MSBnz( (x & 0x7FFFFFFF) | 1 )) : 0 ;
                assert( nLevel < c_nUpperBound )   ;
                return nLevel ;
            }
        };

        /// SkipListSet internal statistics
        struct stat {
            typedef cds::atomicity::event_counter   event_counter ; ///< Event counter type

            event_counter   m_nNodeHeightAdd[c_nHeightLimit] ; ///< Count of added node of each height
            event_counter   m_nNodeHeightDel[c_nHeightLimit] ; ///< Count of deleted node of each height
            event_counter   m_nInsertSuccess        ; ///< Count of success insertion
            event_counter   m_nInsertFailed         ; ///< Count of failed insertion
            event_counter   m_nInsertRetries        ; ///< Count of unsuccessful retries of insertion
            event_counter   m_nEnsureExist          ; ///< Count of \p ensure call for existed node
            event_counter   m_nEnsureNew            ; ///< Count of \p ensure call for new node
            event_counter   m_nUnlinkSuccess        ; ///< Count of successful call of \p unlink
            event_counter   m_nUnlinkFailed         ; ///< Count of failed call of \p unlink
            event_counter   m_nEraseSuccess         ; ///< Count of successful call of \p erase
            event_counter   m_nEraseFailed          ; ///< Count of failed call of \p erase
            event_counter   m_nFindFastSuccess      ; ///< Count of successful call of \p find and all derivatives (via fast-path)
            event_counter   m_nFindFastFailed       ; ///< Count of failed call of \p find and all derivatives (via fast-path)
            event_counter   m_nFindSlowSuccess      ; ///< Count of successful call of \p find and all derivatives (via slow-path)
            event_counter   m_nFindSlowFailed       ; ///< Count of failed call of \p find and all derivatives (via slow-path)
            event_counter   m_nRenewInsertPosition  ; ///< Count of renewing position events while inserting
            event_counter   m_nLogicDeleteWhileInsert   ;   ///< Count of events "The node has been logically deleted while inserting"
            event_counter   m_nNotFoundWhileInsert  ; ///< Count of events "Inserting node is not found"
            event_counter   m_nFastErase            ; ///< Fast erase event counter
            event_counter   m_nSlowErase            ; ///< Slow erase event counter
            event_counter   m_nExtractSuccess       ; ///< Count of successful call of \p extract
            event_counter   m_nExtractFailed        ; ///< Count of failed call of \p extract
            event_counter   m_nExtractRetries       ; ///< Count of retries of \p extract call
            event_counter   m_nExtractMinSuccess    ; ///< Count of successful call of \p extract_min
            event_counter   m_nExtractMinFailed     ; ///< Count of failed call of \p extract_min
            event_counter   m_nExtractMinRetries    ; ///< Count of retries of \p extract_min call
            event_counter   m_nExtractMaxSuccess    ; ///< Count of successful call of \p extract_max
            event_counter   m_nExtractMaxFailed     ; ///< Count of failed call of \p extract_max
            event_counter   m_nExtractMaxRetries    ; ///< Count of retries of \p extract_max call

            //@cond
            void onAddNode( unsigned int nHeight )
            {
                assert( nHeight > 0 && nHeight <= sizeof(m_nNodeHeightAdd) / sizeof(m_nNodeHeightAdd[0])) ;
                ++m_nNodeHeightAdd[nHeight - 1] ;
            }
            void onRemoveNode( unsigned int nHeight )
            {
                assert( nHeight > 0 && nHeight <= sizeof(m_nNodeHeightDel) / sizeof(m_nNodeHeightDel[0])) ;
                ++m_nNodeHeightDel[nHeight - 1] ;
            }

            void onInsertSuccess()          { ++m_nInsertSuccess    ; }
            void onInsertFailed()           { ++m_nInsertFailed     ; }
            void onInsertRetry()            { ++m_nInsertRetries    ; }
            void onEnsureExist()            { ++m_nEnsureExist      ; }
            void onEnsureNew()              { ++m_nEnsureNew        ; }
            void onUnlinkSuccess()          { ++m_nUnlinkSuccess    ; }
            void onUnlinkFailed()           { ++m_nUnlinkFailed     ; }
            void onEraseSuccess()           { ++m_nEraseSuccess     ; }
            void onEraseFailed()            { ++m_nEraseFailed      ; }
            void onFindFastSuccess()        { ++m_nFindFastSuccess  ; }
            void onFindFastFailed()         { ++m_nFindFastFailed   ; }
            void onFindSlowSuccess()        { ++m_nFindSlowSuccess  ; }
            void onFindSlowFailed()         { ++m_nFindSlowFailed   ; }
            void onRenewInsertPosition()    { ++m_nRenewInsertPosition; }
            void onLogicDeleteWhileInsert() { ++m_nLogicDeleteWhileInsert; }
            void onNotFoundWhileInsert()    { ++m_nNotFoundWhileInsert; }
            void onFastErase()              { ++m_nFastErase; }
            void onSlowErase()              { ++m_nSlowErase; }
            void onExtractSuccess()         { ++m_nExtractSuccess;    }
            void onExtractFailed()          { ++m_nExtractFailed;     }
            void onExtractRetry()           { ++m_nExtractRetries;    }
            void onExtractMinSuccess()      { ++m_nExtractMinSuccess; }
            void onExtractMinFailed()       { ++m_nExtractMinFailed;  }
            void onExtractMinRetry()        { ++m_nExtractMinRetries; }
            void onExtractMaxSuccess()      { ++m_nExtractMaxSuccess; }
            void onExtractMaxFailed()       { ++m_nExtractMaxFailed;  }
            void onExtractMaxRetry()        { ++m_nExtractMaxRetries; }

            //@endcond
        };

        /// SkipListSet empty internal statistics
        struct empty_stat {
            //@cond
            void onAddNode( unsigned int nHeight ) const {}
            void onRemoveNode( unsigned int nHeight ) const {}
            void onInsertSuccess()      const {}
            void onInsertFailed()       const {}
            void onInsertRetry()        const {}
            void onEnsureExist()        const {}
            void onEnsureNew()          const {}
            void onUnlinkSuccess()      const {}
            void onUnlinkFailed()       const {}
            void onEraseSuccess()       const {}
            void onEraseFailed()        const {}
            void onFindFastSuccess()    const {}
            void onFindFastFailed()     const {}
            void onFindSlowSuccess()    const {}
            void onFindSlowFailed()     const {}
            void onRenewInsertPosition()    const {}
            void onLogicDeleteWhileInsert() const {}
            void onNotFoundWhileInsert()    const {}
            void onFastErase()              const {}
            void onSlowErase()              const {}
            void onExtractSuccess()         const {}
            void onExtractFailed()          const {}
            void onExtractRetry()           const {}
            void onExtractMinSuccess()      const {}
            void onExtractMinFailed()       const {}
            void onExtractMinRetry()        const {}
            void onExtractMaxSuccess()      const {}
            void onExtractMaxFailed()       const {}
            void onExtractMaxRetry()        const {}

            //@endcond
        };

        //@cond
        // For internal use only!!!
        template <typename Type>
        struct internal_node_builder {
            template <typename Base>
            struct pack: public Base
            {
                typedef Type internal_node_builder ;
            };
        };
        //@endcond

        /// Type traits for SkipListSet class
        struct type_traits
        {
            /// Hook used
            /**
                Possible values are: skip_list::base_hook, skip_list::member_hook, skip_list::traits_hook.
            */
            typedef base_hook<>       hook        ;

            /// Key comparison functor
            /**
                No default functor is provided. If the option is not specified, the \p less is used.
            */
            typedef opt::none                       compare     ;

            /// specifies binary predicate used for key compare.
            /**
                Default is \p std::less<T>.
            */
            typedef opt::none                       less        ;

            /// Disposer
            /**
                The functor used for dispose removed items. Default is opt::v::empty_disposer.
            */
            typedef opt::v::empty_disposer          disposer    ;

            /// Item counter
            /**
                The type for item counting feature.
                Default is no item counter (\ref atomicity::empty_item_counter)
            */
            typedef atomicity::empty_item_counter     item_counter;

            /// C++ memory ordering model
            /**
                List of available memory ordering see opt::memory_model
            */
            typedef opt::v::relaxed_ordering        memory_model    ;

            /// Random level generator
            /**
                The random level generator is an important part of skip-list algorithm.
                The node height in the skip-list have a probabilistic distribution
                where half of the nodes that have level \p i pointers also have level <tt>i+1</tt> pointers
                (i = 0..30). So, the height of a node is in range [0..31].

                See skip_list::random_level_generator option setter.
            */
            typedef turbo_pascal                    random_level_generator  ;

            /// Allocator
            /**
                Although the skip-list is an intrusive container,
                an allocator should be provided to maintain variable randomly-calculated height of the node
                since the node can contain up to 32 next pointers.
                The allocator specified is used to allocate an array of next pointers
                for nodes which height is more than 1.
            */
            typedef CDS_DEFAULT_ALLOCATOR           allocator ;

            /// back-off strategy used
            /**
                If the option is not specified, the cds::backoff::Default is used.
            */
            typedef cds::backoff::Default           back_off    ;

            /// Internal statistics
            typedef empty_stat                      stat ;

            /// RCU deadlock checking policy (only for \ref cds_intrusive_SkipListSet_rcu "RCU-based SkipListSet")
            /**
                List of available options see opt::rcu_check_deadlock
            */
            typedef opt::v::rcu_throw_deadlock      rcu_check_deadlock ;

            //@cond
            // For internal use only!!!
            typedef opt::none                       internal_node_builder    ;
            //@endcond
        };

        /// Metafunction converting option list to SkipListSet traits
        /**
            This is a wrapper for <tt> cds::opt::make_options< type_traits, Options...> </tt>
            \p Options list see \ref SkipListSet.
        */
        template <CDS_DECL_OPTIONS13>
        struct make_traits {
#   ifdef CDS_DOXYGEN_INVOKED
            typedef implementation_defined type ;   ///< Metafunction result
#   else
            typedef typename cds::opt::make_options<
                typename cds::opt::find_type_traits< type_traits, CDS_OPTIONS13 >::type
                ,CDS_OPTIONS13
            >::type   type ;
#   endif
        };

        //@cond
        namespace details {
            template <typename Node>
            class head_node: public Node
            {
                typedef Node node_type ;

                typename node_type::atomic_marked_ptr   m_Tower[skip_list::c_nHeightLimit] ;

            public:
                head_node( unsigned int nHeight )
                {
                    for ( size_t i = 0; i < sizeof(m_Tower) / sizeof(m_Tower[0]); ++i )
                        m_Tower[i].store( typename node_type::marked_ptr(), CDS_ATOMIC::memory_order_relaxed )  ;

                    node_type::make_tower( nHeight, m_Tower ) ;
                }

                node_type * head() const
                {
                    return const_cast<node_type *>( static_cast<node_type const *>(this)) ;
                }
            };

            template <typename NodeType, typename AtomicNodePtr, typename Alloc>
            struct intrusive_node_builder
            {
                typedef NodeType        node_type       ;
                typedef AtomicNodePtr   atomic_node_ptr ;
                typedef Alloc           allocator_type  ;

                typedef cds::details::Allocator< atomic_node_ptr, allocator_type >  tower_allocator ;

                template <typename RandomGen>
                static node_type * make_tower( node_type * pNode, RandomGen& gen )
                {
                    return make_tower( pNode, gen() + 1 )  ;
                }

                static node_type * make_tower( node_type * pNode, unsigned int nHeight )
                {
                    if ( nHeight > 1 )
                        pNode->make_tower( nHeight, tower_allocator().NewArray( nHeight - 1, null_ptr<node_type *>() )) ;
                    return pNode ;
                }

                static void dispose_tower( node_type * pNode )
                {
                    unsigned int nHeight = pNode->height() ;
                    if ( nHeight > 1 )
                        tower_allocator().Delete( pNode->release_tower(), nHeight ) ;
                }

                struct node_disposer {
                    void operator()( node_type * pNode )
                    {
                        dispose_tower( pNode )  ;
                    }
                };
            };

            // Forward declaration
            template <class GC, typename NodeTraits, typename BackOff, bool IsConst>
            class iterator ;

        } // namespace details
        //@endcond

    }   // namespace skip_list

    // Forward declaration
    template <class GC, typename T, typename Traits = skip_list::type_traits >
    class SkipListSet  ;

}}   // namespace cds::intrusive

#endif // #ifndef __CDS_INTRUSIVE_SKIP_LIST_BASE_H
