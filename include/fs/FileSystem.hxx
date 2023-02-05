#ifndef COSMOS_FILESYSTEM_HXX
#define COSMOS_FILESYSTEM_HXX

// C++ stdlib
#include <optional>
#include <string>
#include <string_view>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/types.hxx"

namespace cosmos {

/// Strong boolean type to enable following of symlinks in the file system
using FollowSymlinks = NamedBool<struct follow_links_t, false>;

namespace fs {

/// Returns whether the given file system object exists
/**
 * The information returned is only a snapshot in time. It is subject
 * to race conditions i.e. when trying to open the file afterwards it
 * may already be gone or have been replaced by a different file
 * system object.
 *
 * For safely testing for existence and opening a file, you should
 * open the file right away and test for an according error condition.
 *
 * This function will not follow symlinks i.e. if \c path refers to a
 * dangling symlink then it will still return \c true.
 *
 * If the condition cannot be exactly determined, because an error
 * different than "ENOENT" is returned by the operating system then an
 * exception is thrown.
 **/
COSMOS_API bool existsFile(const std::string_view path);

/// change the calling process's current working directory to \c path
/**
 * On error an ApiError exception is thrown.
 **/
COSMOS_API void changeDir(const std::string_view path);

/// returns the process's current working directory
/**
 * This call can fail e.g. on out of memory conditions or if the CWD has been
 * unlinked. An ApiError is thrown in such cases.
 **/
COSMOS_API std::string getWorkingDir();

/// find the full path to the executable program \c exec_base
/**
 * This function looks in all directories listed in the PATH environment
 * variable for an executable named \c exec_base. If one is found then the
 * full path to this executable is returned.
 *
 * If \c exec_base is an absolute path then it is only checked whether the
 * path (symlinks are followed) is accessible and executable and returns it
 * unmodified if this is the case.
 *
 * If the program cannot be found or is not accessible then {} is returned.
 * This fucntion does not throw exceptions, on error {} is returned.
 **/
COSMOS_API std::optional<std::string> which(const std::string_view exec_base) noexcept;

} // end ns
} // end ns

#endif // inc. guard
