//$$CDS-header$$

#ifndef __CDS_COMPILER_GCC_X86_CXX11_ATOMIC_H
#define __CDS_COMPILER_GCC_X86_CXX11_ATOMIC_H
//@cond

#include <cstdint>
#include <cds/compiler/gcc/x86/cxx11_atomic32.h>

namespace cds { namespace cxx11_atomics {
    namespace platform { CDS_CXX11_INLINE_NAMESPACE namespace gcc { CDS_CXX11_INLINE_NAMESPACE namespace x86 {

        //*****************************************************************************
        // 64bit primitives
        //*****************************************************************************

        template <typename T>
        static inline bool cas64_strong( T volatile * pDest, T& expected, T desired, memory_order mo_success, memory_order mo_fail ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T) == 8, "Illegal size of operand" )   ;
            assert( cds::details::is_aligned( pDest, 8 ))  ;

            uint32_t ebxStore;
            T prev = expected;

            fence_before(mo_success);

            // We must save EBX in PIC mode
            __asm__ __volatile__ (
                "movl %%ebx, %[ebxStore]\n"
                "movl %[desiredLo], %%ebx\n"
                "lock; cmpxchg8b 0(%[pDest])\n"
                "movl %[ebxStore], %%ebx\n"
                : [prev] "=A" (prev), [ebxStore] "=m" (ebxStore)
                : [desiredLo] "D" ((int)desired), [desiredHi] "c" ((int)(desired >> 32)), [pDest] "S" (pDest), "0" (prev)
                : "memory");
            bool success = (prev == expected);
            if (success)
                fence_after(mo_success);
            else {
                fence_after(mo_fail);
                expected = prev;
            }
            return success;
        }

        template <typename T>
        static inline bool cas64_weak( T volatile * pDest, T& expected, T desired, memory_order mo_success, memory_order mo_fail ) CDS_NOEXCEPT
        {
            return cas64_strong( pDest, expected, desired, mo_success, mo_fail ) ;
        }

        template <typename T>
        static inline T load64( T volatile const * pSrc, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T) == 8, "Illegal size of operand" )   ;
            assert( order ==  memory_order_relaxed
                || order ==  memory_order_consume
                || order ==  memory_order_acquire
                || order ==  memory_order_seq_cst
                ) ;
            assert( pSrc != NULL )  ;
            assert( cds::details::is_aligned( pSrc, 8 ))  ;

            T CDS_DATA_ALIGNMENT(8) v ;
            __asm__ __volatile__(
                "movq   (%[pSrc]), %[v]   ;   \n\t"
                : [v] "=x" (v)
                : [pSrc] "r" (pSrc)
                :
            );
            return v ;
        }


        template <typename T>
        static inline T exchange64( T volatile * pDest, T v, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T) == 8, "Illegal size of operand" )   ;
            assert( cds::details::is_aligned( pDest, 8 ))  ;

            T cur = load64( pDest, memory_order_relaxed )   ;
            do {
            } while (!cas64_weak( pDest, cur, v, order, memory_order_relaxed )) ;
            return cur ;
        }

        template <typename T>
        static inline void store64( T volatile * pDest, T val, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T) == 8, "Illegal size of operand" )   ;
            assert( order ==  memory_order_relaxed
                || order ==  memory_order_release
                || order == memory_order_seq_cst
                ) ;
            assert( pDest != NULL )  ;
            assert( cds::details::is_aligned( pDest, 8 ))  ;

            if ( order != memory_order_seq_cst ) {
                fence_before( order )   ;
                // Atomically stores 64bit value by SSE instruction
                __asm__ __volatile__(
                    "movq       %[val], (%[pDest])   ;   \n\t"
                    :
                    : [val] "x" (val), [pDest] "r" (pDest)
                    : "memory"
                    )   ;
            }
            else {
                exchange64( pDest, val, order )   ;
            }
        }


        //*****************************************************************************
        // pointer primitives
        //*****************************************************************************

        template <typename T>
        static inline T * exchange_ptr( T * volatile * pDest, T * v, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T *) == sizeof(void *), "Illegal size of operand" )   ;

            return (T *) exchange32( (uint32_t volatile *) pDest, (uint32_t) v, order )  ;
        }

        template <typename T>
        static inline void store_ptr( T * volatile * pDest, T * src, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T *) == sizeof(void *), "Illegal size of operand" )   ;
            assert( order ==  memory_order_relaxed
                || order ==  memory_order_release
                || order == memory_order_seq_cst
                ) ;
            assert( pDest != NULL )  ;

            if ( order != memory_order_seq_cst ) {
                fence_before( order )   ;
                *pDest = src ;
            }
            else {
                exchange_ptr( pDest, src, order )   ;
            }
        }

        template <typename T>
        static inline T * load_ptr( T * volatile const * pSrc, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T *) == sizeof(void *), "Illegal size of operand" )   ;
            assert( order ==  memory_order_relaxed
                || order ==  memory_order_consume
                || order ==  memory_order_acquire
                || order ==  memory_order_seq_cst
                ) ;
            assert( pSrc != NULL )  ;

            T * v = *pSrc ;
            fence_after_load( order )   ;
            return v    ;
        }

        template <typename T>
        static inline bool cas_ptr_strong( T * volatile * pDest, T *& expected, T * desired, memory_order mo_success, memory_order mo_fail ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T *) == sizeof(void *), "Illegal size of operand" )   ;

            return cas32_strong( (uint32_t volatile *) pDest, *reinterpret_cast<uint32_t *>( &expected ), (uint32_t) desired, mo_success, mo_fail )    ;
        }

        template <typename T>
        static inline bool cas_ptr_weak( T * volatile * pDest, T *& expected, T * desired, memory_order mo_success, memory_order mo_fail ) CDS_NOEXCEPT
        {
            return cas_ptr_strong( pDest, expected, desired, mo_success, mo_fail ) ;
        }


    }} // namespace gcc::x86

#ifndef CDS_CXX11_INLINE_NAMESPACE_SUPPORT
        using namespace gcc::x86 ;
#endif
    }   // namespace platform

}}  // namespace cds::cxx11_atomics

//@endcond
#endif // #ifndef __CDS_COMPILER_GCC_X86_CXX11_ATOMIC_H
