//$$CDS-header$$

#ifndef __CDS_THREADING_DETAILS_CXX11_MANAGER_H
#define __CDS_THREADING_DETAILS_CXX11_MANAGER_H

#include <cds/threading/details/_common.h>

#ifndef CDS_CXX11_THREAD_LOCAL_SUPPORT
#   error "The compiler does not support C++11 thread_local keyword. You cannot use CDS_THREADING_CXX11 threading model."
#endif

//@cond
namespace cds { namespace threading {

    //@cond
    struct cxx11_internal {
        typedef unsigned char  ThreadDataPlaceholder[ sizeof(ThreadData) ]          ;
        static thread_local ThreadDataPlaceholder CDS_DATA_ALIGNMENT(8) s_threadData    ;
        static thread_local ThreadData * s_pThreadData ;
    } ;
    //@endcond

    /// cds::threading::Manager implementation based on c++11 thread_local declaration
    CDS_CXX11_INLINE_NAMESPACE namespace cxx11 {

        /// Thread-specific data manager based on c++11 thread_local feature
        class Manager {
        private :
            //@cond

            static ThreadData * _threadData()
            {
                return cxx11_internal::s_pThreadData ;
            }

            static ThreadData * create_thread_data()
            {
                if ( !cxx11_internal::s_pThreadData ) {
                    cxx11_internal::s_pThreadData = new (cxx11_internal::s_threadData) ThreadData() ;
                }
                return cxx11_internal::s_pThreadData ;
            }

            static void destroy_thread_data()
            {
                if ( cxx11_internal::s_pThreadData ) {
                    cxx11_internal::s_pThreadData->ThreadData::~ThreadData() ;
                    cxx11_internal::s_pThreadData = NULL ;
                }
            }
            //@endcond

        public:
            /// Initialize manager (empty function)
            /**
                This function is automatically called by cds::Initialize
            */
            static void init()
            {}

            /// Terminate manager (empty function)
            /**
                This function is automatically called by cds::Terminate
            */
            static void fini()
            {}

            /// Checks whether current thread is attached to \p libcds feature or not.
            static bool isThreadAttached()
            {
                return _threadData() != NULL ;
            }

            /// This method must be called in beginning of thread execution
            static void attachThread()
            {
                create_thread_data()->init() ;
            }

            /// This method must be called in end of thread execution
            static void detachThread()
            {
                assert( _threadData() ) ;

                if ( _threadData()->fini() )
                    destroy_thread_data() ;
            }

            /// Returns ThreadData pointer for the current thread
            static ThreadData * thread_data()
            {
                ThreadData * p = _threadData() ;
                assert( p ) ;
                return p ;
            }

            /// Get gc::HP thread GC implementation for current thread
            /**
                The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
                or if you did not use gc::HP.
                To initialize gc::HP GC you must constuct cds::gc::HP object in the beginning of your application
            */
            static gc::HP::thread_gc_impl&   getHZPGC()
            {
                assert( _threadData()->m_hpManager != NULL )    ;
                return *(_threadData()->m_hpManager)            ;
            }

            /// Get gc::HRC thread GC implementation for current thread
            /**
                The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
                or if you did not use gc::HRC.
                To initialize gc::HRC GC you must constuct cds::gc::HRC object in the beginning of your application
            */
            static gc::HRC::thread_gc_impl&   getHRCGC()
            {
                assert( _threadData()->m_hrcManager != NULL )   ;
                return *(_threadData()->m_hrcManager)           ;
            }

            /// Get gc::PTB thread GC implementation for current thread
            /**
                The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
                or if you did not use gc::PTB.
                To initialize gc::PTB GC you must constuct cds::gc::PTB object in the beginning of your application
            */
            static gc::PTB::thread_gc_impl&   getPTBGC()
            {
                assert( _threadData()->m_ptbManager != NULL )   ;
                return *(_threadData()->m_ptbManager)           ;
            }

            //@cond
            static size_t fake_current_processor()
            {
                return _threadData()->fake_current_processor()  ;
            }
            //@endcond
        } ;

    } // namespace cxx11

}} // namespace cds::threading
//@endcond

#endif // #ifndef __CDS_THREADING_DETAILS_CXX11_MANAGER_H
