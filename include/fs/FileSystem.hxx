#ifndef COSMOS_FILESYSTEM_HXX
#define COSMOS_FILESYSTEM_HXX

// C++ stdlib
#include <optional>
#include <string>
#include <string_view>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/fs/types.hxx"

namespace cosmos::fs {

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
 *
 * This function does not determine the file *type* i.e. it could be also a
 * directory, socket etc.
 **/
COSMOS_API bool existsFile(const std::string_view path);

/// Removes the file object found at \c path
/**
 * This function removes the file object found at \c path from the file
 * system. Processes that already have opened the file can continue using it,
 * but the name is removed from the file system.
 *
 * This call does not work with directories, use removeDir() for them instead.
 *
 * If the \c path does not exist then this is considered an error and an
 * exception is thrown.
 **/
COSMOS_API void unlinkFile(const std::string_view path);

/// Change the calling process's current working directory to \c path
/**
 * On error an ApiError exception is thrown.
 **/
COSMOS_API void changeDir(const std::string_view path);

/// Returns the process's current working directory
/**
 * This call can fail e.g. on out of memory conditions or if the CWD has been
 * unlinked. An ApiError is thrown in such cases.
 **/
COSMOS_API std::string getWorkingDir();

/// Find the full path to the executable program \c exec_base
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

/// Creates a directory at the given location
/**
 * This attempts to create a single new directory at the given \c path. The
 * parent directory must already exist, otherwise an ApiError is thrown.
 *
 * The given \c mode determines the permissions of the newly created
 * directory. The permissions are modified by the process's umask
 * (mode = mode * & ~umask)
 **/
COSMOS_API void makeDir(const std::string_view path, const FileMode mode);

/// Removes an empty directory at the given location
/**
 * The directory must exist and must be empty for the call to succeed. On
 * error an ApiError is thrown.
 **/
COSMOS_API void removeDir(const std::string_view path);

/// Creates a directory, potentially creating multiple directory components
/**
 * This is similar to makeDir() but also creates possibly missing parent
 * directory elements in \c path. All created directory components will
 * receive the given \c mode.
 *
 * If any of the path components cannot be created (or accessed) then an
 * ApiError is thrown. This could mean that some of the paths have been
 * created in the error case, but not the full path.
 *
 * TODO: this does not currently normalize the input path which can lead to
 * unnecessary directories being created like when using path
 * "/some/ugly/../dir", then "/some/ugly" would also be created. This needs a
 * normlization helper.
 *
 * \return Errno::NO_ERROR if the full path (and thus at least the final
 *         directory component) was created, Errno::EXISTS if the directory
 *         was already existing.
 **/
COSMOS_API Errno makeAllDirs(const std::string_view path, const FileMode mode);

/// Recursively removes all directory content in \c path
/**
 * This function recursively removes all content in \c path. It is expected
 * that as least \c path itself exists and is a directory, otherwise an
 * exception is thrown.
 *
 * If an error occurs while removing any of the descendant path elements then
 * an ApiError is thrown and the state of the directory tree is undefined.
 * Note that using this function with concurrently file system access from
 * within the calling or another process in the system can cause race
 * conditions that leads to undefined behaviour.
 **/
COSMOS_API void removeTree(const std::string_view path);

} // end ns

#endif // inc. guard
