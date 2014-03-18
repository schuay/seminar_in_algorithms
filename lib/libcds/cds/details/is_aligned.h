//$$CDS-header$$

#ifndef __CDS_DETAILS_IS_ALIGNED_H
#define __CDS_DETAILS_IS_ALIGNED_H

#include <cds/details/defs.h>

namespace cds { namespace details {

    /// Checks if the pointer \p p has \p ALIGN byte alignment
    /**
        \p ALIGN must be power of 2.

        The function is mostly intended for run-time assertion
    */
    template <int ALIGN, typename T>
    static inline bool is_aligned(T const * p)
    {
        return (((uptr_atomic_t)p) & uptr_atomic_t(ALIGN - 1)) == 0   ;
    }

    /// Checks if the pointer \p p has \p nAlign byte alignment
    /**
        \p nAlign must be power of 2.

        The function is mostly intended for run-time assertion
    */
    template <typename T>
    static inline bool is_aligned(T const * p, size_t nAlign)
    {
        return (((uptr_atomic_t)p) & uptr_atomic_t(nAlign - 1)) == 0   ;
    }

}} // namespace cds::details

#endif // #ifndef __CDS_DETAILS_IS_ALIGNED_H
