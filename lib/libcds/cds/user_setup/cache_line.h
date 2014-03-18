//$$CDS-header$$

#ifndef __CDS_USER_SETUP_CACHE_LINE_H
#define __CDS_USER_SETUP_CACHE_LINE_H

/** \file
    \brief Cache-line size definition

    This file defines cache-line size constant. The constant is used many \p libcds algorithm
    to solve false-sharing problem.

    The value of cache-line size must be power of two.
    You may define your cache-line size by defining macro -DCDS_CACHE_LINE_SIZE=xx
    (where \p xx is 2**N: 32, 64, 128,...) at compile-time or you may define your constant
    value just editing \p cds/user_setup/cache_line.h file.

    The default value is 64.
*/

//@cond
namespace cds {
#ifndef CDS_CACHE_LINE_SIZE
    static const size_t c_nCacheLineSize = 64  ;
#else
    static const size_t c_nCacheLineSize = CDS_CACHE_LINE_SIZE  ;
#endif
}   // namespace cds
//@endcond

#endif // #ifndef __CDS_USER_SETUP_CACHE_LINE_H
