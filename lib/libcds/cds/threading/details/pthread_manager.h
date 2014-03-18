//$$CDS-header$$

#ifndef __CDS_THREADING_DETAILS_PTHREAD_MANAGER_H
#define __CDS_THREADING_DETAILS_PTHREAD_MANAGER_H

#include <stdio.h>
#include <pthread.h>
#include <cds/threading/details/_common.h>

//@cond
namespace cds { namespace threading {

    /// cds::threading::Manager implementation based on pthread thread-specific data functions
    CDS_CXX11_INLINE_NAMESPACE namespace pthread {

        /// Thread-specific data manager based on pthread thread-specific data functions
        /**
            Manager throws an exception of Manager::pthread_exception class if an error occurs
        */
        class Manager {
        private :
            /// pthread error code type
            typedef int pthread_error_code  ;

            /// pthread exception
            class pthread_exception: public cds::Exception {
            public:
                const pthread_error_code    m_errCode   ;   ///< pthread error code, -1 if no pthread error code
            public:
                /// Exception constructor
                pthread_exception( pthread_error_code nCode, const char * pszFunction )
                    : m_errCode( nCode )
                {
                    char buf[256]   ;
                    sprintf( buf, "Pthread error %i [function %s]", nCode, pszFunction ) ;
                    m_strMsg = buf  ;
                }
            };

            /// pthread TLS key holder
            struct Holder {
            //@cond
                static pthread_key_t   m_key;

                static void key_destructor(void * p)
                {
                    if ( p ) {
                        reinterpret_cast<ThreadData *>(p)->fini()   ;
                        delete reinterpret_cast<ThreadData *>(p)    ;
                    }
                }

                static void init()
                {
                    pthread_error_code  nErr    ;
                    if ( (nErr = pthread_key_create( &m_key, key_destructor )) != 0 )
                        throw pthread_exception( nErr, "pthread_key_create" )   ;
                }

                static void fini()
                {
                    pthread_error_code  nErr    ;
                    if ( (nErr = pthread_key_delete( m_key )) != 0 )
                        throw pthread_exception( nErr, "pthread_key_delete" )   ;
                }

                static ThreadData *    get()
                {
                    return reinterpret_cast<ThreadData *>( pthread_getspecific( m_key ) )    ;
                }

                static void alloc()
                {
                    pthread_error_code  nErr    ;
                    ThreadData * pData = new ThreadData ;
                    if ( ( nErr = pthread_setspecific( m_key, pData )) != 0 )
                        throw pthread_exception( nErr, "pthread_setspecific" )   ;
                }
                static void free()
                {
                    ThreadData * p  ;
                    if ( (p = get()) != NULL ) {
                        delete p    ;

                        pthread_setspecific( m_key, NULL )  ;
                    }
                }
            //@endcond
            };

            //@cond
            enum EThreadAction {
                do_getData,
                do_attachThread,
                do_detachThread,
                do_checkData,
                init_holder,
                fini_holder
            };
            //@endcond

            //@cond
            static ThreadData * _threadData( EThreadAction nAction )
            {
                switch ( nAction ) {
                    case do_getData:
                        return Holder::get() ;
                    case do_checkData:
                        return Holder::get() ;
                    case do_attachThread:
                        if ( Holder::get() == NULL )
                            Holder::alloc()  ;
                        return Holder::get() ;
                    case do_detachThread:
                        Holder::free()   ;
                        return NULL     ;
                    case init_holder:
                    case fini_holder:
                        break;
                    default:
                        assert( false ) ;   // anything forgotten?..
                }
                assert(false)   ;   // how did we get here?
                return NULL     ;
            }
            //@endcond

        public:
            /// Initialize manager
            /**
                This function is automatically called by cds::Initialize
            */
            static void init()
            {
                Holder::init()  ;
            }

            /// Terminate manager
            /**
                This function is automatically called by cds::Terminate
            */
            static void fini()
            {
                Holder::fini()  ;
            }

            /// Checks whether current thread is attached to \p libcds feature or not.
            static bool isThreadAttached()
            {
                return _threadData( do_checkData ) != NULL ;
            }

            /// This method must be called in beginning of thread execution
            /**
                If TLS pointer to manager's data is NULL, pthread_exception is thrown
                with code = -1.
                If an error occurs in call of pthread API function, pthread_exception is thrown
                with pthread error code.
            */
            static void attachThread()
            {
                ThreadData * pData = _threadData( do_attachThread )    ;
                assert( pData != NULL ) ;

                if ( pData ) {
                    pData->init()   ;
                }
                else
                    throw pthread_exception( -1, "cds::threading::pthread::Manager::attachThread" )   ;
            }

            /// This method must be called in end of thread execution
            /**
                If TLS pointer to manager's data is NULL, pthread_exception is thrown
                with code = -1.
                If an error occurs in call of pthread API function, pthread_exception is thrown
                with pthread error code.
            */
            static void detachThread()
            {
                ThreadData * pData = _threadData( do_getData )    ;
                assert( pData != NULL ) ;

                if ( pData ) {
                    if ( pData->fini() )
                        _threadData( do_detachThread )    ;
                }
                else
                    throw pthread_exception( -1, "cds::threading::pthread::Manager::detachThread" )   ;
            }

            /// Returns ThreadData pointer for the current thread
            static ThreadData * thread_data()
            {
                return _threadData( do_getData )    ;
            }

            /// Get gc::HP thread GC implementation for current thread
            /**
                The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
                or if you did not use gc::HP.
                To initialize gc::HP GC you must constuct cds::gc::HP object in the beginning of your application
            */
            static gc::HP::thread_gc_impl&   getHZPGC()
            {
                return *(_threadData( do_getData )->m_hpManager)    ;
            }

            /// Get gc::HRC thread GC implementation for current thread
            /**
                The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
                or if you did not use gc::HRC.
                To initialize gc::HRC GC you must constuct cds::gc::HRC object in the beginning of your application
            */
            static gc::HRC::thread_gc_impl&   getHRCGC()
            {
                return *(_threadData( do_getData )->m_hrcManager)   ;
            }

            /// Get gc::PTB thread GC implementation for current thread
            /**
                The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
                or if you did not use gc::PTB.
                To initialize gc::PTB GC you must constuct cds::gc::PTB object in the beginning of your application
            */
            static gc::PTB::thread_gc_impl&   getPTBGC()
            {
                return *(_threadData( do_getData )->m_ptbManager)   ;
            }

            //@cond
            static size_t fake_current_processor()
            {
                return _threadData( do_getData )->fake_current_processor()  ;
            }
            //@endcond

        } ;

    } // namespace pthread
}} // namespace cds::threading
//@endcond

#endif // #ifndef __CDS_THREADING_DETAILS_PTHREAD_MANAGER_H
