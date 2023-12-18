#ifndef COSMOS_LIMITS_HXX
#define COSMOS_LIMITS_HXX

// C++
#include <cstddef>

// Linux
#include <linux/limits.h>

namespace cosmos {

namespace max {
/*
 * common preprocessor constants used in Linux systems are redefined here as
 * constexpr constants.
 */

/// The maximum length of a path in the file system.
/**
 * Note that this is rather an API limit (in the C library) these days than an
 * actual file system limit. There can be file systems that support even
 * longer paths (accessible via native Linux system calls only) or there can
 * be file systems that only support shorter paths. The latter can be
 * identifier via statvfs().
 **/
constexpr size_t PATH = PATH_MAX;

/// The maximum length of a file name in the file system.
/**
 * Similarly to cosmos::max::PATH this limit is more an API limit than an
 * actual system limit.
 **/
constexpr size_t NAME = NAME_MAX;

}

}

#endif // inc. guard
