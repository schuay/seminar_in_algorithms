//$$CDS-header$$

// Forward declarations
namespace cds {
    namespace intrusive {}
    namespace opt {}
}

// Including this header is a bad thing for header testing. How to avoid it?..
#include <cds/cxx11_atomic.h>   // for cds::atomicity::empty_item_counter

namespace test_beans {
    template <typename ItemCounter>
    struct check_item_counter {
        bool operator()( size_t nReal, size_t nExpected )
        {
            return nReal == nExpected ;
        }
    };

    template <>
    struct check_item_counter<cds::atomicity::empty_item_counter>
    {
        bool operator()( size_t nReal, size_t /*nExpected*/ )
        {
            return nReal == 0    ;
        }
    };
} // namespace beans
