//$$CDS-header$$

#ifndef __CDS_OS_DETAILS_FAKE_TOPOLOGY_H
#define __CDS_OS_DETAILS_FAKE_TOPOLOGY_H

#include <cds/details/defs.h>
#include <cds/threading/model.h>

//@cond
namespace cds { namespace OS { namespace details {

    /// Fake system topology
    struct fake_topology {
        /// Logical processor count for the system
        static unsigned int processor_count()
        {
            return 1 ;
        }

        /// Get current processor number
        /**
            The function emulates "current processor number" using thread-specific data.
        */
        static unsigned int current_processor()
        {
            // Use fake "current processor number" assigned for current thread
            return (unsigned int) threading::Manager::fake_current_processor()    ;
        }

        /// Synonym for \ref current_processor
        static unsigned int native_current_processor()
        {
            return current_processor()  ;
        }
    };
}}}  // namespace cds::OS::details
//@endcond

#endif  // #ifndef __CDS_OS_DETAILS_FAKE_TOPOLOGY_H
