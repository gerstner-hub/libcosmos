#pragma once

/**
 * @file
 * This header determines compilation environment related information and
 * provides compiler or C++ standard library specific features
 **/

// needed for C++ stdlib preprocessor defines to become available
#include <iosfwd>

#if defined(__GLIBCXX__)
#       define COSMOS_GNU_CXXLIB
#elif defined(_LIBCPP_VERSION)
#       define COSMOS_LLVM_CXXLIB
#else
#       error "Couldn't determine the kind of C++ standard library"
#endif

#if defined(__clang__)
#	define COSMOS_CLANG
#elif defined(__GNUC__)
#	define COSMOS_GCC
#endif

#ifdef COSMOS_GCC
// if the compiler supports it then this macro causes printf function style
// sanity checks to be performed for the function this is attached to.
#define COSMOS_FORMAT_PRINTF(format_index, first_vararg_index) __attribute__((format(printf, (format_index), (first_vararg_index))))
#elif defined(COSMOS_CLANG)
#define COSMOS_FORMAT_PRINTF(format_index, first_vararg_index) __attribute__((__format__ (__printf__, (format_index), (first_vararg_index))))
#else
#define COSMOS_FORMAT_PRINTF(format_index, first_Vararg_index)
#endif

namespace cosmos::arch {

#ifdef __x86_64__
#	define COSMOS_X86_64
#	define COSMOS_X86
/// Whether we're sitting on 64-bit x64-64.
constexpr inline bool X86_64 = true;

/*
 * There is scarce documentation about this, but to detect X32 ABI.
 * These preprocessor defines can be checked. This refers to "integer/long/pointer == 32 bit".
 */
#	if defined(__ILP32__) || defined(_ILP32)
#		define COSMOS_X32
constexpr inline bool X32 = true;
#	else
constexpr inline bool X32 = false;
#	endif

#else
constexpr inline bool X86_64 = false;
constexpr inline bool X32 = false;
#endif

#ifdef __i386__
#	define COSMOS_I386
#	define COSMOS_X86
/// Whether we're sitting on 32-bit x86.
constexpr inline bool I386 = true;
#else
constexpr inline bool I386 = false;
#endif

/// Whether we're running on either 32-bit or 64-bit x86.
constexpr inline bool X86 = X86_64 || I386;

#ifdef __arm__
/// Whether we're sitting on arm EABI (32-bit)
#	define COSMOS_ARM
constexpr inline bool ARM = true;
#else
constexpr inline bool ARM = false;
#endif

#ifdef __aarch64__
/// Whether we're sitting on aarch64
#	define COSMOS_AARCH64
constexpr inline bool AARCH64 = true;
#else
constexpr inline bool AARCH64 = false;
#endif

} // end ns
