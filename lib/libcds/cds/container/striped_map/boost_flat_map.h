//$$CDS-header$$

#ifndef __CDS_CONTAINER_STRIPED_MAP_BOOST_FLAT_MAP_ADAPTER_H
#define __CDS_CONTAINER_STRIPED_MAP_BOOST_FLAT_MAP_ADAPTER_H

#include <boost/version.hpp>
#if BOOST_VERSION < 104800
#   error "For boost::container::flat_map you must use boost 1.48 or above"
#endif

#include <cds/container/striped_set/adapter.h>
#include <boost/container/flat_map.hpp>

//#if CDS_COMPILER == CDS_COMPILER_MSVC && CDS_COMPILER_VERSION >= 1700
//#   error "boost::container::flat_map is not compatible with MS VC++ 11"
//#endif


//@cond
namespace cds { namespace container {
    namespace striped_set {

        // Copy policy for map
        template <typename Key, typename T, typename Traits, typename Alloc>
        struct copy_item_policy< boost::container::flat_map< Key, T, Traits, Alloc > >
            : public details::boost_map_copy_policies<boost::container::flat_map< Key, T, Traits, Alloc > >::copy_item_policy
        {};

        // Swap item policy
        template <typename Key, typename T, typename Traits, typename Alloc>
        struct swap_item_policy< boost::container::flat_map< Key, T, Traits, Alloc > >
            : public details::boost_map_copy_policies<boost::container::flat_map< Key, T, Traits, Alloc > >::swap_item_policy
        {};

#ifdef CDS_MOVE_SEMANTICS_SUPPORT
        // Move policy for map
        template <typename Key, typename T, typename Traits, typename Alloc>
        struct move_item_policy< boost::container::flat_map< Key, T, Traits, Alloc > >
            : public details::boost_map_copy_policies<boost::container::flat_map< Key, T, Traits, Alloc > >::move_item_policy
        {};
#endif
    }   // namespace striped_set
}} // namespace cds::container

namespace cds { namespace intrusive { namespace striped_set {

    template <typename Key, typename T, class Traits, class Alloc, CDS_SPEC_OPTIONS>
    class adapt< boost::container::flat_map< Key, T, Traits, Alloc>, CDS_OPTIONS >
    {
    public:
        typedef boost::container::flat_map< Key, T, Traits, Alloc>     container_type          ;   ///< underlying container type
        typedef cds::container::striped_set::details::boost_map_adapter< container_type, CDS_OPTIONS >    type ;
    };
}}} // namespace cds::intrusive::striped_set

//@endcond

#endif // #ifndef __CDS_CONTAINER_STRIPED_MAP_BOOST_FLAT_MAP_ADAPTER_H
