//$$CDS-header$$

#ifndef __CDS_INTRUSIVE_CUCKOO_SET_H
#define __CDS_INTRUSIVE_CUCKOO_SET_H

#include <cds/intrusive/base.h>
#include <cds/opt/compare.h>
#include <cds/opt/hash.h>
#include <cds/lock/array.h>
#include <cds/details/std/type_traits.h>
#include <cds/ref.h>
#include <cds/os/thread.h>
#include <cds/details/std/memory.h>
#include <cds/details/functor_wrapper.h>
#include <cds/lock/spinlock.h>

#include <cds/details/std/mutex.h>
//#include <boost/thread/recursive_mutex.hpp>

namespace cds { namespace intrusive {

    /// CuckooSet-related definitions
    namespace cuckoo {

        /// Option to define probeset type
        /**
            The option specifies probeset type for the CuckooSet.
            Available values:
            - \p cds::intrusive::cuckoo::list - the probeset is a single-linked list.
                The node contains pointer to next node in probeset.
            - \p cds::intrusive::cuckoo::vector<Capacity> - the probeset is a vector
                with constant-size \p Capacity where \p Capacity is an <tt>unsigned int</tt> constant.
                The node does not contain any auxiliary data.
        */
        template <typename Type>
        struct probeset_type
        {
            //@cond
            template <typename Base>
            struct pack: public Base {
                typedef Type    probeset_type ;
            };
            //@endcond
        };

        /// Option specifying whether to store hash values in the node
        /**
             This option reserves additional space in the hook to store the hash value of the object once it's introduced in the container.
             When this option is used, the unordered container will store the calculated hash value in the hook and rehashing operations won't need
             to recalculate the hash of the value. This option will improve the performance of unordered containers
             when rehashing is frequent or hashing the value is a slow operation

             The \p Count template parameter defines the size of hash array. Remember that cuckoo hashing implies at least two
             hash values per item.

             Possible values of \p Count:
             - 0 - no hash storing in the node
             - greater that 1 - store hash values.

             Value 1 is deprecated.
        */
        template <unsigned int Count>
        struct store_hash
        {
            //@cond
            template <typename Base>
            struct pack: public Base {
                static unsigned int const store_hash = Count ;
            };
            //@endcond
        };


        //@cond
        // Probeset type placeholders
        struct list_probeset_class      ;
        struct vector_probeset_class    ;
        //@endcond

        //@cond
        // Probeset type declarations.
        struct list    ;
        template <unsigned int Capacity>
        struct vector
        {
            static unsigned int const c_nCapacity = Capacity ;
        };
        //@endcond

        /// CuckooSet node
        /**
            Template arguments:
            - \p ProbesetType - type of probeset. Can be \p cds::intrusive::cuckoo::list
                or \p cds::intrusive::cuckoo::vector<Capacity>.
            - \p StoreHashCount - constant that defines whether to store node hash values.
                See cuckoo::store_hash option for explanation
            - Tag - a tag used to distinguish between different implementation when two or more
                \p node is needed in single struct.
        */
        template <typename ProbesetType = cuckoo::list, unsigned int StoreHashCount = 0, typename Tag = opt::none>
        struct node
#ifdef CDS_DOXYGEN_INVOKED
        {
            typedef ProbesetType    probeset_type   ;   ///< Probeset type
            typedef Tag             tag             ;   ///< Tag
            static unsigned int const hash_array_size = StoreHashCount ;    ///< The size of hash array
        }
#endif
        ;

        //@cond
        template <typename Tag>
        struct node< cuckoo::list, 0, Tag>
        {
            typedef list_probeset_class probeset_class  ;
            typedef cuckoo::list        probeset_type   ;
            typedef Tag                 tag ;
            static unsigned int const hash_array_size = 0 ;
            static unsigned int const probeset_size = 0 ;

            node *  m_pNext ;

            CDS_CONSTEXPR node() CDS_NOEXCEPT
                : m_pNext( null_ptr<node *>() )
            {}

            void store_hash( size_t * )
            {}

            size_t * get_hash() const
            {
                // This node type does not store hash values!!!
                assert(false) ;
                return null_ptr<size_t *>() ;
            }

            void clear()
            {
                m_pNext = null_ptr<node *>() ;
            }
        };

        template <unsigned int StoreHashCount, typename Tag>
        struct node< cuckoo::list, StoreHashCount, Tag>
        {
            typedef list_probeset_class probeset_class  ;
            typedef cuckoo::list        probeset_type   ;
            typedef Tag                 tag ;
            static unsigned int const hash_array_size = StoreHashCount ;
            static unsigned int const probeset_size = 0 ;

            node *  m_pNext ;
            size_t  m_arrHash[ hash_array_size ]    ;

            node() CDS_NOEXCEPT
                : m_pNext( null_ptr<node *>() )
            {
                memset( m_arrHash, 0, sizeof(m_arrHash)) ;
            }

            void store_hash( size_t * pHashes )
            {
                memcpy( m_arrHash, pHashes, hash_array_size ) ;
            }

            size_t * get_hash() const
            {
                return const_cast<size_t *>( m_arrHash ) ;
            }

            void clear()
            {
                m_pNext = null_ptr<node *>() ;
            }

        };

        template <unsigned int VectorSize, typename Tag>
        struct node< cuckoo::vector<VectorSize>, 0, Tag>
        {
            typedef vector_probeset_class       probeset_class  ;
            typedef cuckoo::vector<VectorSize>  probeset_type   ;
            typedef Tag                         tag ;
            static unsigned int const hash_array_size = 0 ;
            static unsigned int const probeset_size = probeset_type::c_nCapacity ;

            node() CDS_NOEXCEPT
            {}

            void store_hash( size_t * )
            {}

            size_t * get_hash() const
            {
                // This node type does not store hash values!!!
                assert(false) ;
                return null_ptr<size_t *>() ;
            }

            void clear()
            {}
        };

        template <unsigned int VectorSize, unsigned int StoreHashCount, typename Tag>
        struct node< cuckoo::vector<VectorSize>, StoreHashCount, Tag>
        {
            typedef vector_probeset_class       probeset_class  ;
            typedef cuckoo::vector<VectorSize>  probeset_type   ;
            typedef Tag                         tag ;
            static unsigned int const hash_array_size = StoreHashCount ;
            static unsigned int const probeset_size = probeset_type::c_nCapacity ;

            size_t  m_arrHash[ hash_array_size ]    ;

            node() CDS_NOEXCEPT
            {
                memset( m_arrHash, 0, sizeof(m_arrHash)) ;
            }

            void store_hash( size_t * pHashes )
            {
                memcpy( m_arrHash, pHashes, hash_array_size ) ;
            }

            size_t * get_hash() const
            {
                return const_cast<size_t *>( m_arrHash ) ;
            }

            void clear()
            {}
        };
        //@endcond


        //@cond
        struct default_hook {
            typedef cuckoo::list probeset_type  ;
            static unsigned int const store_hash = 0 ;
            typedef opt::none   tag ;
        };

        template < typename HookType, CDS_DECL_OPTIONS3>
        struct hook
        {
            typedef typename opt::make_options< default_hook, CDS_OPTIONS3>::type  options ;

            typedef typename options::probeset_type probeset_type   ;
            typedef typename options::tag tag   ;
            static unsigned int const store_hash = options::store_hash ;

            typedef node<probeset_type, store_hash, tag>   node_type   ;

            typedef HookType        hook_type   ;
        };
        //@endcond

        /// Base hook
        /**
            \p Options are:
            - cuckoo::probeset_type - probeset type. Defaul is \p cuckoo::list
            - cuckoo::store_hash - store hash values in the node or not. Default is 0 (no storing)
            - opt::tag - tag to distinguish different nodes in one struct. Default is opt::none
        */
        template < CDS_DECL_OPTIONS3 >
        struct base_hook: public hook< opt::base_hook_tag, CDS_OPTIONS3 >
        {};

        /// Member hook
        /**
            \p MemberOffset defines offset in bytes of \ref node member into your structure.
            Use \p offsetof macro to define \p MemberOffset

            \p Options are:
            - cuckoo::probeset_type - probeset type. Defaul is \p cuckoo::list
            - cuckoo::store_hash - store hash values in the node or not. Default is 0 (no storing)
            - opt::tag - tag to distinguish different nodes in one struct. Default is opt::none
        */
        template < size_t MemberOffset, CDS_DECL_OPTIONS3 >
        struct member_hook: public hook< opt::member_hook_tag, CDS_OPTIONS3 >
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
            - cuckoo::probeset_type - probeset type. Defaul is \p cuckoo::list
            - cuckoo::store_hash - store hash values in the node or not. Default is 0 (no storing)
            - opt::tag - tag to distinguish different nodes in one struct. Default is opt::none
        */
        template <typename NodeTraits, CDS_DECL_OPTIONS3 >
        struct traits_hook: public hook< opt::traits_hook_tag, CDS_OPTIONS3 >
        {
            //@cond
            typedef NodeTraits node_traits ;
            //@endcond
        };

        /// Internal statistics for \ref striping mutex policy
        struct striping_stat {
            typedef cds::atomicity::event_counter   counter_type;   ///< Counter type

            counter_type   m_nCellLockCount    ;  ///< Count of obtaining cell lock
            counter_type   m_nCellTryLockCount ;  ///< Count of cell \p try_lock attempts
            counter_type   m_nFullLockCount    ;  ///< Count of obtaining full lock
            counter_type   m_nResizeLockCount  ;  ///< Count of obtaining resize lock
            counter_type   m_nResizeCount      ;  ///< Count of resize event

            //@cond
            void    onCellLock()    { ++m_nCellLockCount; }
            void    onCellTryLock() { ++m_nCellTryLockCount; }
            void    onFullLock()    { ++m_nFullLockCount; }
            void    onResizeLock()  { ++m_nResizeLockCount;   }
            void    onResize()      { ++m_nResizeCount; }
            //@endcond
        };

        /// Dummy internal statistics for \ref striping mutex policy
        struct empty_striping_stat {
            //@cond
            void    onCellLock()    const {}
            void    onCellTryLock() const {}
            void    onFullLock()    const {}
            void    onResizeLock()  const {}
            void    onResize()      const {}
            //@endcond
        };

        /// Lock striping concurrent access policy
        /**
            This is one of available opt::mutex_policy option type for CuckooSet

            Lock striping is very simple technique.
            The cuckoo set consists of the bucket tables and the array of locks.
            There is single lock array for each bucket table, at least, the count of bucket table is 2.
            Initially, the capacity of lock array and each bucket table is the same.
            When set is resized, bucket table capacity will be doubled but lock array will not.
            The lock \p i protects each bucket \p j, where <tt> j = i mod L </tt>,
            where \p L - the size of lock array.

            The policy contains an internal array of \p RecursiveLock locks.

            Template arguments:
            - \p RecursiveLock - the type of recursive mutex. The default is \p cds_std::recursive_mutex. The mutex type should be default-constructible.
                Note that a recursive spin-lock is not suitable for lock striping for performance reason.
            - \p Arity - unsigned int constant that specifies an arity. The arity is the count of hash functors, i.e., the
                count of lock arrays. Default value is 2.
            - \p Alloc - allocator type used for lock array memory allocation. Default is \p CDS_DEFAULT_ALLOCATOR.
            - \p Stat - internal statistics type. Note that this template argument is automatically selected by \ref CuckooSet
                class according to its \p opt::stat option.
        */
        template <
            class RecursiveLock = cds_std::recursive_mutex,
            unsigned int Arity = 2,
            class Alloc = CDS_DEFAULT_ALLOCATOR,
            class Stat = empty_striping_stat
        >
        class striping
        {
        public:
            typedef RecursiveLock   lock_type       ;   ///< lock type
            typedef Alloc           allocator_type  ;   ///< allocator type
            static unsigned int const c_nArity = Arity ;    ///< the arity
            typedef Stat            statistics_type ;   ///< Internal statistics type (\ref striping_stat or \ref empty_striping_stat)

            //@cond
            typedef striping_stat       real_stat  ;
            typedef empty_striping_stat empty_stat ;

            template <typename Stat2>
            struct rebind_statistics {
                typedef striping<lock_type, c_nArity, allocator_type, Stat2> other ;
            };
            //@endcond

            typedef cds::lock::array< lock_type, cds::lock::pow2_select_policy, allocator_type >    lock_array_type ;   ///< lock array type

        protected:
            //@cond
            class lock_array: public lock_array_type
            {
            public:
                // placeholder ctor
                lock_array(): lock_array_type( typename lock_array_type::select_cell_policy(2) ) {}

                // real ctor
                lock_array( size_t nCapacity ): lock_array_type( nCapacity, typename lock_array_type::select_cell_policy(nCapacity) ) {}
            };

            class scoped_lock: public cds::lock::scoped_lock< lock_array_type >
            {
                typedef cds::lock::scoped_lock< lock_array_type > base_class ;
            public:
                scoped_lock( lock_array& arrLock, size_t nHash ): base_class( arrLock, nHash ) {}
            };
            //@endcond

        protected:
            //@cond
            lock_array      m_Locks[c_nArity] ;   ///< array of lock_array_type
            statistics_type m_Stat              ; ///< internal statistics
            //@endcond

        public:
            //@cond
            class scoped_cell_lock {
                lock_type * m_guard[c_nArity]   ;

            public:
                scoped_cell_lock( striping& policy, size_t const* arrHash )
                {
                    for ( unsigned int i = 0; i < c_nArity; ++i ) {
                        m_guard[i] = &( policy.m_Locks[i].at( policy.m_Locks[i].lock( arrHash[i] ))) ;
                    }
                    policy.m_Stat.onCellLock() ;
                }

                ~scoped_cell_lock()
                {
                    for ( unsigned int i = 0; i < c_nArity; ++i )
                        m_guard[i]->unlock()    ;
                }
            };

            class scoped_cell_trylock
            {
                typedef typename lock_array_type::lock_type lock_type ;

                lock_type * m_guard[c_nArity]   ;
                bool        m_bLocked           ;

            public:
                scoped_cell_trylock( striping& policy, size_t const* arrHash )
                {
                    size_t nCell = policy.m_Locks[0].try_lock( arrHash[0] ) ;
                    m_bLocked = nCell != lock_array_type::c_nUnspecifiedCell ;
                    if ( m_bLocked ) {
                        m_guard[0] = &(policy.m_Locks[0].at(nCell)) ;
                        for ( unsigned int i = 1; i < c_nArity; ++i ) {
                            m_guard[i] = &( policy.m_Locks[i].at( policy.m_Locks[i].lock( arrHash[i] )) ) ;
                        }
                    }
                    else {
                        std::fill( m_guard, m_guard + c_nArity, null_ptr<lock_type *>() ) ;
                    }
                    policy.m_Stat.onCellTryLock() ;
                }
                ~scoped_cell_trylock()
                {
                    if ( m_bLocked ) {
                        for ( unsigned int i = 0; i < c_nArity; ++i )
                            m_guard[i]->unlock() ;
                    }
                }

                bool locked() const
                {
                    return m_bLocked ;
                }
            };

            class scoped_full_lock {
                cds::lock::scoped_lock< lock_array_type >   m_guard  ;
            public:
                scoped_full_lock( striping& policy )
                    : m_guard( policy.m_Locks[0] )
                {
                    policy.m_Stat.onFullLock() ;
                }

                /// Ctor for scoped_resize_lock - no statistics is incremented
                scoped_full_lock( striping& policy, bool )
                    : m_guard( policy.m_Locks[0] )
                {}
            };

            class scoped_resize_lock: public scoped_full_lock {
            public:
                scoped_resize_lock( striping& policy )
                    : scoped_full_lock( policy, false )
                {
                    policy.m_Stat.onResizeLock() ;
                }
            };
            //@endcond

        public:
            /// Constructor
            striping(
                size_t nLockCount          ///< The size of lock array. Must be power of two.
            )
            {
                // Trick: initialize the array of locks
                for ( unsigned int i = 0; i < c_nArity; ++i ) {
                    lock_array * pArr = m_Locks + i ;
                    pArr->lock_array::~lock_array()         ;
                    new ( pArr ) lock_array( nLockCount )   ;
                }
            }

            /// Returns lock array size
            /**
                Lock array size is unchanged during \p striping object lifetime
            */
            size_t lock_count() const
            {
                return m_Locks[0].size()   ;
            }

            //@cond
            void resize( size_t )
            {
                m_Stat.onResize() ;
            }
            //@endcond

            /// Returns the arity of striping mutex policy
            CDS_CONSTEXPR unsigned int arity() const CDS_NOEXCEPT
            {
                return c_nArity ;
            }

            /// Returns internal statistics
            statistics_type const& statistics() const
            {
                return m_Stat ;
            }
        };

        /// Internal statistics for \ref refinable mutex policy
        struct refinable_stat {
            typedef cds::atomicity::event_counter   counter_type    ;   ///< Counter type

            counter_type   m_nCellLockCount         ;   ///< Count of obtaining cell lock
            counter_type   m_nCellLockWaitResizing  ;   ///< Count of loop iteration to wait for resizing
            counter_type   m_nCellLockArrayChanged  ;   ///< Count of event "Lock array has been changed when obtaining cell lock"
            counter_type   m_nCellLockFailed        ;   ///< Count of event "Cell lock failed because of the array is owned by other thread"

            counter_type   m_nSecondCellLockCount   ;   ///< Count of obtaining cell lock when another cell is already locked
            counter_type   m_nSecondCellLockFailed  ;   ///< Count of unsuccess obtaining cell lock when another cell is already locked

            counter_type   m_nFullLockCount         ;   ///< Count of obtaining full lock
            counter_type   m_nFullLockIter          ;   ///< Count of unsuccessfull iteration to obtain full lock

            counter_type   m_nResizeLockCount       ;   ///< Count of obtaining resize lock
            counter_type   m_nResizeLockIter        ;   ///< Count of unsuccessfull iteration to obtain resize lock
            counter_type   m_nResizeLockArrayChanged;   ///< Count of event "Lock array has been changed when obtaining resize lock"
            counter_type   m_nResizeCount           ;   ///< Count of resize event

            //@cond
            void    onCellLock()    { ++m_nCellLockCount; }
            void    onCellWaitResizing() { ++m_nCellLockWaitResizing; }
            void    onCellArrayChanged() { ++m_nCellLockArrayChanged; }
            void    onCellLockFailed()   { ++m_nCellLockFailed; }
            void    onSecondCellLock()   { ++m_nSecondCellLockCount; }
            void    onSecondCellLockFailed() { ++m_nSecondCellLockFailed; }
            void    onFullLock()         { ++m_nFullLockCount; }
            void    onFullLockIter()     { ++m_nFullLockIter; }
            void    onResizeLock()       { ++m_nResizeLockCount;   }
            void    onResizeLockIter()   { ++m_nResizeLockIter; }
            void    onResizeLockArrayChanged() { ++m_nResizeLockArrayChanged; }
            void    onResize()           { ++m_nResizeCount; }
            //@endcond
        };

        /// Dummy internal statistics for \ref refinable mutex policy
        struct empty_refinable_stat {
            //@cond
            void    onCellLock()            const {}
            void    onCellWaitResizing()    const {}
            void    onCellArrayChanged()    const {}
            void    onCellLockFailed()      const {}
            void    onSecondCellLock()      const {}
            void    onSecondCellLockFailed() const {}
            void    onFullLock()            const {}
            void    onFullLockIter()        const {}
            void    onResizeLock()          const {}
            void    onResizeLockIter()      const {}
            void    onResizeLockArrayChanged() const {}
            void    onResize()              const {}
            //@endcond
        };

        /// Refinable concurrent access policy
        /**
            This is one of available opt::mutex_policy option type for CuckooSet

            Refining is like a striping technique (see cuckoo::striping)
            but it allows growing the size of lock array when resizing the hash table.
            So, the sizes of hash table and lock array are equal.

            Template arguments:
            - \p RecursiveLock - the type of mutex. Reentrant (recursive) mutex is required.
                The default is \p cds_std::recursive_mutex. The mutex type should be default-constructible.
            - \p Arity - unsigned int constant that specifies an arity. The arity is the count of hash functors, i.e., the
                count of lock arrays. Default value is 2.
            - \p BackOff - back-off strategy. Default is cds::backoff::yield
            - \p Alloc - allocator type used for lock array memory allocation. Default is \p CDS_DEFAULT_ALLOCATOR.
            - \p Stat - internal statistics type. Note that this template argument is automatically selected by \ref CuckooSet
                class according to its \p opt::stat option.
        */
        template <
            class RecursiveLock = cds_std::recursive_mutex,
            unsigned int Arity = 2,
            typename BackOff = cds::backoff::yield,
            class Alloc = CDS_DEFAULT_ALLOCATOR,
            class Stat = empty_refinable_stat
        >
        class refinable
        {
        public:
            typedef RecursiveLock   lock_type       ;   ///< lock type
            typedef Alloc           allocator_type  ;   ///< allocator type
            typedef BackOff         back_off        ;   ///< back-off strategy
            typedef Stat            statistics_type ;   ///< internal statistics type
            static unsigned int const c_nArity = Arity; ///< the arity

            //@cond
            typedef refinable_stat          real_stat ;
            typedef empty_refinable_stat    empty_stat;

            template <typename Stat2>
            struct rebind_statistics {
                typedef refinable< lock_type, c_nArity, back_off, allocator_type, Stat2>    other ;
            };
            //@endcond

        protected:
            //@cond
            typedef cds::lock::trivial_select_policy  lock_selection_policy   ;

            class lock_array_type
                : public cds::lock::array< lock_type, lock_selection_policy, allocator_type >
                , public std::enable_shared_from_this< lock_array_type >
            {
                typedef cds::lock::array< lock_type, lock_selection_policy, allocator_type >    lock_array_base ;
            public:
                lock_array_type( size_t nCapacity )
                    : lock_array_base( nCapacity )
                {}
            };
            typedef std::shared_ptr< lock_array_type >  lock_array_ptr ;
            typedef cds::details::Allocator< lock_array_type, allocator_type >  lock_array_allocator    ;

            typedef unsigned long long  owner_t     ;
            typedef cds::OS::ThreadId   threadId_t  ;

            typedef cds::lock::Spin     spinlock_type ;
            typedef cds::lock::scoped_lock< spinlock_type > scoped_spinlock ;
            //@endcond

        protected:
            //@cond
            static owner_t const c_nOwnerMask = (((owner_t) 1) << (sizeof(owner_t) * 8 - 1)) - 1 ;

            CDS_ATOMIC::atomic< owner_t >   m_Owner     ;   ///< owner mark (thread id + boolean flag)
            CDS_ATOMIC::atomic<size_t>      m_nCapacity ;   ///< lock array capacity
            lock_array_ptr                  m_arrLocks[ c_nArity ]  ; ///< Lock array. The capacity of array is specified in constructor.
            spinlock_type                   m_access    ;   ///< access to m_arrLocks
            statistics_type                 m_Stat      ;   ///< internal statistics
            //@endcond

        protected:
            //@cond
            struct lock_array_disposer {
                void operator()( lock_array_type * pArr )
                {
                    lock_array_allocator().Delete( pArr )   ;
                }
            };

            lock_array_ptr create_lock_array( size_t nCapacity )
            {
                return lock_array_ptr( lock_array_allocator().New( nCapacity ), lock_array_disposer() ) ;
            }

            void acquire( size_t const * arrHash, lock_array_ptr * pLockArr, lock_type ** parrLock )
            {
                owner_t me = (owner_t) cds::OS::getCurrentThreadId() ;
                owner_t who ;

                back_off bkoff  ;
                while ( true ) {

                    {
                        scoped_spinlock sl(m_access) ;
                        for ( unsigned int i = 0; i < c_nArity; ++i )
                            pLockArr[i] = m_arrLocks[i] ;
                    }

                    // wait while resizing
                    while ( true ) {
                        who = m_Owner.load( CDS_ATOMIC::memory_order_acquire )  ;
                        if ( !( who & 1 ) || (who >> 1) == (me & c_nOwnerMask) )
                            break   ;
                        bkoff()     ;
                        m_Stat.onCellWaitResizing() ;
                    }

                    if ( pLockArr[0] != m_arrLocks[0] ) {
                        m_Stat.onCellArrayChanged() ;
                    }
                    else {

                        size_t const nMask = pLockArr[0]->size() - 1  ;
                        assert( cds::beans::is_power2( nMask + 1 )) ;

                        for ( unsigned int i = 0; i < c_nArity; ++i ) {
                            parrLock[i] = &( pLockArr[i]->at( arrHash[i] & nMask )) ;
                            parrLock[i]->lock() ;
                        }

                        who = m_Owner.load( CDS_ATOMIC::memory_order_acquire )  ;
                        if ( ( !(who & 1) || (who >> 1) == (me & c_nOwnerMask) ) && m_arrLocks[0] == pLockArr[0] ) {
                            m_Stat.onCellLock() ;
                            return ;
                        }

                        for ( unsigned int i = 0; i < c_nArity; ++i ) {
                            parrLock[i]->unlock() ;
                        }

                        m_Stat.onCellLockFailed() ;
                    }

                    // clears pLockArr can lead to calling dtor for each item of pLockArr[i] that may be a heavy-weighted operation
                    // (each pLockArr[i] is a shared pointer to array of a ton of mutexes)
                    // It is better to do this before the next loop iteration where we will use spin-locked assignment to pLockArr
                    // Destructing a lot of mutexes under spin-lock is a bad solution
                    for ( unsigned int i = 0; i < c_nArity; ++i )
                        pLockArr[i].reset() ;
                }
            }

            bool try_second_acquire( size_t const * arrHash, lock_type ** parrLock )
            {
                // It is assumed that the current thread already has a lock
                // and requires a second lock for other hash

                size_t const nMask = m_nCapacity.load(CDS_ATOMIC::memory_order_acquire) - 1 ;
                size_t nCell = m_arrLocks[0]->try_lock( arrHash[0] & nMask) ;
                if ( nCell == lock_array_type::c_nUnspecifiedCell ) {
                    m_Stat.onSecondCellLockFailed() ;
                    return false ;
                }
                parrLock[0] = &(m_arrLocks[0]->at(nCell)) ;

                for ( unsigned int i = 1; i < c_nArity; ++i ) {
                    parrLock[i] = &( m_arrLocks[i]->at( m_arrLocks[i]->lock( arrHash[i] & nMask)) ) ;
                }

                m_Stat.onSecondCellLock() ;
                return true ;
            }

            void acquire_all()
            {
                owner_t me = (owner_t) cds::OS::getCurrentThreadId() ;

                back_off bkoff ;
                while ( true ) {
                    owner_t ownNull = 0 ;
                    if ( m_Owner.compare_exchange_strong( ownNull, (me << 1) | 1, CDS_ATOMIC::memory_order_acq_rel, CDS_ATOMIC::memory_order_relaxed )) {
                        m_arrLocks[0]->lock_all() ;

                        m_Stat.onFullLock() ;
                        return ;
                    }
                    bkoff() ;
                    m_Stat.onFullLockIter() ;
                }
            }

            void release_all()
            {
                m_arrLocks[0]->unlock_all() ;
                m_Owner.store( 0, CDS_ATOMIC::memory_order_release )  ;
            }

            void acquire_resize( lock_array_ptr * pOldLocks )
            {
                owner_t me = (owner_t) cds::OS::getCurrentThreadId() ;

                while ( true ) {
                    {
                        scoped_spinlock sl(m_access) ;
                        for ( unsigned int i = 0; i < c_nArity; ++i )
                            pOldLocks[i] = m_arrLocks[i] ;
                    }

                    // global lock
                    owner_t ownNull = 0 ;
                    if ( m_Owner.compare_exchange_strong( ownNull, (me << 1) | 1, CDS_ATOMIC::memory_order_acq_rel, CDS_ATOMIC::memory_order_relaxed )) {
                        if ( pOldLocks[0] != m_arrLocks[0] ) {
                            m_Owner.store( 0, CDS_ATOMIC::memory_order_release ) ;
                            m_Stat.onResizeLockArrayChanged() ;
                        }
                        else {
                            pOldLocks[0]->lock_all() ;
                            m_Stat.onResizeLock() ;
                            return ;
                        }
                    }
                    else
                        m_Stat.onResizeLockIter() ;

                    // clears pOldLocks can lead to calling dtor for each item of pOldLocks[i] that may be a heavy-weighted operation
                    // (each pOldLocks[i] is a shared pointer to array of a ton of mutexes)
                    // It is better to do this before the next loop iteration where we will use spin-locked assignment to pOldLocks
                    // Destructing a lot of mutexes under spin-lock is a bad solution
                    for ( unsigned int i = 0; i < c_nArity; ++i )
                        pOldLocks[i].reset() ;
                }
            }

            void release_resize( lock_array_ptr * pOldLocks )
            {
                m_Owner.store( 0, CDS_ATOMIC::memory_order_release )  ;
                pOldLocks[0]->unlock_all() ;
            }
            //@endcond

        public:
            //@cond
            class scoped_cell_lock {
                lock_type * m_arrLock[ c_nArity ]   ;
                lock_array_ptr  m_arrLockArr[ c_nArity ]  ;

            public:
                scoped_cell_lock( refinable& policy, size_t const* arrHash )
                {
                    policy.acquire( arrHash, m_arrLockArr, m_arrLock )    ;
                }

                ~scoped_cell_lock()
                {
                    for ( unsigned int i = 0; i < c_nArity; ++i )
                        m_arrLock[i]->unlock() ;
                }
            };

            class scoped_cell_trylock {
                lock_type * m_arrLock[ c_nArity ]   ;
                bool        m_bLocked               ;

            public:
                scoped_cell_trylock( refinable& policy, size_t const* arrHash )
                {
                    m_bLocked = policy.try_second_acquire( arrHash, m_arrLock ) ;
                }

                ~scoped_cell_trylock()
                {
                    if ( m_bLocked ) {
                        for ( unsigned int i = 0; i < c_nArity; ++i )
                            m_arrLock[i]->unlock() ;
                    }
                }

                bool locked() const
                {
                    return m_bLocked ;
                }
            };

            class scoped_full_lock {
                refinable& m_policy ;
            public:
                scoped_full_lock( refinable& policy )
                    : m_policy( policy )
                {
                    policy.acquire_all() ;
                }
                ~scoped_full_lock()
                {
                    m_policy.release_all() ;
                }
            };

            class scoped_resize_lock
            {
                refinable&      m_policy                ;
                lock_array_ptr  m_arrLocks[ c_nArity ]  ;
            public:
                scoped_resize_lock( refinable& policy )
                    : m_policy(policy)
                {
                    policy.acquire_resize( m_arrLocks ) ;
                }
                ~scoped_resize_lock()
                {
                    m_policy.release_resize( m_arrLocks ) ;
                }
            };
            //@endcond

        public:
            /// Constructor
            refinable(
                size_t nLockCount   ///< The size of lock array. Must be power of two.
            )   : m_Owner(0)
                , m_nCapacity( nLockCount )
            {
                assert( cds::beans::is_power2( nLockCount )) ;
                for ( unsigned int i = 0; i < c_nArity; ++i )
                    m_arrLocks[i] = create_lock_array( nLockCount ) ;
            }

            //@cond
            void resize( size_t nCapacity )
            {
                lock_array_ptr pNew[ c_nArity ] ;
                for ( unsigned int i = 0; i < c_nArity; ++i )
                    pNew[i] = create_lock_array( nCapacity ) ;

                /*
                // Assignment m_arrLocks[i] = pNew[i] may call heavy-weighted dtor for each item of m_arrLocks
                // that is unacceptable under spin-lock
                // So, we store copy of m_arrLocks in pOld
                lock_array_ptr pOld[ c_nArity ] ;
                for ( unsigned int i = 0; i < c_nArity; ++i )
                    pOld[i] = m_arrLocks[i] ;

                // m_arrLocks assignment will not lead to calling dtor of each item of m_arrLocks
                // since copy of m_arrLocks locates in pOld and assignment will not be too painful for spin-lock
                */

                {
                    scoped_spinlock sl(m_access) ;
                    for ( unsigned int i = 0; i < c_nArity; ++i )
                        m_arrLocks[i] = pNew[i] ;
                }
                m_nCapacity.store( nCapacity, CDS_ATOMIC::memory_order_release ) ;

                m_Stat.onResize() ;
            }
            //@endcond

            /// Returns lock array size
            /**
                Lock array size is not a constant for \p refinable policy and can be changed when the set is resized.
            */
            size_t lock_count() const
            {
                return m_nCapacity.load(CDS_ATOMIC::memory_order_relaxed) ;
            }

            /// Returns the arity of \p refinable mutex policy
            CDS_CONSTEXPR unsigned int arity() const CDS_NOEXCEPT
            {
                return c_nArity ;
            }

            /// Returns internal statistics
            statistics_type const& statistics() const
            {
                return m_Stat ;
            }
        };

        /// CuckooSet internal statistics
        struct stat {
            typedef cds::atomicity::event_counter   counter_type ;  ///< Counter type

            counter_type    m_nRelocateCallCount    ; ///< Count of \p relocate function call
            counter_type    m_nRelocateRoundCount   ; ///< Count of attempts to relocate items
            counter_type    m_nFalseRelocateCount   ; ///< Count of unneeded attempts of \p relocate call
            counter_type    m_nSuccessRelocateCount ; ///< Count of successfull item relocating
            counter_type    m_nRelocateAboveThresholdCount; ///< Count of item relocating above probeset threshold
            counter_type    m_nFailedRelocateCount  ;   ///< Count of failed relocation attemp (when all probeset is full)

            counter_type    m_nResizeCallCount      ;   ///< Count of \p resize function call
            counter_type    m_nFalseResizeCount     ;   ///< Count of false \p resize function call (when other thread has been resized the set)
            counter_type    m_nResizeSuccessNodeMove;   ///< Count of successfull node moving when resizing
            counter_type    m_nResizeRelocateCall   ;   ///< Count of \p relocate function call from \p resize function

            counter_type    m_nInsertSuccess        ;   ///< Count of successfull \p insert function call
            counter_type    m_nInsertFailed         ;   ///< Count of failed \p insert function call
            counter_type    m_nInsertResizeCount    ;   ///< Count of \p resize function call from \p insert
            counter_type    m_nInsertRelocateCount  ;   ///< Count of \p relocate function call from \p insert
            counter_type    m_nInsertRelocateFault  ;   ///< Count of failed \p relocate function call from \p insert

            counter_type    m_nEnsureExistCount     ;   ///< Count of call \p ensure function for existing node
            counter_type    m_nEnsureSuccessCount   ;   ///< Count of successfull \p insert function call for new node
            counter_type    m_nEnsureResizeCount    ;   ///< Count of \p resize function call from \p ensure
            counter_type    m_nEnsureRelocateCount  ;   ///< Count of \p relocate function call from \p ensure
            counter_type    m_nEnsureRelocateFault  ;   ///< Count of failed \p relocate function call from \p ensure

            counter_type    m_nUnlinkSuccess        ;   ///< Count of success \p unlink function call
            counter_type    m_nUnlinkFailed         ;   ///< Count of failed \p unlink function call

            counter_type    m_nEraseSuccess         ;   ///< Count of success \p erase function call
            counter_type    m_nEraseFailed          ;   ///< Count of failed \p erase function call

            counter_type    m_nFindSuccess         ;   ///< Count of success \p find function call
            counter_type    m_nFindFailed          ;   ///< Count of failed \p find function call

            counter_type    m_nFindEqualSuccess         ;   ///< Count of success \p find_equal function call
            counter_type    m_nFindEqualFailed          ;   ///< Count of failed \p find_equal function call

            counter_type    m_nFindWithSuccess         ;   ///< Count of success \p find_with function call
            counter_type    m_nFindWithFailed          ;   ///< Count of failed \p find_with function call

            //@cond
            void    onRelocateCall()        { ++m_nRelocateCallCount; }
            void    onRelocateRound()       { ++m_nRelocateRoundCount; }
            void    onFalseRelocateRound()  { ++m_nFalseRelocateCount; }
            void    onSuccessRelocateRound(){ ++m_nSuccessRelocateCount; }
            void    onRelocateAboveThresholdRound() { ++m_nRelocateAboveThresholdCount; }
            void    onFailedRelocate()      { ++m_nFailedRelocateCount; }

            void    onResizeCall()          { ++m_nResizeCallCount; }
            void    onFalseResizeCall()     { ++m_nFalseResizeCount; }
            void    onResizeSuccessMove()   { ++m_nResizeSuccessNodeMove; }
            void    onResizeRelocateCall()  { ++m_nResizeRelocateCall; }

            void    onInsertSuccess()       { ++m_nInsertSuccess; }
            void    onInsertFailed()        { ++m_nInsertFailed; }
            void    onInsertResize()        { ++m_nInsertResizeCount; }
            void    onInsertRelocate()      { ++m_nInsertRelocateCount; }
            void    onInsertRelocateFault() { ++m_nInsertRelocateFault; }

            void    onEnsureExist()         { ++m_nEnsureExistCount; }
            void    onEnsureSuccess()       { ++m_nEnsureSuccessCount; }
            void    onEnsureResize()        { ++m_nEnsureResizeCount; }
            void    onEnsureRelocate()      { ++m_nEnsureRelocateCount; }
            void    onEnsureRelocateFault() { ++m_nEnsureRelocateFault; }

            void    onUnlinkSuccess()       { ++m_nUnlinkSuccess; }
            void    onUnlinkFailed()        { ++m_nUnlinkFailed; }

            void    onEraseSuccess()        { ++m_nEraseSuccess; }
            void    onEraseFailed()         { ++m_nEraseFailed; }

            void    onFindSuccess()         { ++m_nFindSuccess; }
            void    onFindFailed()          { ++m_nFindFailed; }

            void    onFindWithSuccess()     { ++m_nFindWithSuccess; }
            void    onFindWithFailed()      { ++m_nFindWithFailed; }
            //@endcond
        };

        /// CuckooSet empty internal statistics
        struct empty_stat {
            //@cond
            void    onRelocateCall()        const {}
            void    onRelocateRound()       const {}
            void    onFalseRelocateRound()  const {}
            void    onSuccessRelocateRound()const {}
            void    onRelocateAboveThresholdRound() const {}
            void    onFailedRelocate()      const {}

            void    onResizeCall()          const {}
            void    onFalseResizeCall()     const {}
            void    onResizeSuccessMove()   const {}
            void    onResizeRelocateCall()  const {}

            void    onInsertSuccess()       const {}
            void    onInsertFailed()        const {}
            void    onInsertResize()        const {}
            void    onInsertRelocate()      const {}
            void    onInsertRelocateFault() const {}

            void    onEnsureExist()         const {}
            void    onEnsureSuccess()       const {}
            void    onEnsureResize()        const {}
            void    onEnsureRelocate()      const {}
            void    onEnsureRelocateFault() const {}

            void    onUnlinkSuccess()       const {}
            void    onUnlinkFailed()        const {}

            void    onEraseSuccess()        const {}
            void    onEraseFailed()         const {}

            void    onFindSuccess()         const {}
            void    onFindFailed()          const {}

            void    onFindWithSuccess()     const {}
            void    onFindWithFailed()      const {}
            //@endcond
        };

        /// Type traits for CuckooSet class
        struct type_traits
        {
            /// Hook used
            /**
                Possible values are: cuckoo::base_hook, cuckoo::member_hook, cuckoo::traits_hook.
            */
            typedef base_hook<>         hook ;

            /// Hash functors tuple
            /**
                This is mandatory type and has no predefined one.

                At least, two hash functors should be provided. All hash functor
                should be orthogonal (different): for each <tt> i,j: i != j => h[i](x) != h[j](x) </tt>.
                The hash functors are defined as <tt> std::tuple< H1, H2, ... Hn > </tt>:
                \@code cds::opt::hash< std::tuple< h1, h2 > > \@endcode
                The number of hash functors specifies the number \p k - the count of hash tables in cuckoo hashing.
                Up to 10 different hash functors are supported.
            */
            typedef cds::opt::none      hash ;

            /// Concurrent access policy
            /**
                Available opt::mutex_policy types:
                - cuckoo::striping - simple, but the lock array is not resizable
                - cuckoo::refinable - resizable lock array, but more complex access to set data.

                Default is cuckoo::striping.
            */
            typedef cuckoo::striping<>               mutex_policy   ;

            /// Key equality functor
            /**
                Default is <tt>std::equal_to<T></tt>
            */
            typedef opt::none                       equal_to    ;

            /// Key comparison functor
            /**
                No default functor is provided. If the option is not specified, the \p less is used.
            */
            typedef opt::none                       compare     ;

            /// specifies binary predicate used for key comparison.
            /**
                Default is \p std::less<T>.
            */
            typedef opt::none                       less        ;

            /// Item counter
            /**
                The type for item counting feature.
                Default is cds::atomicity::item_counter

                Only atomic item counter type is allowed.
            */
            typedef atomicity::item_counter             item_counter;

            /// Allocator type
            /**
                The allocator type for allocating bucket tables.
            */
            typedef CDS_DEFAULT_ALLOCATOR       allocator;

            /// Disposer
            /**
                The disposer functor is used in CuckooSet::clear member function
                to free set's node.
            */
            typedef intrusive::opt::v::empty_disposer   disposer ;

            /// Internal statistics. Available statistics: cuckoo::stat, cuckoo::empty_stat
            typedef empty_stat                  stat ;
        };

        /// Metafunction converting option list to CuckooSet traits
        /**
            This is a wrapper for <tt> cds::opt::make_options< type_traits, Options...> </tt>
            \p Options list see \ref CuckooSet.
        */
        template <CDS_DECL_OPTIONS11>
        struct make_traits {
            typedef typename cds::opt::make_options<
                typename cds::opt::find_type_traits< cuckoo::type_traits, CDS_OPTIONS10 >::type
                ,CDS_OPTIONS11
            >::type   type ;    ///< Result of metafunction
        };

        //@cond
        namespace details {
            template <typename Node, typename Probeset>
            class bucket_entry ;

            template <typename Node>
            class bucket_entry<Node, cuckoo::list>
            {
            public:
                typedef Node                        node_type       ;
                typedef cuckoo::list_probeset_class probeset_class  ;
                typedef cuckoo::list                probeset_type   ;

            protected:
                node_type *     pHead   ;
                unsigned int    nSize   ;

            public:
                class iterator
                {
                    node_type * pNode   ;
                    friend class bucket_entry ;

                public:
                    iterator()
                        : pNode( null_ptr<node_type *>())
                    {}
                    iterator( node_type * p )
                        : pNode( p )
                    {}
                    iterator( iterator const& it)
                        : pNode( it.pNode )
                    {}

                    iterator& operator=( iterator const& it )
                    {
                        pNode = it.pNode    ;
                        return *this        ;
                    }

                    iterator& operator=( node_type * p )
                    {
                        pNode = p       ;
                        return *this    ;
                    }

                    node_type * operator->()
                    {
                        return pNode    ;
                    }
                    node_type& operator*()
                    {
                        assert( pNode != null_ptr<node_type *>()) ;
                        return *pNode ;
                    }

                    // preinc
                    iterator& operator ++()
                    {
                        if ( pNode )
                            pNode = pNode->m_pNext  ;
                        return *this ;
                    }

                    bool operator==(iterator const& it ) const
                    {
                        return pNode == it.pNode ;
                    }
                    bool operator!=(iterator const& it ) const
                    {
                        return !( *this == it ) ;
                    }
                };

            public:
                bucket_entry()
                    : pHead( null_ptr<node_type *>())
                    , nSize(0)
                {
                    static_assert(( std::is_same<typename node_type::probeset_type, probeset_type>::value ), "Incompatible node type" ) ;
                }

                iterator begin()
                {
                    return iterator(pHead)  ;
                }
                iterator end()
                {
                    return iterator()   ;
                }

                void insert_after( iterator it, node_type * p )
                {
                    node_type * pPrev = it.pNode    ;
                    if ( pPrev ) {
                        p->m_pNext = pPrev->m_pNext ;
                        pPrev->m_pNext = p          ;
                    }
                    else {
                        // insert as head
                        p->m_pNext = pHead ;
                        pHead = p   ;
                    }
                    ++nSize ;
                }

                void remove( iterator itPrev, iterator itWhat )
                {
                    node_type * pPrev = itPrev.pNode    ;
                    node_type * pWhat = itWhat.pNode    ;
                    assert( (!pPrev && pWhat == pHead) || (pPrev && pPrev->m_pNext == pWhat) )   ;

                    if ( pPrev )
                        pPrev->m_pNext = pWhat->m_pNext ;
                    else {
                        assert( pWhat == pHead )    ;
                        pHead = pHead->m_pNext      ;
                    }
                    pWhat->clear() ;
                    --nSize ;
                }

                void clear()
                {
                    node_type * pNext ;
                    for ( node_type * pNode = pHead; pNode; pNode = pNext ) {
                        pNext = pNode->m_pNext      ;
                        pNode->clear()              ;
                    }

                    nSize = 0 ;
                    pHead = null_ptr<node_type *>() ;
                }

                template <typename Disposer>
                void clear( Disposer disp )
                {
                    node_type * pNext ;
                    for ( node_type * pNode = pHead; pNode; pNode = pNext ) {
                        pNext = pNode->m_pNext      ;
                        pNode->clear()              ;
                        cds::unref(disp)( pNode )   ;
                    }

                    nSize = 0 ;
                    pHead = null_ptr<node_type *>() ;
                }

                unsigned int size() const
                {
                    return nSize ;
                }
            };

            template <typename Node, unsigned int Capacity>
            class bucket_entry<Node, cuckoo::vector<Capacity> >
            {
            public:
                typedef Node                            node_type       ;
                typedef cuckoo::vector_probeset_class   probeset_class  ;
                typedef cuckoo::vector<Capacity>        probeset_type   ;

                static unsigned int const c_nCapacity = probeset_type::c_nCapacity ;

            protected:
                node_type *     m_arrNode[c_nCapacity]    ;
                unsigned int    m_nSize ;

                void shift_up( unsigned int nFrom )
                {
                    assert( m_nSize < c_nCapacity ) ;

                    // std alorithm
                    if ( nFrom < m_nSize )
                        std::copy_backward( m_arrNode + nFrom, m_arrNode + m_nSize, m_arrNode + m_nSize + 1 ) ;

                    // alternative: low-level byte copying
                    //memmove( m_arrNode + nFrom + 1, m_arrNode + nFrom, (m_nSize - nFrom) * sizeof(m_arrNode[0]) )   ;
                }

                void shift_down( node_type ** pFrom )
                {
                    assert( m_arrNode <= pFrom && pFrom < m_arrNode + m_nSize) ;
                    // std algo
                    std::copy( pFrom + 1, m_arrNode + m_nSize, pFrom  )  ;

                    // alternative: low-level byte copying
                    //memmove( pFrom + 1, pFrom, (m_nSize - nFrom - 1) * sizeof(m_arrNode[0])) ;
                }
            public:
                class iterator
                {
                    node_type **    pArr    ;
                    friend class bucket_entry ;

                public:
                    iterator()
                        : pArr( null_ptr<node_type **>() )
                    {}
                    iterator( node_type ** p )
                        : pArr(p)
                    {}
                    iterator( iterator const& it)
                        : pArr( it.pArr )
                    {}

                    iterator& operator=( iterator const& it )
                    {
                        pArr = it.pArr      ;
                        return *this        ;
                    }

                    node_type * operator->()
                    {
                        assert( pArr != null_ptr<node_type **>()) ;
                        return *pArr ;
                    }
                    node_type& operator*()
                    {
                        assert( pArr != null_ptr<node_type **>()) ;
                        assert( *pArr != null_ptr<node_type *>()) ;
                        return *(*pArr)  ;
                    }

                    // preinc
                    iterator& operator ++()
                    {
                        ++pArr  ;
                        return *this ;
                    }

                    bool operator==(iterator const& it ) const
                    {
                        return pArr == it.pArr ;
                    }
                    bool operator!=(iterator const& it ) const
                    {
                        return !( *this == it ) ;
                    }
                };

            public:
                bucket_entry()
                    : m_nSize(0)
                {
                    memset( m_arrNode, 0, sizeof(m_arrNode))    ;
                    static_assert(( std::is_same<typename node_type::probeset_type, probeset_type>::value ), "Incompatible node type" ) ;
                }

                iterator begin()
                {
                    return iterator(m_arrNode)   ;
                }
                iterator end()
                {
                    return iterator(m_arrNode + size()) ;
                }

                void insert_after( iterator it, node_type * p )
                {
                    assert( m_nSize < c_nCapacity ) ;
                    assert( !it.pArr || (m_arrNode <= it.pArr && it.pArr <= m_arrNode + m_nSize)) ;

                    if ( it.pArr ) {
                        shift_up( (unsigned int)(it.pArr - m_arrNode) + 1 ) ;
                        *(it.pArr + 1) = p ;
                    }
                    else {
                        shift_up(0)   ;
                        m_arrNode[0] = p ;
                    }
                    ++m_nSize ;
                }

                void remove( iterator /*itPrev*/, iterator itWhat )
                {
                    itWhat->clear() ;
                    shift_down( itWhat.pArr ) ;
                    --m_nSize ;
                }

                void clear()
                {
                    m_nSize = 0 ;
                }

                template <typename Disposer>
                void clear( Disposer disp )
                {
                    for ( unsigned int i = 0; i < m_nSize; ++i ) {
                        cds::unref(disp)( m_arrNode[i] ) ;
                    }
                    m_nSize = 0 ;
                }

                unsigned int size() const
                {
                    return m_nSize ;
                }
            };

            template <typename Node, unsigned int ArraySize>
            struct hash_ops {
                static void store( Node * pNode, size_t * pHashes )
                {
                    memcpy( pNode->m_arrHash, pHashes, sizeof(size_t) * ArraySize ) ;
                }
                static bool equal_to( Node& node, unsigned int nTable, size_t nHash )
                {
                    return node.m_arrHash[nTable] == nHash ;
                }
            };
            template <typename Node>
            struct hash_ops<Node, 0>
            {
                static void store( Node * /*pNode*/, size_t * /*pHashes*/ )
                {}
                static bool equal_to( Node& /*node*/, unsigned int /*nTable*/, size_t /*nHash*/ )
                {
                    return true ;
                }
            };

            template <typename NodeTraits, bool Ordered>
            struct contains ;

            template <typename NodeTraits>
            struct contains<NodeTraits, true>
            {
                template <typename BucketEntry, typename Position, typename Q, typename Compare>
                static bool find( BucketEntry& probeset, Position& pos, unsigned int nTable, size_t nHash, Q const& val, Compare cmp )
                {
                    // Ordered version
                    typedef typename BucketEntry::iterator bucket_iterator ;

                    bucket_iterator itPrev ;

                    for ( bucket_iterator it = probeset.begin(), itEnd = probeset.end(); it != itEnd; ++it ) {
                        int cmpRes = cmp( *NodeTraits::to_value_ptr(*it), val ) ;
                        if ( cmpRes >= 0 ) {
                            pos.itFound = it    ;
                            pos.itPrev = itPrev ;
                            return cmpRes == 0  ;
                        }

                        itPrev = it ;
                    }

                    pos.itPrev = itPrev ;
                    pos.itFound = probeset.end() ;
                    return false ;
                }
            };

            template <typename NodeTraits>
            struct contains<NodeTraits, false>
            {
                template <typename BucketEntry, typename Position, typename Q, typename EqualTo>
                static bool find( BucketEntry& probeset, Position& pos, unsigned int nTable, size_t nHash, Q const& val, EqualTo eq )
                {
                    // Unordered version
                    typedef typename BucketEntry::iterator  bucket_iterator ;
                    typedef typename BucketEntry::node_type node_type ;

                    bucket_iterator itPrev ;

                    for ( bucket_iterator it = probeset.begin(), itEnd = probeset.end(); it != itEnd; ++it ) {
                        if ( hash_ops<node_type, node_type::hash_array_size>::equal_to( *it, nTable, nHash ) && eq( *NodeTraits::to_value_ptr(*it), val )) {
                            pos.itFound = it ;
                            pos.itPrev = itPrev ;
                            return true ;
                        }
                        itPrev = it ;
                    }

                    pos.itPrev = itPrev ;
                    pos.itFound = probeset.end() ;
                    return false ;
                }
            };

        }   // namespace details
        //@endcond

    } // namespace cuckoo

    /// Cuckoo hash set
    /** @ingroup cds_intrusive_map

        Source
            - [2007] M.Herlihy, N.Shavit, M.Tzafrir "Concurrent Cuckoo Hashing. Technical report"
            - [2008] Maurice Herlihy, Nir Shavit "The Art of Multiprocessor Programming"

        <b>About Cuckoo hashing</b>

            [From <i>"The Art of Multiprocessor Programming"</i>]
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
            The probe set may be ordered or not. Ordered probe set can be more efficient since
            the average search complexity is <tt>O(PROBE_SET/2)</tt>.
            However, the overhead of sorting can eliminate a gain of ordered search.

            The probe set is ordered if opt::compare or opt::less is specified in \p Traits template
            parameter. Otherwise, the probe set is unordered and \p Traits must contain
            opt::equal_to option.

            The cds::intrusive::cuckoo namespace contains \p %CuckooSet-related declarations.

        Template arguments:
        - \p T - the type stored in the set.  The type must be based on cuckoo::node (for cuckoo::base_hook)
            or it must have a member of type %cuckoo::node (for cuckoo::member_hook),
            or it must be convertible to \p %cuckoo::node (for cuckoo::traits_hook)
        - \p Traits - type traits. See cuckoo::type_traits for explanation. It is possible to declare option-based
            set with cuckoo::make_traits metafunction result as \p Traits template argument.

        Template argument list \p Options... of cuckoo::make_traits metafunction are:
        - intrusive::opt::hook - hook used. Possible values are: cuckoo::base_hook, cuckoo::member_hook, cuckoo::traits_hook.
            If the option is not specified, <tt>%cuckoo::base_hook<></tt> is used.
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
        - opt::item_counter - the type of item counting feature. Default is \ref atomicity::item_counter
            The item counter should be atomic.
        - opt::allocator - the allocator type using for allocating bucket tables.
            Default is \p CDS_DEFAULT_ALLOCATOR
        - intrusive::opt::disposer - the disposer type used in \ref clear() member function for
            freeing nodes. Default is intrusive::opt::v::empty_disposer
        - opt::stat - internal statistics. Possibly types: cuckoo::stat, cuckoo::empty_stat.
            Default is cuckoo::empty_stat

        The probe set options cuckoo::probeset_type and cuckoo::store_hash are taken from \p node type
        specified by \p opt::hook option.

        <b>How to use</b>

        You should incorporate cuckoo::node into your struct \p T and provide
        appropriate cuckoo::type_traits::hook in your \p Traits template parameters. Usually, for \p Traits you
        define a struct based on cuckoo::type_traits.

        Example for base hook and list-based probe-set:
        \code
        #include <cds/intrusive/cuckoo_set.h>

        // Data stored in cuckoo set
        // We use list as probe-set container and store hash values in the node
        // (since we use two hash functions we should store 2 hash values per node)
        struct my_data: public cds::intrusive::cuckoo::node< cds::intrusive::cuckoo::list, 2 >
        {
            // key field
            std::string     strKey  ;

            // other data
            // ...
        }   ;

        // Provide equal_to functor for my_data since we will use unordered probe-set
        struct my_data_equal_to {
            bool operator()( const my_data& d1, const my_data& d2 ) const
            {
                return d1.strKey.compare( d2.strKey ) == 0  ;
            }

            bool operator()( const my_data& d, const std::string& s ) const
            {
                return d.strKey.compare(s) == 0  ;
            }

            bool operator()( const std::string& s, const my_data& d ) const
            {
                return s.compare( d.strKey ) == 0   ;
            }
        } ;

        // Provide two hash functor for my_data
        struct hash1 {
            size_t operator()(std::string const& s) const
            {
                return cds::opt::v::hash<std::string>( s ) ;
            }
            size_t operator()( my_data const& d ) const
            {
                return (*this)( d.strKey ) ;
            }
        };

        struct hash2: private hash1 {
            size_t operator()(std::string const& s) const
            {
                size_t h = ~( hash1::operator()(s))        ;
                return ~h + 0x9e3779b9 + (h << 6) + (h >> 2)    ;
            }
            size_t operator()( my_data const& d ) const
            {
                return (*this)( d.strKey ) ;
            }
        };

        // Declare type traits
        struct my_traits: public cds::intrusive::cuckoo::type_traits
        {
            typedef cds::intrusive::cuckoo::base_hook<
                cds::intrusive::cuckoo::probeset_type< my_data::probeset_type >
                ,cds::intrusive::cuckoo::store_hash< my_data::hash_array_size >
            >   hook    ;
            typedef my_data_equa_to equal_to ;
            typedef std::tuple< hash1, hash2 >  hash ;
        };

        // Declare CuckooSet type
        typedef cds::intrusive::CuckooSet< my_data, my_traits > my_cuckoo_set ;

        // Equal option-based declaration
        typedef cds::intrusive::CuckooSet< my_data,
            cds::intrusive::cuckoo::make_traits<
                cds::intrusive::opt::hook< cds::intrusive::cuckoo::base_hook<
                    cds::intrusive::cuckoo::probeset_type< my_data::probeset_type >
                    ,cds::intrusive::cuckoo::store_hash< my_data::hash_array_size >
                > >
                ,cds::opt::hash< std::tuple< hash1, hash2 > >
                ,cds::opt::equal_to< my_data_equal_to >
            >::type
        > opt_cuckoo_set ;
        \endcode

        If we provide \p compare function instead of \p equal_to for \p my_data
        we get as a result a cuckoo set with ordered probe set that may improve
        performance.
        Example for base hook and ordered vector-based probe-set:

        \code
        #include <cds/intrusive/cuckoo_set.h>

        // Data stored in cuckoo set
        // We use a vector of capacity 4 as probe-set container and store hash values in the node
        // (since we use two hash functions we should store 2 hash values per node)
        struct my_data: public cds::intrusive::cuckoo::node< cds::intrusive::cuckoo::vector<4>, 2 >
        {
            // key field
            std::string     strKey  ;

            // other data
            // ...
        }   ;

        // Provide compare functor for my_data since we want to use ordered probe-set
        struct my_data_compare {
            int operator()( const my_data& d1, const my_data& d2 ) const
            {
                return d1.strKey.compare( d2.strKey )  ;
            }

            int operator()( const my_data& d, const std::string& s ) const
            {
                return d.strKey.compare(s)  ;
            }

            int operator()( const std::string& s, const my_data& d ) const
            {
                return s.compare( d.strKey )    ;
            }
        } ;

        // Provide two hash functor for my_data
        struct hash1 {
            size_t operator()(std::string const& s) const
            {
                return cds::opt::v::hash<std::string>( s ) ;
            }
            size_t operator()( my_data const& d ) const
            {
                return (*this)( d.strKey ) ;
            }
        };

        struct hash2: private hash1 {
            size_t operator()(std::string const& s) const
            {
                size_t h = ~( hash1::operator()(s))        ;
                return ~h + 0x9e3779b9 + (h << 6) + (h >> 2)    ;
            }
            size_t operator()( my_data const& d ) const
            {
                return (*this)( d.strKey ) ;
            }
        };

        // Declare type traits
        struct my_traits: public cds::intrusive::cuckoo::type_traits
        {
            typedef cds::intrusive::cuckoo::base_hook<
                cds::intrusive::cuckoo::probeset_type< my_data::probeset_type >
                ,cds::intrusive::cuckoo::store_hash< my_data::hash_array_size >
            >   hook    ;
            typedef my_data_compare compare ;
            typedef std::tuple< hash1, hash2 >  hash ;
        };

        // Declare CuckooSet type
        typedef cds::intrusive::CuckooSet< my_data, my_traits > my_cuckoo_set ;

        // Equal option-based declaration
        typedef cds::intrusive::CuckooSet< my_data,
            cds::intrusive::cuckoo::make_traits<
                cds::intrusive::opt::hook< cds::intrusive::cuckoo::base_hook<
                    cds::intrusive::cuckoo::probeset_type< my_data::probeset_type >
                    ,cds::intrusive::cuckoo::store_hash< my_data::hash_array_size >
                > >
                ,cds::opt::hash< std::tuple< hash1, hash2 > >
                ,cds::opt::compare< my_data_compare >
            >::type
        > opt_cuckoo_set ;
        \endcode

    */
    template <typename T, typename Traits = cuckoo::type_traits>
    class CuckooSet
    {
    public:
        typedef T   value_type  ;   ///< The value type stored in the set
        typedef Traits options  ;   ///< Set traits

        typedef typename options::hook      hook        ;   ///< hook type
        typedef typename hook::node_type    node_type   ;   ///< node type
        typedef typename get_node_traits< value_type, node_type, hook>::type node_traits ;    ///< node traits

        typedef typename options::hash              hash ;   ///< hash functor tuple wrapped for internal use
        typedef typename hash::hash_tuple_type      hash_tuple_type ; ///< Type of hash tuple

        typedef typename options::stat              stat    ;   ///< internal statistics type

        typedef typename options::mutex_policy      original_mutex_policy   ;   ///< Concurrent access policy, see cuckoo::type_traits::mutex_policy

        /// Actual mutex policy
        /**
            Actual mutex policy is built from mutex policy type provided by \p Traits template argument (see cuckoo::type_traits::mutex_policy)
            but mutex policy internal statistics is conformed with cukoo::type_traits::stat type provided by \p Traits:
            - if \p %cuckoo::type_traits::stat is cuckoo::empty_stat then mutex policy statistics is already empty one
            - otherwise real mutex policy statistics is used
        */
        typedef typename original_mutex_policy::template rebind_statistics<
            typename std::conditional<
                std::is_same< stat, cuckoo::empty_stat >::value
                ,typename original_mutex_policy::empty_stat
                ,typename original_mutex_policy::real_stat
            >::type
        >::other    mutex_policy ;

        static bool const c_isSorted = !( std::is_same< typename options::compare, opt::none >::value
                && std::is_same< typename options::less, opt::none >::value ) ; ///< whether the probe set should be ordered
        static size_t const c_nArity = hash::size ; ///< the arity of cuckoo hashing: the number of hash functors provided; minimum 2.

        /// Key equality functor; used only for unordered probe-set
        typedef typename opt::details::make_equal_to< value_type, options, !c_isSorted>::type key_equal_to ;

        /// key comparing functor based on opt::compare and opt::less option setter. Used only for ordered probe set
        typedef typename opt::details::make_comparator< value_type, options >::type key_comparator  ;

        /// allocator type
        typedef typename options::allocator     allocator   ;

        /// item counter type
        typedef typename options::item_counter  item_counter ;

        /// node disposer
        typedef typename options::disposer      disposer    ;

    protected:
        //@cond
        typedef typename node_type::probeset_class  probeset_class  ;
        typedef typename node_type::probeset_type   probeset_type   ;
        static unsigned int const c_nNodeHashArraySize = node_type::hash_array_size ;

        typedef typename mutex_policy::scoped_cell_lock     scoped_cell_lock    ;
        typedef typename mutex_policy::scoped_cell_trylock  scoped_cell_trylock ;
        typedef typename mutex_policy::scoped_full_lock     scoped_full_lock    ;
        typedef typename mutex_policy::scoped_resize_lock   scoped_resize_lock  ;

        typedef cuckoo::details::bucket_entry< node_type, probeset_type >   bucket_entry    ;
        typedef typename bucket_entry::iterator                     bucket_iterator ;
        typedef cds::details::Allocator< bucket_entry, allocator >  bucket_table_allocator ;

        typedef size_t  hash_array[c_nArity]    ;   ///< hash array

#   ifndef CDS_CXX11_LAMBDA_SUPPORT
        struct empty_insert_functor {
            void operator()( value_type& )
            {}
        };

        struct empty_erase_functor  {
            void operator()( value_type const& )
            {}
        };

        struct empty_find_functor {
            template <typename Q>
            void operator()( value_type& item, Q& val )
            {}
        };
#   endif

#   if !defined(CDS_CXX11_LAMBDA_SUPPORT) || ((CDS_COMPILER == CDS_COMPILER_MSVC || CDS_COMPILER == CDS_COMPILER_INTEL ) && _MSC_VER == 1600)
        template <typename Disposer>
        class disposer_wrapper: protected cds::details::functor_wrapper<Disposer>
        {
            typedef cds::details::functor_wrapper<Disposer> base_class ;
        public:
            disposer_wrapper( Disposer d): base_class(d) {}

            void operator()( node_type * pNode )
            {
                base_class::get()( node_traits::to_value_ptr( pNode )) ;
            }
        };
#   endif

        struct position {
            bucket_iterator     itPrev ;
            bucket_iterator     itFound;
        };

        typedef typename std::conditional< c_isSorted
            , cuckoo::details::contains< node_traits, true >
            , cuckoo::details::contains< node_traits, false >
        >::type contains_action ;

        template <typename Predicate>
        struct predicate_wrapper {
            typedef typename std::conditional< c_isSorted, cds::opt::details::make_comparator_from_less<Predicate>, Predicate>::type   type ;
        };

        typedef typename std::conditional< c_isSorted, key_comparator, key_equal_to >::type key_predicate ;
        //@endcond

    public:
        static unsigned int const   c_nDefaultProbesetSize = 4  ;   ///< default probeset size
        static size_t const         c_nDefaultInitialSize = 16  ;   ///< default initial size
        static unsigned int const   c_nRelocateLimit = c_nArity * 2 - 1 ;   ///< Count of attempts to relocate before giving up

    protected:
        bucket_entry *      m_BucketTable[ c_nArity ] ; ///< Bucket tables

        size_t              m_nBucketMask           ;   ///< Hash bitmask; bucket table size minus 1.
        unsigned int const  m_nProbesetSize         ;   ///< Probe set size
        unsigned int const  m_nProbesetThreshold    ;   ///< Probe set threshold

        hash            m_Hash              ;   ///< Hash functor tuple
        mutex_policy    m_MutexPolicy       ;   ///< concurrent access policy
        item_counter    m_ItemCounter       ;   ///< item counter
        mutable stat    m_Stat              ;   ///< internal statistics

    protected:
        //@cond
        static void check_common_constraints()
        {
            static_assert( (c_nArity == mutex_policy::c_nArity), "The count of hash functors must be equal to mutex_policy arity" ) ;
        }

        void check_probeset_properties() const
        {
            assert( m_nProbesetThreshold < m_nProbesetSize ) ;

            // if probe set type is cuckoo::vector<N> then m_nProbesetSize == N
            assert( node_type::probeset_size == 0 || node_type::probeset_size == m_nProbesetSize ) ;
        }

        template <typename Q>
        void hashing( size_t * pHashes, Q const& v ) const
        {
            m_Hash( pHashes, v ) ;
        }

        void copy_hash( size_t * pHashes, value_type const& v ) const
        {
            if ( c_nNodeHashArraySize )
                memcpy( pHashes, node_traits::to_node_ptr( v )->get_hash(), sizeof(pHashes[0]) * c_nNodeHashArraySize ) ;
            else
                hashing( pHashes, v ) ;
        }

        bucket_entry& bucket( unsigned int nTable, size_t nHash )
        {
            assert( nTable < c_nArity ) ;
            return m_BucketTable[nTable][nHash & m_nBucketMask] ;
        }

        static void store_hash( node_type * pNode, size_t * pHashes )
        {
            cuckoo::details::hash_ops< node_type, c_nNodeHashArraySize >::store( pNode, pHashes ) ;
            //memcpy( pNode->m_arrHash, pHashes, sizeof(size_t) * c_nArity ) ;
        }

        static bool equal_hash( node_type& node, unsigned int nTable, size_t nHash )
        {
            return cuckoo::details::hash_ops< node_type, c_nNodeHashArraySize >::equal_to( node, nTable, nHash ) ;
        }

        void allocate_bucket_tables( size_t nSize )
        {
            assert( cds::beans::is_power2( nSize ) ) ;

            m_nBucketMask = nSize - 1 ;
            bucket_table_allocator alloc ;
            for ( unsigned int i = 0; i < c_nArity; ++i )
                m_BucketTable[i] = alloc.NewArray( nSize )  ;
        }

        static void free_bucket_tables( bucket_entry ** pTable, size_t nCapacity )
        {
            bucket_table_allocator alloc ;
            for ( unsigned int i = 0; i < c_nArity; ++i ) {
                alloc.Delete( pTable[i], nCapacity ) ;
                pTable[i] = null_ptr<bucket_entry *>() ;
            }
        }
        void free_bucket_tables()
        {
            free_bucket_tables( m_BucketTable, m_nBucketMask + 1 ) ;
        }

        static unsigned int const c_nUndefTable = (unsigned int) -1 ;
        template <typename Q, typename Predicate >
        unsigned int contains( position * arrPos, size_t * arrHash, Q const& val, Predicate pred )
        {
            // Buckets must be locked

            for ( unsigned int i = 0; i < c_nArity; ++i ) {
                bucket_entry& probeset = bucket( i, arrHash[i] ) ;
                if ( contains_action::find( probeset, arrPos[i], i, arrHash[i], val, pred ))
                    return i ;
            }
            return c_nUndefTable ;
        }

        template <typename Q, typename Predicate, typename Func>
        value_type * erase_( Q const& val, Predicate pred, Func f )
        {
            hash_array arrHash ;
            hashing( arrHash, val )     ;
            position arrPos[ c_nArity ] ;

            {
                scoped_cell_lock guard( m_MutexPolicy, arrHash ) ;

                unsigned int nTable = contains( arrPos, arrHash, val, pred ) ;
                if ( nTable != c_nUndefTable ) {
                    node_type& node = *arrPos[nTable].itFound ;
                    cds::unref(f)( *node_traits::to_value_ptr(node) ) ;
                    bucket( nTable, arrHash[nTable]).remove( arrPos[nTable].itPrev, arrPos[nTable].itFound ) ;
                    --m_ItemCounter ;
                    m_Stat.onEraseSuccess() ;
                    return node_traits::to_value_ptr( node )    ;
                }
            }

            m_Stat.onEraseFailed() ;
            return null_ptr<value_type *>() ;
        }

        template <typename Q, typename Predicate, typename Func>
        bool find_( Q& val, Predicate pred, Func f )
        {
            hash_array arrHash ;
            position arrPos[ c_nArity ] ;
            hashing( arrHash, val ) ;
            scoped_cell_lock sl( m_MutexPolicy, arrHash ) ;

            unsigned int nTable = contains( arrPos, arrHash, val, pred ) ;
            if ( nTable != c_nUndefTable ) {
                cds::unref(f)( *node_traits::to_value_ptr( *arrPos[nTable].itFound ), val ) ;
                m_Stat.onFindSuccess() ;
                return true ;
            }

            m_Stat.onFindFailed() ;
            return false ;
        }

        bool relocate( unsigned int nTable, size_t * arrGoalHash )
        {
            // arrGoalHash contains hash values for relocating element
            // Relocating element is first one from bucket( nTable, arrGoalHash[nTable] ) probeset

            m_Stat.onRelocateCall() ;

            hash_array arrHash  ;
            value_type * pVal   ;
            for ( unsigned int nRound = 0; nRound < c_nRelocateLimit; ++nRound ) {
                m_Stat.onRelocateRound() ;

                while ( true ) {
                    scoped_cell_lock guard( m_MutexPolicy, arrGoalHash ) ;

                    bucket_entry& refBucket = bucket( nTable, arrGoalHash[nTable] ) ;
                    if ( refBucket.size() < m_nProbesetThreshold ) {
                        // probeset is not above the threshold
                        m_Stat.onFalseRelocateRound() ;
                        return true ;
                    }

                    pVal = node_traits::to_value_ptr( *refBucket.begin() ) ;
                    copy_hash( arrHash, *pVal ) ;

                    scoped_cell_trylock guard2( m_MutexPolicy, arrHash ) ;
                    if ( !guard2.locked() )
                        continue ;  // try one more time

                    refBucket.remove( typename bucket_entry::iterator(), refBucket.begin() ) ;

                    unsigned int i = (nTable + 1) % c_nArity ;

                    // try insert into free probeset
                    while ( i != nTable ) {
                        bucket_entry& bkt = bucket( i, arrHash[i] ) ;
                        if ( bkt.size() < m_nProbesetThreshold ) {
                            position pos ;
                            contains_action::find( bkt, pos, i, arrHash[i], *pVal, key_predicate() ) ; // must return false!
                            bkt.insert_after( pos.itPrev, node_traits::to_node_ptr( pVal )) ;
                            m_Stat.onSuccessRelocateRound() ;
                            return true     ;
                        }
                        i = ( i + 1 ) % c_nArity ;
                    }

                    // try insert into partial probeset
                    i = (nTable + 1) % c_nArity ;
                    while ( i != nTable ) {
                        bucket_entry& bkt = bucket( i, arrHash[i] ) ;
                        if ( bkt.size() < m_nProbesetSize ) {
                            position pos ;
                            contains_action::find( bkt, pos, i, arrHash[i], *pVal, key_predicate() ) ; // must return false!
                            bkt.insert_after( pos.itPrev, node_traits::to_node_ptr( pVal )) ;
                            nTable = i  ;
                            memcpy( arrGoalHash, arrHash, sizeof(arrHash)) ;
                            m_Stat.onRelocateAboveThresholdRound() ;
                            goto next_iteration ;
                        }
                        i = (i + 1) % c_nArity ;
                    }

                    // all probeset is full, relocating fault
                    refBucket.insert_after( typename bucket_entry::iterator(), node_traits::to_node_ptr( pVal )) ;
                    m_Stat.onFailedRelocate() ;
                    return false ;
                }

            next_iteration: ;
            }
            return false ;
        }

        void resize()
        {
            m_Stat.onResizeCall() ;

            size_t nOldCapacity = bucket_count()        ;
            bucket_entry *      pOldTable[ c_nArity ]   ;
            {
                scoped_resize_lock guard( m_MutexPolicy )   ;

                if ( nOldCapacity != bucket_count() ) {
                    m_Stat.onFalseResizeCall() ;
                    return ;
                }

                size_t nCapacity = nOldCapacity * 2         ;

                m_MutexPolicy.resize( nCapacity )   ;
                memcpy( pOldTable, m_BucketTable, sizeof(pOldTable)) ;
                allocate_bucket_tables( nCapacity )         ;

                typedef typename bucket_entry::iterator bucket_iterator ;
                hash_array arrHash ;
                position arrPos[ c_nArity ] ;

                for ( unsigned int nTable = 0; nTable < c_nArity; ++nTable ) {
                    bucket_entry * pTable = pOldTable[nTable]   ;
                    for ( size_t k = 0; k < nOldCapacity; ++k ) {
                        bucket_iterator itNext ;
                        for ( bucket_iterator it = pTable[k].begin(), itEnd = pTable[k].end(); it != itEnd; it = itNext ) {
                            itNext = it ;
                            ++itNext    ;

                            value_type& val = *node_traits::to_value_ptr( *it ) ;
                            copy_hash( arrHash, val ) ;
                            contains( arrPos, arrHash, val, key_predicate() ) ; // must return c_nUndefTable

                            for ( unsigned int i = 0; i < c_nArity; ++i ) {
                                bucket_entry& refBucket = bucket( i, arrHash[i] ) ;
                                if ( refBucket.size() < m_nProbesetThreshold ) {
                                    refBucket.insert_after( arrPos[i].itPrev, &*it ) ;
                                    m_Stat.onResizeSuccessMove() ;
                                    goto do_next ;
                                }
                            }

                            for ( unsigned int i = 0; i < c_nArity; ++i ) {
                                bucket_entry& refBucket = bucket( i, arrHash[i] ) ;
                                if ( refBucket.size() < m_nProbesetSize ) {
                                    refBucket.insert_after( arrPos[i].itPrev, &*it ) ;
                                    assert( refBucket.size() > 1 )  ;
                                    copy_hash( arrHash, *node_traits::to_value_ptr( *refBucket.begin()) ) ;
                                    m_Stat.onResizeRelocateCall() ;
                                    relocate( i, arrHash )  ;
                                    break ;
                                }
                            }
                        do_next: ;
                        }
                    }
                }
            }
            free_bucket_tables( pOldTable, nOldCapacity ) ;
        }

        CDS_CONSTEXPR static unsigned int calc_probeset_size( unsigned int nProbesetSize ) CDS_NOEXCEPT
        {
            return nProbesetSize
                ? nProbesetSize
                : ( node_type::probeset_size ? node_type::probeset_size : c_nDefaultProbesetSize )
                ;
        }
        //@endcond

    public:
        /// Default constructor
        /**
            Initial size = \ref c_nDefaultInitialSize

            Probe set size:
            - \p c_nDefaultProbesetSize if \p probeset_type is \p cuckoo::list
            - \p Capacity if \p probeset_type is <tt> cuckoo::vector<Capacity> </tt>

            Probe set threshold = probe set size - 1
        */
        CuckooSet()
            : m_nProbesetSize( calc_probeset_size(0) )
            , m_nProbesetThreshold( m_nProbesetSize - 1 )
            , m_MutexPolicy( c_nDefaultInitialSize )
        {
            check_common_constraints() ;
            check_probeset_properties();

            allocate_bucket_tables( c_nDefaultInitialSize ) ;
        }

        /// Constructs the set object with given probe set size and threshold
        /**
            If probe set type is <tt> cuckoo::vector<Capacity> </tt> vector
            then \p nProbesetSize should be equal to vector's \p Capacity.
        */
        CuckooSet(
            size_t nInitialSize                 ///< Initial set size; if 0 - use default initial size \ref c_nDefaultInitialSize
            , unsigned int nProbesetSize        ///< probe set size
            , unsigned int nProbesetThreshold = 0   ///< probe set threshold, <tt>nProbesetThreshold < nProbesetSize</tt>. If 0, nProbesetThreshold = nProbesetSize - 1
        )
            : m_nProbesetSize( calc_probeset_size(nProbesetSize) )
            , m_nProbesetThreshold( nProbesetThreshold ? nProbesetThreshold : m_nProbesetSize - 1 )
            , m_MutexPolicy( cds::beans::ceil2(nInitialSize ? nInitialSize : c_nDefaultInitialSize ))
        {
            check_common_constraints()  ;
            check_probeset_properties() ;

            allocate_bucket_tables( nInitialSize ? cds::beans::ceil2( nInitialSize ) : c_nDefaultInitialSize ) ;
        }

        /// Constructs the set object with given hash functor tuple
        /**
            The probe set size and threshold are set as default, see CuckooSet()
        */
        CuckooSet(
            hash_tuple_type const& h    ///< hash functor tuple of type <tt>std::tuple<H1, H2, ... Hn></tt> where <tt> n == \ref c_nArity </tt>
        )
            : m_nProbesetSize( calc_probeset_size(0) )
            , m_nProbesetThreshold( m_nProbesetSize -1 )
            , m_Hash( h )
            , m_MutexPolicy( c_nDefaultInitialSize )
        {
            check_common_constraints()  ;
            check_probeset_properties() ;

            allocate_bucket_tables( c_nDefaultInitialSize ) ;
        }

        /// Constructs the set object with given probe set properties and hash functor tuple
        /**
            If probe set type is <tt> cuckoo::vector<Capacity> </tt> vector
            then \p nProbesetSize should be equal to vector's \p Capacity.
        */
        CuckooSet(
            size_t nInitialSize                 ///< Initial set size; if 0 - use default initial size \ref c_nDefaultInitialSize
            , unsigned int nProbesetSize        ///< probe set size, positive integer
            , unsigned int nProbesetThreshold   ///< probe set threshold, <tt>nProbesetThreshold < nProbesetSize</tt>. If 0, nProbesetThreshold = nProbesetSize - 1
            , hash_tuple_type const& h    ///< hash functor tuple of type <tt>std::tuple<H1, H2, ... Hn></tt> where <tt> n == \ref c_nArity </tt>
        )
            : m_nProbesetSize( calc_probeset_size(nProbesetSize) )
            , m_nProbesetThreshold( nProbesetThreshold ? nProbesetThreshold : m_nProbesetSize - 1)
            , m_Hash( h )
            , m_MutexPolicy( cds::beans::ceil2(nInitialSize ? nInitialSize : c_nDefaultInitialSize ))
        {
            check_common_constraints()  ;
            check_probeset_properties() ;

            allocate_bucket_tables( nInitialSize ? cds::beans::ceil2( nInitialSize ) : c_nDefaultInitialSize ) ;
        }

#   ifdef CDS_MOVE_SEMANTICS_SUPPORT
        /// Constructs the set object with given hash functor tuple (move semantics)
        /**
            The probe set size and threshold are set as default, see CuckooSet()
        */
        CuckooSet(
            hash_tuple_type&& h     ///< hash functor tuple of type <tt>std::tuple<H1, H2, ... Hn></tt> where <tt> n == \ref c_nArity </tt>
        )
            : m_nProbesetSize( calc_probeset_size(0) )
            , m_nProbesetThreshold( m_nProbesetSize / 2 )
            , m_Hash( std::forward<hash_tuple_type>(h) )
            , m_MutexPolicy( c_nDefaultInitialSize )
        {
            check_common_constraints()  ;
            check_probeset_properties() ;

            allocate_bucket_tables( c_nDefaultInitialSize ) ;
        }

        /// Constructs the set object with given probe set properties and hash functor tuple (move semantics)
        /**
            If probe set type is <tt> cuckoo::vector<Capacity> </tt> vector
            then \p nProbesetSize should be equal to vector's \p Capacity.
        */
        CuckooSet(
            size_t nInitialSize                 ///< Initial set size; if 0 - use default initial size \ref c_nDefaultInitialSize
            , unsigned int nProbesetSize        ///< probe set size, positive integer
            , unsigned int nProbesetThreshold   ///< probe set threshold, <tt>nProbesetThreshold < nProbesetSize</tt>. If 0, nProbesetThreshold = nProbesetSize - 1
            , hash_tuple_type&& h    ///< hash functor tuple of type <tt>std::tuple<H1, H2, ... Hn></tt> where <tt> n == \ref c_nArity </tt>
        )
            : m_nProbesetSize( calc_probeset_size(nProbesetSize) )
            , m_nProbesetThreshold( nProbesetThreshold ? nProbesetThreshold : m_nProbesetSize - 1)
            , m_Hash( std::forward<hash_tuple_type>(h) )
            , m_MutexPolicy( cds::beans::ceil2(nInitialSize ? nInitialSize : c_nDefaultInitialSize ))
        {
            check_common_constraints()  ;
            check_probeset_properties() ;

            allocate_bucket_tables( nInitialSize ? cds::beans::ceil2( nInitialSize ) : c_nDefaultInitialSize ) ;
        }
#   endif   // ifdef CDS_MOVE_SEMANTICS_SUPPORT

        /// Destructor
        ~CuckooSet()
        {
            free_bucket_tables()    ;
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
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return insert( val, []( value_type& ) {} )    ;
#       else
            return insert( val, empty_insert_functor() )    ;
#       endif
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
            where \p val is the item inserted.

            The user-defined functor is called only if the inserting is success and can be passed by reference
            using <tt>boost::ref</tt>
        */
        template <typename Func>
        bool insert( value_type& val, Func f )
        {
            hash_array arrHash ;
            position arrPos[ c_nArity ] ;
            unsigned int nGoalTable ;

            hashing( arrHash, val ) ;
            node_type * pNode = node_traits::to_node_ptr( val ) ;
            store_hash( pNode, arrHash ) ;

            while (true) {
                {
                    scoped_cell_lock guard( m_MutexPolicy, arrHash ) ;

                    if ( contains( arrPos, arrHash, val, key_predicate() ) != c_nUndefTable ) {
                        m_Stat.onInsertFailed() ;
                        return false ;
                    }

                    for ( unsigned int i = 0; i < c_nArity; ++i ) {
                        bucket_entry& refBucket = bucket( i, arrHash[i] ) ;
                        if ( refBucket.size() < m_nProbesetThreshold ) {
                            refBucket.insert_after( arrPos[i].itPrev, pNode ) ;
                            cds::unref(f)( val )    ;
                            ++m_ItemCounter         ;
                            m_Stat.onInsertSuccess();
                            return true ;
                        }
                    }

                    for ( unsigned int i = 0; i < c_nArity; ++i ) {
                        bucket_entry& refBucket = bucket( i, arrHash[i] ) ;
                        if ( refBucket.size() < m_nProbesetSize ) {
                            refBucket.insert_after( arrPos[i].itPrev, pNode ) ;
                            cds::unref(f)( val );
                            ++m_ItemCounter     ;
                            nGoalTable = i      ;
                            assert( refBucket.size() > 1 )  ;
                            copy_hash( arrHash, *node_traits::to_value_ptr( *refBucket.begin()) ) ;
                            goto do_relocate    ;
                        }
                    }
                }

                m_Stat.onInsertResize() ;
                resize()    ;
            }

        do_relocate:
            m_Stat.onInsertRelocate();
            if ( !relocate( nGoalTable, arrHash )) {
                m_Stat.onInsertRelocateFault() ;
                m_Stat.onInsertResize() ;
                resize()    ;
            }

            m_Stat.onInsertSuccess();
            return true     ;
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

            The functor may change non-key fields of the \p item.

            You may pass \p func argument by reference using <tt>boost::ref</tt> or cds::ref.

            Returns <tt> std::pair<bool, bool> </tt> where \p first is \p true if operation is successful,
            \p second is \p true if new item has been added or \p false if the item with \p key
            already is in the set.
        */
        template <typename Func>
        std::pair<bool, bool> ensure( value_type& val, Func func )
        {
            hash_array arrHash ;
            position arrPos[ c_nArity ] ;
            unsigned int nGoalTable ;

            hashing( arrHash, val ) ;
            node_type * pNode = node_traits::to_node_ptr( val ) ;
            store_hash( pNode, arrHash ) ;

            while (true) {
                {
                    scoped_cell_lock guard( m_MutexPolicy, arrHash ) ;

                    unsigned int nTable = contains( arrPos, arrHash, val, key_predicate() ) ;
                    if ( nTable != c_nUndefTable ) {
                        cds::unref(func)( false, *node_traits::to_value_ptr( *arrPos[nTable].itFound ), val ) ;
                        m_Stat.onEnsureExist() ;
                        return std::make_pair( true, false ) ;
                    }

                    node_type * pNode = node_traits::to_node_ptr( val ) ;
                    store_hash( pNode, arrHash ) ;

                    for ( unsigned int i = 0; i < c_nArity; ++i ) {
                        bucket_entry& refBucket = bucket( i, arrHash[i] ) ;
                        if ( refBucket.size() < m_nProbesetThreshold ) {
                            refBucket.insert_after( arrPos[i].itPrev, pNode ) ;
                            cds::unref(func)( true, val, val ) ;
                            ++m_ItemCounter ;
                            m_Stat.onEnsureSuccess() ;
                            return std::make_pair( true, true ) ;
                        }
                    }

                    for ( unsigned int i = 0; i < c_nArity; ++i ) {
                        bucket_entry& refBucket = bucket( i, arrHash[i] ) ;
                        if ( refBucket.size() < m_nProbesetSize ) {
                            refBucket.insert_after( arrPos[i].itPrev, pNode ) ;
                            cds::unref(func)( true, val, val ) ;
                            ++m_ItemCounter     ;
                            nGoalTable = i      ;
                            assert( refBucket.size() > 1 )  ;
                            copy_hash( arrHash, *node_traits::to_value_ptr( *refBucket.begin()) ) ;
                            goto do_relocate    ;
                        }
                    }
                }

                m_Stat.onEnsureResize() ;
                resize()    ;
            }

        do_relocate:
            m_Stat.onEnsureRelocate() ;
            if ( !relocate( nGoalTable, arrHash )) {
                m_Stat.onEnsureRelocateFault() ;
                m_Stat.onEnsureResize() ;
                resize()    ;
            }

            m_Stat.onEnsureSuccess() ;
            return std::make_pair( true, true ) ;
        }

        /// Unlink the item \p val from the set
        /**
            The function searches the item \p val in the set and unlink it
            if it is found and is equal to \p val (here, the equality means that
            \p val belongs to the set: if \p item is an item found then
            unlink is successful iif <tt>&val == &item</tt>)

            The function returns \p true if success and \p false otherwise.
        */
        bool unlink( value_type& val )
        {
            hash_array arrHash ;
            hashing( arrHash, val )     ;
            position arrPos[ c_nArity ] ;

            {
                scoped_cell_lock guard( m_MutexPolicy, arrHash ) ;

                unsigned int nTable = contains( arrPos, arrHash, val, key_predicate() ) ;
                if ( nTable != c_nUndefTable && node_traits::to_value_ptr(*arrPos[nTable].itFound) == &val ) {
                    bucket( nTable, arrHash[nTable]).remove( arrPos[nTable].itPrev, arrPos[nTable].itFound ) ;
                    --m_ItemCounter ;
                    m_Stat.onUnlinkSuccess()    ;
                    return true ;
                }
            }

            m_Stat.onUnlinkFailed() ;
            return false ;
        }

        /// Deletes the item from the set
        /** \anchor cds_intrusive_CuckooSet_erase
            The function searches an item with key equal to \p val in the set,
            unlinks it from the set, and returns a pointer to unlinked item.

            If the item with key equal to \p val is not found the function return \p NULL.

            Note the hash functor should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q>
        value_type * erase( Q const& val )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return erase( val, [](value_type const&) {} )  ;
#       else
            return erase( val, empty_erase_functor() )  ;
#       endif
        }

        /// Deletes the item from the set using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_intrusive_CuckooSet_erase "erase(Q const&)"
            but \p pred is used for key comparing.
            If cuckoo set is ordered, then \p Predicate should have the interface and semantics like \p std::less.
            If cuckoo set is unordered, then \p Predicate should have the interface and semantics like \p std::equal_to.
            \p Predicate must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Predicate>
        value_type * erase_with( Q const& val, Predicate pred )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return erase_( val, typename predicate_wrapper<Predicate>::type(), [](value_type const&) {} )  ;
#       else
            return erase_( val, typename predicate_wrapper<Predicate>::type(), empty_erase_functor() )  ;
#       endif
        }

        /// Delete the item from the set
        /** \anchor cds_intrusive_CuckooSet_erase_func
            The function searches an item with key equal to \p val in the set,
            call \p f functor with item found, unlinks it from the set, and returns a pointer to unlinked item.

            The \p Func interface is
            \code
            struct functor {
                void operator()( value_type const& item ) ;
            } ;
            \endcode
            The functor may be passed by reference with <tt>boost:ref</tt>

            If the item with key equal to \p val is not found the function return \p NULL.

            Note the hash functor should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q, typename Func>
        value_type * erase( Q const& val, Func f )
        {
            return erase_( val, key_predicate(), f ) ;
        }

        /// Deletes the item from the set using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_intrusive_CuckooSet_erase_func "erase(Q const&, Func)"
            but \p pred is used for key comparing.
            If you use ordered cuckoo set, then \p Predicate should have the interface and semantics like \p std::less.
            If you use unordered cuckoo set, then \p Predicate should have the interface and semantics like \p std::equal_to.
            \p Predicate must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Predicate, typename Func>
        value_type * erase_with( Q const& val, Predicate pred, Func f )
        {
            return erase_( val, typename predicate_wrapper<Predicate>::type(), f )  ;
        }

        /// Find the key \p val
        /** \anchor cds_intrusive_CuckooSet_find_func
            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You can pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change non-key fields of \p item.

            The \p val argument is non-const since it can be used as \p f functor destination i.e., the functor
            may modify both arguments.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q& val, Func f )
        {
            return find_( val, key_predicate(), f ) ;
        }

        /// Find the key \p val using \p pred predicate for comparing
        /**
            The function is an analog of \ref cds_intrusive_CuckooSet_find_func "find(Q&, Func)"
            but \p pred is used for key comparison.
            If you use ordered cuckoo set, then \p Predicate should have the interface and semantics like \p std::less.
            If you use unordered cuckoo set, then \p Predicate should have the interface and semantics like \p std::equal_to.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Predicate, typename Func>
        bool find_with( Q& val, Predicate pred, Func f )
        {
            return find_( val, typename predicate_wrapper<Predicate>::type(), f ) ;
        }

        /// Find the key \p val
        /** \anchor cds_intrusive_CuckooSet_find_cfunc
            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q const& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You can pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change non-key fields of \p item.

            The \p val argument is non-const since it can be used as \p f functor destination i.e., the functor
            may modify both arguments.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q const& val, Func f )
        {
            return find_( val, key_predicate(), f ) ;
        }

        /// Find the key \p val using \p pred predicate for comparing
        /**
            The function is an analog of \ref cds_intrusive_CuckooSet_find_cfunc "find(Q const&, Func)"
            but \p pred is used for key comparison.
            If you use ordered cuckoo set, then \p Predicate should have the interface and semantics like \p std::less.
            If you use unordered cuckoo set, then \p Predicate should have the interface and semantics like \p std::equal_to.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Predicate, typename Func>
        bool find_with( Q const& val, Predicate pred, Func f )
        {
            return find_( val, typename predicate_wrapper<Predicate>::type(), f ) ;
        }

        /// Find the key \p val
        /** \anchor cds_intrusive_CuckooSet_find_val
            The function searches the item with key equal to \p val
            and returns \p true if it is found, and \p false otherwise.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q>
        bool find( Q const& val )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return find( val, [](value_type&, Q const& ) {} )    ;
#       else
            return find( val, empty_find_functor() )    ;
#       endif
        }

        /// Find the key \p val using \p pred predicate for comparing
        /**
            The function is an analog of \ref cds_intrusive_CuckooSet_find_val "find(Q const&)"
            but \p pred is used for key comparison.
            If you use ordered cuckoo set, then \p Predicate should have the interface and semantics like \p std::less.
            If you use unordered cuckoo set, then \p Predicate should have the interface and semantics like \p std::equal_to.
            \p pred must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Predicate>
        bool find_with( Q const& val, Predicate pred )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return find_with( val, typename predicate_wrapper<Predicate>::type(), [](value_type& , Q const& ) {} )    ;
#       else
            return find_with( val, typename predicate_wrapper<Predicate>::type(), empty_find_functor() )    ;
#       endif
        }

        /// Clears the set
        /**
            The function unlinks all items from the set.
            For any item \ref disposer is called
        */
        void clear()
        {
            clear_and_dispose( disposer() ) ;
        }

        /// Clears the set and calls \p disposer for each item
        /**
            The function unlinks all items from the set calling \p disposer for each item.
            \p Disposer functor interface is:
            \code
            struct Disposer{
                void operator()( value_type * p );
            };
            \endcode

            The \ref disposer specified in \p Traits options is not called.
        */
        template <typename Disposer>
        void clear_and_dispose( Disposer oDisposer )
        {
            // locks entire array
            scoped_full_lock sl( m_MutexPolicy )    ;

#   if !defined(CDS_CXX11_LAMBDA_SUPPORT) || ((CDS_COMPILER == CDS_COMPILER_MSVC || CDS_COMPILER == CDS_COMPILER_INTEL ) && _MSC_VER == 1600)
            disposer_wrapper<Disposer> disp( oDisposer )   ;
#       endif
            for ( unsigned int i = 0; i < c_nArity; ++i ) {
                bucket_entry * pEntry = m_BucketTable[i] ;
                bucket_entry * pEnd = pEntry + m_nBucketMask + 1 ;
                for ( ; pEntry != pEnd ; ++pEntry ) {
#       if defined(CDS_CXX11_LAMBDA_SUPPORT) && !((CDS_COMPILER == CDS_COMPILER_MSVC || CDS_COMPILER == CDS_COMPILER_INTEL) && _MSC_VER == 1600)
                    // MSVC 10: error to call nested typedefs node_traits from lambda
                    pEntry->clear( [&oDisposer]( node_type * pNode ){ oDisposer( node_traits::to_value_ptr( pNode )) ; } ) ;
#       else
                    pEntry->clear( cds::ref(disp) ) ;
#       endif
                }
            }
            m_ItemCounter.reset() ;
        }

        /// Checks if the set is empty
        /**
            Emptiness is checked by item counting: if item count is zero then the set is empty.
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
            The hash table size is non-constant and can be increased via resizing.
        */
        size_t bucket_count() const
        {
            return m_nBucketMask + 1    ;
        }

        /// Returns lock array size
        size_t lock_count() const
        {
            return m_MutexPolicy.lock_count() ;
        }

        /// Returns const reference to internal statistics
        stat const& statistics() const
        {
            return m_Stat ;
        }

        /// Returns const reference to mutex policy internal statistics
        typename mutex_policy::statistics_type const& mutex_policy_statistics() const
        {
            return m_MutexPolicy.statistics() ;
        }
    };
}} // namespace cds::intrusive

#endif // #ifndef __CDS_INTRUSIVE_CUCKOO_SET_H
