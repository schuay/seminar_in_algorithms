//$$CDS-header$$

#ifndef __CDS_COMPILER_BITOP_H
#define __CDS_COMPILER_BITOP_H

// Choose appropriate header for current architecture and compiler

#if CDS_COMPILER == CDS_COMPILER_MSVC || (CDS_COMPILER == CDS_COMPILER_INTEL && CDS_OS_INTERFACE == CDS_OSI_WINDOWS)
/************************************************************************/
/* MS Visual C++                                                        */
/************************************************************************/

#    if CDS_PROCESSOR_ARCH == CDS_PROCESSOR_X86
#        include <cds/compiler/vc/x86/bitop.h>
#   elif CDS_PROCESSOR_ARCH == CDS_PROCESSOR_AMD64
#        include <cds/compiler/vc/amd64/bitop.h>
#    endif

#elif CDS_COMPILER == CDS_COMPILER_GCC || CDS_COMPILER == CDS_COMPILER_CLANG || CDS_COMPILER == CDS_COMPILER_INTEL
/************************************************************************/
/* GCC                                                                  */
/************************************************************************/

#    if CDS_PROCESSOR_ARCH == CDS_PROCESSOR_X86
#       include <cds/compiler/gcc/x86/bitop.h>
#    elif CDS_PROCESSOR_ARCH == CDS_PROCESSOR_AMD64
#       include <cds/compiler/gcc/amd64/bitop.h>
#    elif CDS_PROCESSOR_ARCH == CDS_PROCESSOR_SPARC
#       include <cds/compiler/gcc/sparc/bitop.h>
#    elif CDS_PROCESSOR_ARCH == CDS_PROCESSOR_IA64
#       include <cds/compiler/gcc/ia64/bitop.h>
#    elif CDS_PROCESSOR_ARCH == CDS_PROCESSOR_PPC64
#       include <cds/compiler/gcc/ppc64/bitop.h>
#   endif
#endif  // Compiler choice

// Generic (C) implementation
#include <cds/details/bitop_generic.h>

#endif    // #ifndef __CDS_COMPILER_BITOP_H
