//$$CDS-header$$

#ifndef __CDS_COMPILER_VC_X86_BITOP_H
#define __CDS_COMPILER_VC_X86_BITOP_H

#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)

//@cond none
namespace cds {
    namespace bitop { namespace platform { namespace vc { namespace x86 {
        // MSB - return index (1..32) of most significant bit in nArg. If nArg == 0 return 0
#        define cds_bitop_msb32_DEFINED
        static inline int msb32( atomic32u_t nArg )
        {
            unsigned long nIndex    ;
            if ( _BitScanReverse( &nIndex, nArg ))
                return (int) nIndex + 1   ;
            return 0    ;
        }

#        define cds_bitop_msb32nz_DEFINED
        static inline int msb32nz( atomic32u_t nArg )
        {
            assert( nArg != 0 )    ;
            unsigned long nIndex    ;
            _BitScanReverse( &nIndex, nArg )    ;
            return (int) nIndex       ;
        }

        // LSB - return index (1..32) of least significant bit in nArg. If nArg == 0 return -1U
#        define cds_bitop_lsb32_DEFINED
        static inline int lsb32( atomic32u_t nArg )
        {
            unsigned long nIndex    ;
            if ( _BitScanForward( &nIndex, nArg ))
                return (int) nIndex + 1   ;
            return 0    ;
        }

#        define cds_bitop_lsb32nz_DEFINED
        static inline int lsb32nz( atomic32u_t nArg )
        {
            assert( nArg != 0 )    ;
            unsigned long nIndex    ;
            _BitScanForward( &nIndex, nArg )    ;
            return (int) nIndex       ;
        }

        // bswap - Reverses the byte order of a 32-bit word
#        define cds_bitop_bswap32_DEFINED
        static inline atomic32u_t bswap32( atomic32u_t nArg )
        {
            __asm {
                mov    eax, nArg    ;
                bswap    eax        ;
            }
        }

#       define cds_bitop_complement32_DEFINED
        static inline bool complement32( atomic32u_t * pArg, unsigned int nBit )
        {
            return _bittestandcomplement( reinterpret_cast<long *>( pArg ), nBit ) != 0 ;
        }

#       define cds_bitop_complement64_DEFINED
        static inline bool complement64( atomic64u_t * pArg, unsigned int nBit )
        {
            if ( nBit < 32 )
                return _bittestandcomplement( reinterpret_cast<long *>( pArg ), nBit ) != 0 ;
            else
                return _bittestandcomplement( reinterpret_cast<long *>( pArg ) + 1, nBit - 32 ) != 0 ;
        }

    }} // namespace vc::x86

    using namespace vc::x86     ;

}}}    // namespace cds::bitop::platform
//@endcond

#endif    // #ifndef __CDS_COMPILER_VC_X86_BITOP_H
