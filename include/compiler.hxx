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
