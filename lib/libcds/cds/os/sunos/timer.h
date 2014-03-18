//$$CDS-header$$

#ifndef __CDS_OS_SUNOS_TIMER_H
#define __CDS_OS_SUNOS_TIMER_H

#ifndef __CDS_OS_TIMER_H
#   error "<cds/os/timer.h> must be included"
#endif

#include <sys/time.h>

//@cond none
namespace cds { namespace OS {
    CDS_CXX11_INLINE_NAMESPACE namespace Sun {

        // High resolution timer
        class Timer {
            hrtime_t    m_tmStart    ;
        public:
            typedef hrtime_t    native_timer_type        ;
            typedef long long    native_duration_type    ;

            Timer() : m_tmStart( current() ) {}

            static native_timer_type    current()            { return gethrtime();    }
            static void current( native_timer_type& tmr )    { tmr = gethrtime();    }

            double reset()
            {
                native_timer_type tt = current()    ;
                double ret = (tt - m_tmStart) / 1.0E9;  //gethrtime() freq = nanoseconds
                m_tmStart = tt;
                return ret;
            }

            double duration( native_duration_type dur )
            {
                return double( dur ) / 1.0E9    ;
            }

            double duration()
            {
                return duration( native_duration() )    ;
            }

            native_duration_type    native_duration()
            {
                return native_duration( m_tmStart, current() ) ;
            }

            static native_duration_type    native_duration( native_timer_type nStart, native_timer_type nEnd )
            {
                return native_duration_type( nEnd - nStart )    ;
            }

            static unsigned long long random_seed()
            {
                return (unsigned long long) current() ;
            }
        };

    }   // namespace Sun

#ifndef CDS_CXX11_INLINE_NAMESPACE_SUPPORT
    typedef Sun::Timer    Timer   ;
#endif

}}  // namespace cds::OS
//@endcond

#endif // #ifndef __CDS_OS_SUNOS_TIMER_H
