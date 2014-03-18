//$$CDS-header$$

#ifndef __CDS_OS_POSIX_ALLOC_ALIGNED_H
#define __CDS_OS_POSIX_ALLOC_ALIGNED_H

#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#   define _XOPEN_SOURCE 600
#endif

#include <stdlib.h>

//@cond none
namespace cds { namespace OS {
    namespace posix {
        /// Allocates memory on a specified alignment boundary
        static inline void * aligned_malloc(
            size_t nSize,       ///< Size of the requested memory allocation
            size_t nAlignment   ///< The alignment value, which must be an integer power of 2
            )
        {
            void * pMem ;
            return ::posix_memalign( &pMem, nAlignment, nSize ) == 0 ? pMem : NULL ;
        }

        /// Frees a block of memory that was allocated with aligned_malloc.
        static inline void aligned_free(
            void * pBlock   ///< A pointer to the memory block that was returned to the aligned_malloc function
            )
        {
            ::free( pBlock )   ;
        }
    }   // namespace posix

    using posix::aligned_malloc ;
    using posix::aligned_free   ;

}} // namespace cds::OS
//@endcond


#endif // #ifndef __CDS_OS_POSIX_ALLOC_ALIGNED_H

