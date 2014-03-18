//$$CDS-header$$

#ifndef __CDS_CONTAINER_STRIPED_MAP_BOOST_UNORDERED_MAP_ADAPTER_H
#define __CDS_CONTAINER_STRIPED_MAP_BOOST_UNORDERED_MAP_ADAPTER_H

#include <cds/container/striped_set/adapter.h>
#include <boost/unordered_map.hpp>

//@cond
namespace cds { namespace container {
    namespace striped_set {

        // Copy policy for map
        template <typename Key, typename T, typename Traits, typename Alloc>
        struct copy_item_policy< boost::unordered_map< Key, T, Traits, Alloc > >
            : public details::boost_map_copy_policies<boost::unordered_map< Key, T, Traits, Alloc > >::copy_item_policy
        {};

        // Swap policy for map
        template <typename Key, typename T, typename Traits, typename Alloc>
        struct swap_item_policy< boost::unordered_map< Key, T, Traits, Alloc > >
            : public details::boost_map_copy_policies<boost::unordered_map< Key, T, Traits, Alloc > >::swap_item_policy
        {};

#ifdef CDS_MOVE_SEMANTICS_SUPPORT
        // Move policy for map
        template <typename Key, typename T, typename Traits, typename Alloc>
        struct move_item_policy< boost::unordered_map< Key, T, Traits, Alloc > >
            : public details::boost_map_copy_policies<boost::unordered_map< Key, T, Traits, Alloc > >::move_item_policy
        {};
#endif
    }   // namespace striped_set
}} // namespace cds::container

namespace cds { namespace intrusive { namespace striped_set {

    /// boost::unordered_map  adapter for hash map bucket
    template <typename Key, typename T, class Hash, class Pred, class Alloc, CDS_SPEC_OPTIONS>
    class adapt< boost::unordered_map< Key, T, Hash, Pred, Alloc>, CDS_OPTIONS >
    {
    public:
        typedef boost::unordered_map< Key, T, Hash, Pred, Alloc>  container_type  ;   ///< underlying container type
        typedef cds::container::striped_set::details::boost_map_adapter< container_type, CDS_OPTIONS >    type ;
    };
}}} // namespace cds::intrusive::striped_set


//@endcond

#endif  // #ifndef __CDS_CONTAINER_STRIPED_MAP_BOOST_UNORDERED_MAP_ADAPTER_H
