//$$CDS-header$$

#ifndef __CDS_GC_HZP_HZP_H
#define __CDS_GC_HZP_HZP_H

#include <cds/cxx11_atomic.h>
#include <cds/os/thread.h>
#include <cds/gc/exception.h>

#include <cds/gc/hzp/details/hp_fwd.h>
#include <cds/gc/hzp/details/hp_alloc.h>
#include <cds/gc/hzp/details/hp_retired.h>

#include <vector>
#include <cds/details/noncopyable.h>

#if CDS_COMPILER == CDS_COMPILER_MSVC
#   pragma warning(push)
    // warning C4251: 'cds::gc::hzp::GarbageCollector::m_pListHead' : class 'cds::cxx11_atomics::atomic<T>'
    // needs to have dll-interface to be used by clients of class 'cds::gc::hzp::GarbageCollector'
#   pragma warning(disable: 4251)
#endif

/*
    Editions:
        2007.12.24  khizmax Add statistics and CDS_GATHER_HAZARDPTR_STAT macro
        2008.03.06  khizmax Refactoring: implementation of HazardPtrMgr is moved to hazardptr.cpp
        2008.03.08  khizmax Remove HazardPtrMgr singleton. Now you must initialize/destroy HazardPtrMgr calling
                            HazardPtrMgr::Construct / HazardPtrMgr::Destruct before use (usually in main() function).
        2008.12.06  khizmax Refactoring. Changes class name, namespace hierarchy, all helper defs have been moved to details namespace
        2010.01.27  khizmax Introducing memory order constraint
*/

namespace cds {
    /**
        @page cds_garbage_collectors_comparison GC comparison
        @ingroup cds_garbage_collector

        <table>
            <tr>
                <th>Feature</th>
                <th>%cds::gc::HP</th>
                <th>%cds::gc::HRC</th>
                <th>%cds::gc::PTB</th>
            </tr>
            <tr>
                <td>Implementation quality</td>
                <td>stable</td>
                <td>unstable</td>
                <td>stable</td>
            </tr>
            <tr>
                <td>Performance rank (1 - slowest, 5 - fastest)</td>
                <td>5</td>
                <td>1</td>
                <td>4</td>
            </tr>
            <tr>
                <td>Max number of guarded (hazard) pointers per thread</td>
                <td>limited (specifies in GC object ctor)</td>
                <td>limited (specifies in GC object ctor)</td>
                <td>unlimited (dynamically allocated when needed)</td>
            </tr>
            <tr>
                <td>Max number of retired pointers<sup>1</sup></td>
                <td>bounded</td>
                <td>bounded</td>
                <td>bounded</td>
           </tr>
            <tr>
                <td>Array of retired pointers</td>
                <td>preallocated for each thread, limited in size</td>
                <td>preallocated for each thread, limited in size</td>
                <td>global for the entire process, unlimited (dynamically allocated when needed)</td>
            </tr>
            <tr>
                <td>Support direct pointer to item of lock-free container (useful for iterators)</td>
                <td>not supported</td>
                <td>potentially supported (not implemented)</td>
                <td>not supported</td>
            </tr>
        </table>

        <sup>1</sup>Unbounded count of retired pointer means a possibility of memory exhaustion.
    */

    /// Different safe memory reclamation schemas (garbage collectors)
    /** @ingroup cds_garbage_collector

        This namespace specifies different safe memory reclamation (SMR) algorithms.
        See \ref cds_garbage_collector "Garbage collectors"
    */
    namespace gc {

    /// Michael's Hazard Pointers reclamation schema
    /**
    \par Sources:
        - [2002] Maged M.Michael "Safe memory reclamation for dynamic lock-freeobjects using atomic reads and writes"
        - [2003] Maged M.Michael "Hazard Pointers: Safe memory reclamation for lock-free objects"
        - [2004] Andrei Alexandrescy, Maged Michael "Lock-free Data Structures with Hazard Pointers"


        The cds::gc::hzp namespace and its members are internal representation of Hazard Pointer GC and should not be used directly.
        Use cds::gc::HP class in your code.

        Hazard Pointer garbage collector is a singleton. The main user-level part of Hazard Pointer schema is
        GC class and its nested classes. Before use any HP-related class you must initialize HP garbage collector
        by contructing cds::gc::HP object in beginning of your main().
        See cds::gc::HP class for explanation.
    */
    namespace hzp {

        namespace details {
            /// Hazard pointer record of the thread
            /**
                The structure of type "single writer - multiple reader": only the owner thread may write to this structure
                other threads have read-only access.
            */
            struct HPRec {
                HPAllocator<hazard_pointer>    m_hzp        ; ///< array of hazard pointers. Implicit \ref CDS_DEFAULT_ALLOCATOR dependency
                retired_vector            m_arrRetired ; ///< Retired pointer array

                /// Ctor
                HPRec( const cds::gc::hzp::GarbageCollector& HzpMgr ) ;    // inline
                ~HPRec()
                {}

                /// Clears all hazard pointers
                void clear()
                {
                    m_hzp.clear()    ;
                }
            };
        }    // namespace details

        /// GarbageCollector::Scan phase strategy
        /**
            See GarbageCollector::Scan for explanation
        */
        enum scan_type {
            classic,    ///< classic scan as described in Michael's works (see GarbageCollector::classic_scan)
            inplace     ///< inplace scan without allocation (see GarbageCollector::inplace_scan)
        };

        /// Hazard Pointer singleton
        /**
            Safe memory reclamation schema by Michael "Hazard Pointers"

        \par Sources:
            \li [2002] Maged M.Michael "Safe memory reclamation for dynamic lock-freeobjects using atomic reads and writes"
            \li [2003] Maged M.Michael "Hazard Pointers: Safe memory reclamation for lock-free objects"
            \li [2004] Andrei Alexandrescy, Maged Michael "Lock-free Data Structures with Hazard Pointers"

        */
        class CDS_EXPORT_API GarbageCollector
        {
        public:
            typedef cds::atomicity::event_counter  event_counter   ;   ///< event counter type

            /// Internal GC statistics
            struct InternalState {
                size_t              nHPCount                ;   ///< HP count per thread (const)
                size_t              nMaxThreadCount         ;   ///< Max thread count (const)
                size_t              nMaxRetiredPtrCount     ;   ///< Max retired pointer count per thread (const)
                size_t              nHPRecSize              ;   ///< Size of HP record, bytes (const)

                size_t              nHPRecAllocated         ;   ///< Count of HP record allocations
                size_t              nHPRecUsed              ;   ///< Count of HP record used
                size_t              nTotalRetiredPtrCount   ;   ///< Current total count of retired pointers
                size_t              nRetiredPtrInFreeHPRecs ;   ///< Count of retired pointer in free (unused) HP records

                event_counter::value_type   evcAllocHPRec   ;   ///< Count of HPRec allocations
                event_counter::value_type   evcRetireHPRec  ;   ///< Count of HPRec retire events
                event_counter::value_type   evcAllocNewHPRec;   ///< Count of new HPRec allocations from heap
                event_counter::value_type   evcDeleteHPRec  ;   ///< Count of HPRec deletions

                event_counter::value_type   evcScanCall     ;   ///< Count of Scan calling
                event_counter::value_type   evcHelpScanCall ;   ///< Count of HelpScan calling
                event_counter::value_type   evcScanFromHelpScan;///< Count of Scan calls from HelpScan

                event_counter::value_type   evcDeletedNode  ;   ///< Count of deleting of retired objects
                event_counter::value_type   evcDeferredNode ;   ///< Count of objects that cannot be deleted in Scan phase because of a hazard_pointer guards it
            } ;

            /// No GarbageCollector object is created
            CDS_DECLARE_EXCEPTION( HZPManagerEmpty, "Global Hazard Pointer GarbageCollector is NULL" )    ;

            /// Not enough required Hazard Pointer count
            CDS_DECLARE_EXCEPTION( HZPTooMany, "Not enough required Hazard Pointer count" )    ;

        private:
            /// Internal GC statistics
            struct Statistics {
                event_counter  m_AllocHPRec            ;    ///< Count of HPRec allocations
                event_counter  m_RetireHPRec            ;    ///< Count of HPRec retire events
                event_counter  m_AllocNewHPRec            ;    ///< Count of new HPRec allocations from heap
                event_counter  m_DeleteHPRec            ;    ///< Count of HPRec deletions

                event_counter  m_ScanCallCount            ;    ///< Count of Scan calling
                event_counter  m_HelpScanCallCount        ;    ///< Count of HelpScan calling
                event_counter  m_CallScanFromHelpScan    ;    ///< Count of Scan calls from HelpScan

                event_counter  m_DeletedNode            ;    ///< Count of retired objects deleting
                event_counter  m_DeferredNode            ;    ///< Count of objects that cannot be deleted in Scan phase because of a hazard_pointer guards it
            };

            /// Internal list of cds::gc::hzp::details::HPRec
            struct hplist_node: public details::HPRec
            {
                hplist_node *                       m_pNextNode ; ///< next hazard ptr record in list
                CDS_ATOMIC::atomic<OS::ThreadId>    m_idOwner   ; ///< Owner thread id; 0 - the record is free (not owned)
                CDS_ATOMIC::atomic<bool>            m_bFree     ; ///< true if record if free (not owned)

                //@cond
                hplist_node( const GarbageCollector& HzpMgr )
                    : HPRec( HzpMgr ),
                    m_pNextNode(NULL),
                    m_idOwner( OS::nullThreadId() ),
                    m_bFree( true )
                {}

                ~hplist_node()
                {
                    assert( m_idOwner.load(CDS_ATOMIC::memory_order_relaxed) == OS::nullThreadId() )    ;
                    assert( m_bFree.load(CDS_ATOMIC::memory_order_relaxed) )    ;
                }
                //@endcond
            };

            CDS_ATOMIC::atomic<hplist_node *>   m_pListHead  ;  ///< Head of GC list

            static GarbageCollector *    m_pHZPManager  ;   ///< GC instance pointer

            Statistics              m_Stat              ;   ///< Internal statistics
            bool                    m_bStatEnabled      ;   ///< true - statistics enabled

            const size_t            m_nHazardPointerCount   ;   ///< max count of thread's hazard pointer
            const size_t            m_nMaxThreadCount       ;   ///< max count of thread
            const size_t            m_nMaxRetiredPtrCount   ;   ///< max count of retired ptr per thread
            scan_type               m_nScanType             ;   ///< scan type (see \ref scan_type enum)


        private:
            /// Ctor
            GarbageCollector(
                size_t nHazardPtrCount = 0,         ///< Hazard pointer count per thread
                size_t nMaxThreadCount = 0,         ///< Max count of thread
                size_t nMaxRetiredPtrCount = 0,     ///< Capacity of the array of retired objects
                scan_type nScanType = inplace       ///< Scan type (see \ref scan_type enum)
            )    ;

            /// Dtor
            ~GarbageCollector()    ;

            /// Allocate new HP record
            hplist_node * NewHPRec()    ;

            /// Permanently deletes HPrecord \p pNode
            /**
                Caveat: for performance reason this function is defined as inline and cannot be called directly
            */
            void                DeleteHPRec( hplist_node * pNode );

            /// Permanently deletes retired pointer \p p
            /**
                Caveat: for performance reason this function is defined as inline and cannot be called directly
            */
            void                DeletePtr( details::retired_ptr& p );

            //@cond
            void detachAllThread()  ;
            //@endcond

        public:
            /// Creates GarbageCollector singleton
            /**
                GC is the singleton. If GC instance is not exist then the function creates the instance.
                Otherwise it does nothing.

                The Michael's HP reclamation schema depends of three parameters:

                \p nHazardPtrCount - HP pointer count per thread. Usually it is small number (2-4) depending from
                                     the data structure algorithms. By default, if \p nHazardPtrCount = 0,
                                     the function uses maximum of HP count for CDS library.

                \p nMaxThreadCount - max count of thread with using HP GC in your application. Default is 100.

                \p nMaxRetiredPtrCount - capacity of array of retired pointers for each thread. Must be greater than
                                    \p nHazardPtrCount * \p nMaxThreadCount.
                                    Default is 2 * \p nHazardPtrCount * \p nMaxThreadCount.
            */
            static void    CDS_STDCALL Construct(
                size_t nHazardPtrCount = 0,     ///< Hazard pointer count per thread
                size_t nMaxThreadCount = 0,     ///< Max count of simultaneous working thread in your application
                size_t nMaxRetiredPtrCount = 0, ///< Capacity of the array of retired objects for the thread
                scan_type nScanType = inplace   ///< Scan type (see \ref scan_type enum)
            );

            /// Destroys global instance of GarbageCollector
            /**
                The parameter \p bDetachAll should be used carefully: if its value is \p true,
                then the destroying GC automatically detaches all attached threads. This feature
                can be useful when you have no control over the thread termination, for example,
                when \p libcds is injected into existing external thread.
            */
            static void CDS_STDCALL Destruct(
                bool bDetachAll = false     ///< Detach all threads
            )    ;

            /// Returns pointer to GarbageCollector instance
            static GarbageCollector&   instance()
            {
                if ( m_pHZPManager == NULL )
                    throw HZPManagerEmpty()    ;
                return *m_pHZPManager   ;
            }

            /// Checks if global GC object is constructed and may be used
            static bool isUsed()
            {
                return m_pHZPManager != NULL    ;
            }

            /// Returns max Hazard Pointer count defined in construction time
            size_t            getHazardPointerCount() const        { return m_nHazardPointerCount; }

            /// Returns max thread count defined in construction time
            size_t            getMaxThreadCount() const             { return m_nMaxThreadCount; }

            /// Returns max size of retired objects array. It is defined in construction time
            size_t            getMaxRetiredPtrCount() const        { return m_nMaxRetiredPtrCount; }

            // Internal statistics

            /// Get internal statistics
            InternalState& getInternalState(InternalState& stat) const    ;

            /// Checks if internal statistics enabled
            bool              isStatisticsEnabled() const { return m_bStatEnabled; }

            /// Enables/disables internal statistics
            bool              enableStatistics( bool bEnable )
            {
                bool bEnabled = m_bStatEnabled    ;
                m_bStatEnabled = bEnable        ;
                return bEnabled                    ;
            }

            /// Checks that required hazard pointer count \p nRequiredCount is less or equal then max hazard pointer count
            /**
                If \p nRequiredCount > getHazardPointerCount() then the exception HZPTooMany is thrown
            */
            static void checkHPCount( unsigned int nRequiredCount )
            {
                if ( instance().getHazardPointerCount() < nRequiredCount )
                    throw HZPTooMany()  ;
            }

            /// Get current scan strategy
            scan_type getScanType() const
            {
                return m_nScanType  ;
            }

            /// Set current scan strategy
            /** @anchor hzp_gc_setScanType
                Scan strategy changing is allowed on the fly.
            */
            void setScanType(
                scan_type nScanType     ///< new scan strategy
            )
            {
                m_nScanType = nScanType ;
            }

        public:    // Internals for threads

            /// Allocates Hazard Pointer GC record. For internal use only
            details::HPRec * AllocateHPRec()    ;

            /// Free HP record. For internal use only
            void RetireHPRec( details::HPRec * pRec )    ;

            /// The main garbage collecting function
            /**
                This function is called internally by ThreadGC object when upper bound of thread's list of reclaimed pointers
                is reached.

                There are the following scan algorithm:
                - \ref hzp_gc_classic_scan "classic_scan" allocates memory for internal use
                - \ref hzp_gc_inplace_scan "inplace_scan" does not allocate any memory

                Use \ref hzp_gc_setScanType "setScanType" member function to setup appropriate scan algorithm.
            */
            void Scan( details::HPRec * pRec )
            {
                switch ( m_nScanType ) {
                    case inplace:
                        inplace_scan( pRec )   ;
                        break;
                    default:
                        assert(false)   ;   // Forgotten something?..
                    case classic:
                        classic_scan( pRec )    ;
                        break;
                }
            }

            /// Helper scan routine
            /**
                The function guarantees that every node that is eligible for reuse is eventually freed, barring
                thread failures. To do so, after executing Scan, a thread executes a HelpScan,
                where it checks every HP record. If an HP record is inactive, the thread moves all "lost" reclaimed pointers
                to thread's list of reclaimed pointers.

                The function is called internally by Scan.
            */
            void HelpScan( details::HPRec * pThis ) ;

        protected:
            /// Classic scan algorithm
            /** @anchor hzp_gc_classic_scan
                Classical scan algorithm as described in Michael's paper.

                A scan includes four stages. The first stage involves scanning the array HP for non-null values.
                Whenever a non-null value is encountered, it is inserted in a local list of currently protected pointer.
                Only stage 1 accesses shared variables. The following stages operate only on private variables.

                The second stage of a scan involves sorting local list of protected pointers to allow
                binary search in the third stage.

                The third stage of a scan involves checking each reclaimed node
                against the pointers in local list of protected pointers. If the binary search yields
                no match, the node is freed. Otherwise, it cannot be deleted now and must kept in thread's list
                of reclaimed pointers.

                The forth stage prepares new thread's private list of reclaimed pointers
                that could not be freed during the current scan, where they remain until the next scan.

                This algorithm allocates memory for internal HP array.

                This function is called internally by ThreadGC object when upper bound of thread's list of reclaimed pointers
                is reached.
            */
            void classic_scan( details::HPRec * pRec ) ;

            /// In-place scan algorithm
            /** @anchor hzp_gc_inplace_scan
                Unlike the \ref hzp_gc_classic_scan "classic_scan" algorithm, \p inplace_scan does not allocate any memory.
                All operations are performed in-place.
            */
            void inplace_scan( details::HPRec * pRec );
        };

        /// Thread's hazard pointer manager
        /**
            To use Hazard Pointer reclamation schema each thread object must be linked with the object of ThreadGC class
            that interacts with GarbageCollector global object. The linkage is performed by calling \ref cds_threading "cds::threading::Manager::attachThread()"
            on the start of each thread that uses HP GC. Before terminating the thread linked to HP GC it is necessary to call
            \ref cds_threading "cds::threading::Manager::detachThread()".
        */
        class ThreadGC: cds::details::noncopyable
        {
            GarbageCollector&   m_HzpManager    ; ///< Hazard Pointer GC singleton
            details::HPRec *    m_pHzpRec       ; ///< Pointer to thread's HZP record

        public:
            ThreadGC()
                : m_HzpManager( GarbageCollector::instance() ),
                m_pHzpRec( NULL )
            {}
            ~ThreadGC()
            {
                fini()    ;
            }

            /// Checks if thread GC is initialized
            bool    isInitialized() const   { return m_pHzpRec != NULL ; }

            /// Initialization. Repeat call is available
            void init()
            {
                if ( !m_pHzpRec )
                    m_pHzpRec = m_HzpManager.AllocateHPRec() ;
            }

            /// Finalization. Repeat call is available
            void fini()
            {
                if ( m_pHzpRec ) {
                    details::HPRec * pRec = m_pHzpRec    ;
                    m_pHzpRec = NULL    ;
                    m_HzpManager.RetireHPRec( pRec ) ;
                }
            }

            /// Initializes HP guard \p guard
            details::HPGuard& allocGuard()
            {
                assert( m_pHzpRec != NULL )     ;
                return m_pHzpRec->m_hzp.alloc() ;
            }

            /// Frees HP guard \p guard
            void freeGuard( details::HPGuard& guard )
            {
                assert( m_pHzpRec != NULL )    ;
                m_pHzpRec->m_hzp.free( guard )    ;
            }

            /// Initializes HP guard array \p arr
            template <size_t Count>
            void allocGuard( details::HPArray<Count>& arr )
            {
                assert( m_pHzpRec != NULL )    ;
                m_pHzpRec->m_hzp.alloc( arr )    ;
            }

            /// Frees HP guard array \p arr
            template <size_t Count>
            void freeGuard( details::HPArray<Count>& arr )
            {
                assert( m_pHzpRec != NULL )    ;
                m_pHzpRec->m_hzp.free( arr )    ;
            }

            /// Places retired pointer \p and its deleter \p pFunc into thread's array of retired pointer for deferred reclamation
            template <typename T>
            void retirePtr( T * p, void (* pFunc)(T *) )
            {
                retirePtr( details::retired_ptr( reinterpret_cast<void *>( p ), reinterpret_cast<free_retired_ptr_func>( pFunc ) ) )    ;
            }

            /// Places retired pointer \p into thread's array of retired pointer for deferred reclamation
            void retirePtr( const details::retired_ptr& p )
            {
                m_pHzpRec->m_arrRetired.push( p ) ;

                if ( m_pHzpRec->m_arrRetired.isFull() ) {
                    // Max of retired pointer count is reached. Do scan
                    scan()  ;
                }
            }

            //@cond
            void scan()
            {
                m_HzpManager.Scan( m_pHzpRec )     ;
                m_HzpManager.HelpScan( m_pHzpRec ) ;
            }
            //@endcond
        };

        /// Auto HPGuard.
        /**
            This class encapsulates Hazard Pointer guard to protect a pointer against deletion .
            It allocates one HP from thread's HP array in constructor and free the HP allocated in destruction time.
        */
        class AutoHPGuard
        {
            //@cond
            details::HPGuard&   m_hp    ; ///< Hazard pointer guarded
            ThreadGC&           m_gc    ; ///< Thread GC
            //@endcond

        public:
            typedef details::HPGuard::hazard_ptr hazard_ptr ;  ///< Hazard pointer type
        public:
            /// Allocates HP guard from \p gc
            AutoHPGuard( ThreadGC& gc )
                : m_hp( gc.allocGuard() )
                , m_gc( gc )
            {}

            /// Allocates HP guard from \p gc and protects the pointer \p p of type \p T
            template <typename T>
            AutoHPGuard( ThreadGC& gc, T * p  )
                : m_hp( gc.allocGuard() )
                , m_gc( gc )
            {
                m_hp = p                ;
            }

            /// Frees HP guard. The pointer guarded may be deleted after this.
            ~AutoHPGuard()
            {
                m_gc.freeGuard( m_hp )    ;
            }

            /// Returns thread GC
            ThreadGC&    getGC() const
            {
                return m_gc;
            }

            /// Protects the pointer \p p against reclamation (guards the pointer).
            template <typename T>
            T * operator =( T * p )
            {
                return m_hp = p ;
            }

            //@cond
            hazard_ptr get() const
            {
                return m_hp ;
            }
            //@endcond
        };

        /// Auto-managed array of hazard pointers
        /**
            This class is wrapper around cds::gc::hzp::details::HPArray class.
            \p Count is the size of HP array
        */
        template <size_t Count>
        class AutoHPArray: public details::HPArray<Count>
        {
            ThreadGC&    m_mgr    ;    ///< Thread GC

        public:
            /// Rebind array for other size \p COUNT2
            template <size_t Count2>
            struct rebind {
                typedef AutoHPArray<Count2>  other   ;   ///< rebinding result
            };

        public:
            /// Allocates array of HP guard from \p mgr
            AutoHPArray( ThreadGC& mgr )
                : m_mgr( mgr )
            {
                mgr.allocGuard( *this )    ;
            }

            /// Frees array of HP guard
            ~AutoHPArray()
            {
                m_mgr.freeGuard( *this )    ;
            }

            /// Returns thread GC
            ThreadGC&    getGC() const { return m_mgr; }
        };

    }   // namespace hzp
}}  // namespace cds::gc

// Inlines
#include <cds/gc/hzp/details/hp_inline.h>

#if CDS_COMPILER == CDS_COMPILER_MSVC
#   pragma warning(pop)
#endif

#endif  // #ifndef __CDS_GC_HZP_HZP_H
