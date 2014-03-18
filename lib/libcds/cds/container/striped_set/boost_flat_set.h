//$$CDS-header$$

#ifndef __CDS_CONTAINER_STRIPED_SET_BOOST_FLAT_SET_ADAPTER_H
#define __CDS_CONTAINER_STRIPED_SET_BOOST_FLAT_SET_ADAPTER_H

#include <boost/version.hpp>
#if BOOST_VERSION < 104800
#   error "For boost::container::flat_set you must use boost 1.48 or above"
#endif

#include <cds/container/striped_set/adapter.h>
#include <boost/container/flat_set.hpp>

//#if CDS_COMPILER == CDS_COMPILER_MSVC && CDS_COMPILER_VERSION >= 1700
//#   error "boost::container::flat_set is not compatible with MS VC++ 11"
//#endif

//@cond
namespace cds { namespace container {
    namespace striped_set {

        // Copy policy for boost::container::flat_set
        template <typename T, typename Traits, typename Alloc>
        struct copy_item_policy< boost::container::flat_set< T, Traits, Alloc > >
            : public details::boost_set_copy_policies< boost::container::flat_set< T, Traits, Alloc > >::copy_item_policy
        {};

        // Swap policy is not defined for boost::container::flat_set
        template <typename T, typename Traits, typename Alloc>
        struct swap_item_policy< boost::container::flat_set< T, Traits, Alloc > >
            : public details::boost_set_copy_policies< boost::container::flat_set< T, Traits, Alloc > >::swap_item_policy
        {};

#ifdef CDS_MOVE_SEMANTICS_SUPPORT
        // Move policy for boost::container::flat_set
        template <typename T, typename Traits, typename Alloc>
        struct move_item_policy< boost::container::flat_set< T, Traits, Alloc > >
            : public details::boost_set_copy_policies< boost::container::flat_set< T, Traits, Alloc > >::move_item_policy
        {};
#endif

    }   // namespace striped_set
}} // namespace cds::container

namespace cds { namespace intrusive { namespace striped_set {
    template <typename T, class Traits, class Alloc, CDS_SPEC_OPTIONS>
    class adapt< boost::container::flat_set<T, Traits, Alloc>, CDS_OPTIONS >
    {
    public:
        typedef boost::container::flat_set<T, Traits, Alloc>    container_type ;   ///< underlying container type
        typedef cds::container::striped_set::details::boost_set_adapter< container_type, CDS_OPTIONS >    type ;
    };
}}}

//@endcond

#endif // #ifndef __CDS_CONTAINER_STRIPED_SET_BOOST_FLAT_SET_ADAPTER_H
