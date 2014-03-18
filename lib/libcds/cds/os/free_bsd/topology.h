//$$CDS-header$$

#ifndef __CDS_OS_FREE_BSD_TOPOLOGY_H
#define __CDS_OS_FREE_BSD_TOPOLOGY_H

#ifndef __CDS_OS_TOPOLOGY_H
#   error "<cds/os/topology.h> must be included instead"
#endif

#include <cds/os/details/fake_topology.h>

#include <sys/types.h>
#include <sys/sysctl.h>

namespace cds { namespace OS {

    /// FreeBSD-specific wrappers
    CDS_CXX11_INLINE_NAMESPACE namespace Free_BSD {

        /// System topology
        /**
            The implementation assumes that processor IDs are in numerical order
            from 0 to N - 1, where N - count of processor in the system
        */
        struct topology: public OS::details::fake_topology
        {
        private:
            //@cond
            typedef OS::details::fake_topology  base_class  ;
            //@endcond
        public:

            /// Logical processor count for the system
            static unsigned int processor_count()
            {
                int mib[4];

                /* set the mib for hw.ncpu */
                mib[0] = CTL_HW     ;
                mib[1] = HW_NCPU    ;
                int nCPU = 0   ;
                size_t len = sizeof(nCPU);

                /* get the number of CPUs from the system */
                return ::sysctl(mib, 2, &nCPU, &len, NULL, 0) == 0 && nCPU > 0 ? (unsigned int) nCPU : 1    ;
            }

            /// Get current processor number
            /**
                Caveat: FreeBSD has no "get current processor number" system call.
                The function emulates "current processor number" using thread-specific data.
            */
            static unsigned int current_processor()
            {
                return base_class::current_processor() ;
            }

            /// Synonym for \ref current_processor
            static unsigned int native_current_processor()
            {
                return current_processor()  ;
            }

            //@cond
            static void init()
            {}
            static void fini()
            {}
            //@endcond
        };
    }   // namespace Free_BSD

#ifndef CDS_CXX11_INLINE_NAMESPACE_SUPPORT
    using Free_BSD::topology   ;
#endif
}}  // namespace cds::OS

#endif  // #ifndef __CDS_OS_FREE_BSD_TOPOLOGY_H
