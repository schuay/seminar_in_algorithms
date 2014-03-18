//$$CDS-header$$

#ifndef _CDS_URCU_DETAILS_SH_DECL_H
#define _CDS_URCU_DETAILS_SH_DECL_H

#include <cds/urcu/details/base.h>

#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
#include <cds/details/static_functor.h>
#include <cds/details/lib.h>

#include <signal.h>

//@cond
namespace cds { namespace urcu { namespace details {

    // We could derive thread_data from thread_list_record
    // but in this case m_nAccessControl would have offset != 0
    // that is not so efficiently
#   define CDS_SHURCU_DECLARE_THREAD_DATA(tag_) \
    template <> struct thread_data<tag_> { \
        CDS_ATOMIC::atomic<uint32_t>        m_nAccessControl ; \
        CDS_ATOMIC::atomic<bool>            m_bNeedMemBar    ; \
        thread_list_record< thread_data >   m_list ; \
        thread_data(): m_nAccessControl(0), m_bNeedMemBar(false) {} \
        ~thread_data() {} \
    }

    CDS_SHURCU_DECLARE_THREAD_DATA( signal_buffered_tag );
    CDS_SHURCU_DECLARE_THREAD_DATA( signal_threaded_tag );

#   undef CDS_SHURCU_DECLARE_THREAD_DATA

    template <typename RCUtag>
    struct sh_singleton_instance
    {
        static CDS_EXPORT_API singleton_vtbl *     s_pRCU ;
    };
#if CDS_COMPILER != CDS_COMPILER_MSVC
    template<> CDS_EXPORT_API singleton_vtbl * sh_singleton_instance< signal_buffered_tag >::s_pRCU  ;
    template<> CDS_EXPORT_API singleton_vtbl * sh_singleton_instance< signal_threaded_tag >::s_pRCU ;
#endif

    template <typename SigRCUtag>
    class sh_thread_gc
    {
    public:
        typedef SigRCUtag                   rcu_tag         ;
        typedef typename rcu_tag::rcu_class rcu_class       ;
        typedef thread_data< rcu_tag >      thread_record   ;
        typedef cds::urcu::details::scoped_lock< sh_thread_gc > scoped_lock     ;

    protected:
        static thread_record * get_thread_record() ;

    public:
        sh_thread_gc()  ;
        ~sh_thread_gc() ;
    public:
        static void access_lock() ;
        static void access_unlock();
        static bool is_locked() ;

        /// Retire pointer \p by the disposer \p Disposer
        template <typename Disposer, typename T>
        static void retire( T * p )
        {
            retire( p, cds::details::static_functor<Disposer, T>::call )    ;
        }

        /// Retire pointer \p by the disposer \p pFunc
        template <typename T>
        static void retire( T * p, void (* pFunc)(T *) )
        {
            retired_ptr rp( reinterpret_cast<void *>( p ), reinterpret_cast<free_retired_ptr_func>( pFunc ) ) ;
            retire( rp )    ;
        }

        /// Retire pointer \p
        static void retire( retired_ptr& p )
        {
            assert( sh_singleton_instance< rcu_tag >::s_pRCU ) ;
            sh_singleton_instance< rcu_tag >::s_pRCU->retire_ptr( p ) ;
        }
    };

#   define CDS_SH_RCU_DECLARE_THREAD_GC( tag_ ) template <> class thread_gc<tag_>: public sh_thread_gc<tag_> {}

    CDS_SH_RCU_DECLARE_THREAD_GC( signal_buffered_tag  );
    CDS_SH_RCU_DECLARE_THREAD_GC( signal_threaded_tag );

#   undef CDS_SH_RCU_DECLARE_THREAD_GC

    template <class RCUtag>
    class sh_singleton: public singleton_vtbl
    {
    public:
        typedef RCUtag  rcu_tag ;
        typedef cds::urcu::details::thread_gc< rcu_tag >   thread_gc       ;

    protected:
        typedef typename thread_gc::thread_record   thread_record ;
        typedef sh_singleton_instance< rcu_tag >    rcu_instance  ;

    protected:
        CDS_ATOMIC::atomic<uint32_t>    m_nGlobalControl;
        thread_list< rcu_tag >          m_ThreadList    ;
        int const                       m_nSigNo        ;

    protected:
        sh_singleton( int nSignal )
            : m_nGlobalControl(1)
            , m_nSigNo( nSignal )
        {
            set_signal_handler()    ;
        }

        ~sh_singleton()
        {
            clear_signal_handler()  ;
        }

    public:
        static sh_singleton * instance()
        {
            return static_cast< sh_singleton *>( rcu_instance::s_pRCU ) ;
        }

        static bool isUsed()
        {
            return rcu_instance::s_pRCU != NULL ;
        }

        int signal_no() const
        {
            return m_nSigNo ;
        }

    public:
        virtual void retire_ptr( retired_ptr& p ) = 0 ;

    public: // thread_gc interface
        thread_record * attach_thread()
        {
            return m_ThreadList.alloc() ;
        }

        void detach_thread( thread_record * pRec )
        {
            m_ThreadList.retire( pRec ) ;
        }

        uint32_t global_control_word( CDS_ATOMIC::memory_order mo ) const
        {
            return m_nGlobalControl.load( mo ) ;
        }

    protected:
        void set_signal_handler()       ;
        void clear_signal_handler()     ;
        static void signal_handler( int signo, siginfo_t * sigInfo, void * context )    ;
        void raise_signal( cds::OS::ThreadId tid ) ;

        template <class Backoff>
        void force_membar_all_threads( Backoff& bkOff ) ;

        void switch_next_epoch()
        {
            m_nGlobalControl.fetch_xor( rcu_tag::c_nControlBit, CDS_ATOMIC::memory_order_seq_cst ) ;
        }
        bool check_grace_period( thread_record * pRec ) const ;

        template <class Backoff>
        void wait_for_quiescent_state( Backoff& bkOff ) ;
    };

#   define CDS_SIGRCU_DECLARE_SINGLETON( tag_ ) \
    template <> class singleton< tag_ > { \
    public: \
        typedef tag_  rcu_tag ; \
        typedef cds::urcu::details::thread_gc< rcu_tag >   thread_gc ; \
    protected: \
        typedef thread_gc::thread_record            thread_record ; \
        typedef sh_singleton_instance< rcu_tag >    rcu_instance  ; \
        typedef sh_singleton< rcu_tag >             rcu_singleton ; \
    public: \
        static bool isUsed() { return rcu_singleton::isUsed() ; } \
        static rcu_singleton * instance() { assert( rcu_instance::s_pRCU ); return static_cast<rcu_singleton *>( rcu_instance::s_pRCU ); } \
        static thread_record * attach_thread() { return instance()->attach_thread() ; } \
        static void detach_thread( thread_record * pRec ) { return instance()->detach_thread( pRec ) ; } \
        static uint32_t global_control_word( CDS_ATOMIC::memory_order mo ) { return instance()->global_control_word( mo ) ; } \
    }

    CDS_SIGRCU_DECLARE_SINGLETON( signal_buffered_tag  );
    CDS_SIGRCU_DECLARE_SINGLETON( signal_threaded_tag );

#   undef CDS_SIGRCU_DECLARE_SINGLETON

}}} // namespace cds::urcu::details
//@endcond

#endif // #ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
#endif // #ifndef _CDS_URCU_DETAILS_SH_DECL_H
