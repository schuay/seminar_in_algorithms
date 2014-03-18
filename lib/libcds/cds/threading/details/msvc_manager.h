//$$CDS-header$$

#ifndef __CDS_THREADING_DETAILS_MSVC_MANAGER_H
#define __CDS_THREADING_DETAILS_MSVC_MANAGER_H

#if !( CDS_COMPILER == CDS_COMPILER_MSVC || CDS_COMPILER == CDS_COMPILER_INTEL )
#   error "threading/details/msvc_manager.h may be used only with Microsoft Visual C++ / Intel C++ compiler"
#endif

#include <cds/threading/details/_common.h>

//@cond
namespace cds { namespace threading {

    //@cond
    struct msvc_internal {
        typedef unsigned char  ThreadDataPlaceholder[ sizeof(ThreadData) ]  ;
        __declspec( thread ) static ThreadDataPlaceholder s_threadData      ;
        __declspec( thread ) static ThreadData * s_pThreadData ;
    } ;
    //@endcond

    /// cds::threading::Manager implementation based on Microsoft Visual C++ __declspec( thread ) construction
    CDS_CXX11_INLINE_NAMESPACE namespace msvc {

        /// Thread-specific data manager based on MSVC __declspec( thread ) feature
        class Manager {
        private :
            //@cond
            static ThreadData * _threadData()
            {
                return msvc_internal::s_pThreadData ;
            }

            static ThreadData * create_thread_data()
            {
                if ( !msvc_internal::s_pThreadData ) {
                    msvc_internal::s_pThreadData = new (msvc_internal::s_threadData) ThreadData() ;
                }
                return msvc_internal::s_pThreadData ;
            }

            static void destroy_thread_data()
            {
                if ( msvc_internal::s_pThreadData ) {
                    msvc_internal::s_pThreadData->ThreadData::~ThreadData() ;
                    msvc_internal::s_pThreadData = NULL ;
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
                ThreadData * pData = _threadData()    ;
                return pData != NULL ;
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

            /// Returns internal ThreadData pointer for the current thread
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

    } // namespace msvc
}} // namespace cds::threading
//@endcond

#endif // #ifndef __CDS_THREADING_DETAILS_MSVC_MANAGER_H
