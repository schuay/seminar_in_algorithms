//$$CDS-header$$

#ifndef __CDS_CONTAINER_BASE_H
#define __CDS_CONTAINER_BASE_H

#include <cds/intrusive/base.h>
#include <cds/details/allocator.h>

namespace cds {

/// Standard (non-intrusive) containers
/**
    @ingroup cds_nonintrusive_containers
    This namespace contains implementations of non-intrusive (std-like) lock-free containers.
*/
namespace container {

    /// Common options for non-intrusive containers
    /** @ingroup cds_nonintrusive_helper
        This namespace contains options for non-intrusive containers that is, in general, the same as for the intrusive containers.
        It imports all definitions from cds::opt and cds::intrusive::opt namespaces
    */
    namespace opt {
        using namespace cds::intrusive::opt ;
    }   // namespace opt

    /// @defgroup cds_nonintrusive_containers Non-intrusive containers
    /** @defgroup cds_nonintrusive_helper Helper structs for non-intrusive containers
        @ingroup cds_nonintrusive_containers
    */

    /** @defgroup cds_nonintrusive_stack Stack
        @ingroup cds_nonintrusive_containers
    */
    /** @defgroup cds_nonintrusive_queue Queue
        @ingroup cds_nonintrusive_containers
    */
    /** @defgroup cds_nonintrusive_deque Deque
        @ingroup cds_nonintrusive_containers
    */
    /** @defgroup cds_nonintrusive_priority_queue Priority queue
        @ingroup cds_nonintrusive_containers
    */
    /** @defgroup cds_nonintrusive_map Map
        @ingroup cds_nonintrusive_containers
    */
    /** @defgroup cds_nonintrusive_set Set
        @ingroup cds_nonintrusive_containers
    */
    /** @defgroup cds_nonintrusive_list List
        @ingroup cds_nonintrusive_containers
    */
    /** @defgroup cds_nonintrusive_tree Tree
        @ingroup cds_nonintrusive_containers
    */

}   // namespace container
}   // namespace cds

#endif // #ifndef __CDS_CONTAINER_BASE_H
