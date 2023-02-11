#ifndef COSMOS_FILESYSTEM_HXX
#define COSMOS_FILESYSTEM_HXX

// C++ stdlib
#include <optional>
#include <string>
#include <string_view>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
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

/// Changes the FileMode of the given path
/**
 * Attempts to change the FileMode associated with the file object found at
 * the given path. Symlinks will be followed.
 *
 * To change the mode of symlinks use linkChangeOwner(). To avoid symlinks
 * use File::open() with OpenSettings::NOFOLLOW. Obtain a FileStatus() for the
 * open FileDescriptor and then use changeMode(FileDescriptor, FileMode)
 * if the file isn't a symlink.
 *
 * \note On Linux there is no way to change the mode symlinks.
 *
 * Various errors can occur which will cause a FileError to be thrown. The
 * most common errors are:
 *
 * - file does not exist (Errno::NO_ENTRY)
 * - no permission to change the mode (Errno::PERMISSION)
 * - read only file system (Errno::READ_ONLY_FS)
 **/
COSMOS_API void changeMode(const std::string_view path, const FileMode mode);

/// Changes the FileMode of the given open file descriptor
/**
 * This behaves the same as changeMode(std::string_view, FileMode) with the
 * exception that the file associated with the given file descriptor will be
 * affected instead of a to-be-opened path.
 *
 * Additional common errors that can occur:
 *
 * - bad file descriptor (Errno::BAD_FD)
 **/
COSMOS_API void changeMode(const FileDescriptor fd, const FileMode mode);

/// Change numerical owner and/or group ID of a file path
/**
 * Attempts to change the owner and/or the group of the file object found at
 * \c path. If \c path refers to a symbolic link then the target of the link
 * is affected. UserID::INVALID and GroupID::INVALID will cause the respective
 * item *not* to be touched.
 *
 * Various errors can occur which will cause a FileError to be thrown. The
 * most common errors are:
 *
 * - file does not exist (Errno::NO_ENTRY)
 * - no permission to change the owner (Errno::PERMISSION)
 * - read only file system (Errno::READ_ONLY_FS)
 **/
COSMOS_API void changeOwner(const std::string_view path, const UserID uid, const GroupID gid = GroupID::INVALID);

/// Change numerical owner and/or group ID of the given open file descriptor
/**
 * This behaves the same as changeOwner(std::string_view, UserID, GroupID)
 * with the exception that the file associated with the given file descriptor
 * will be affected instead of a to-be-opened path.
 *
 * Additional common errors that can occur:
 *
 * - bad file descriptor (Errno::BAD_FD)
 **/
COSMOS_API void changeOwner(const FileDescriptor fd, const UserID uid, const GroupID gid = GroupID::INVALID);

/// Change owner and/or group of the given path by user name and/or group name
/**
 * This is a convenience function on top of changeOwner(std::string_view,
 * UserID, GroupID). It looks up the numerical UserID of \c user and the
 * numerical GroupID of \c group.
 *
 * To skip changing the owner of user or group simply pass an empty
 * string_view as respective parameter.
 *
 * Additional errors that can occur are the ones described in PasswdInfo(),
 * GroupInfo() and RuntimeError() in case the username or group do not
 * exist.
 **/
COSMOS_API void changeOwner(const std::string_view path, const std::string_view user,
		const std::string_view group = {});

/// Change owner and/or group of the given file descriptor by user name and/or group name
/**
 * This is a convenience function on top of changeOwner(FileDescriptor,
 * UserID, GroupID). The description of changeOwner(std::string_view,
 * std::string_view, std::string_view) applies here as well.
 **/
COSMOS_API void changeOwner(const FileDescriptor fd, const std::string_view user,
		const std::string_view group = {});

/// Convenience wrapper of changeOwner() to change only the group of a file
inline void changeGroup(const FileDescriptor fd, const GroupID id) {
	changeOwner(fd, UserID::INVALID, id);
}

/// Convenience wrapper of changeOwnner() to change only the group of a file
inline void changeGroup(const FileDescriptor fd, const std::string_view group) {
	changeOwner(fd, {}, group);
}

/// Convenience wrapper of changeOwnner() to change only the group of a file
inline void changeGroup(const std::string_view path, const GroupID id) {
	changeOwner(path, UserID::INVALID, id);
}

/// Convenience wrapper of changeOwnner() to change only the group of a file
inline void changeGroup(const std::string_view path, const std::string_view group) {
	changeOwner(path, {}, group);
}

/// Changes owner and/or group of the given path while not following symlinks
/**
 * This behaves the same as changeOwner(const std::string_view, const UserID,
 * const GroupID) with the exception that if the final path component refers
 * to a symbolic link that the ownership of the link is changed, not that of
 * the target.
 *
 * If \c path is not a symlink then it's owner will still be changed and no
 * error is thrown.
 **/
COSMOS_API void linkChangeOwner(const std::string_view path, const UserID uid,
		const GroupID gid = GroupID::INVALID);

/// Changes owner and/or group of the given path while not following symlinks
/**
 * \see linkChangeOwner(const std::string_view, const UserID, const GroupID)
 **/
COSMOS_API void linkChangeOwner(const std::string_view path, const std::string_view user,
		const std::string_view group = {});

/// Creates a symbolic link at \c path pointing to \c target
/**
 * A symbolic link is simply a pointer to another file system location as
 * provided in \c target. Leading ".." components in \c target will refer to
 * the parent directories of \c path.
 *
 * The permission bits (FileMode) of a symlink are ignored by Linux. The
 * user and group ownership are ignored when dereferencing a symlink, except
 * for when the protected_symlinks feature is enabled (see proc(5); the
 * feature is enabled on most Linux systems these days). The ownership is also
 * relevant for renaming or removing a symlink.
 *
 * If an error occurs then a FileError is thrown. Common errors are:
 *
 * - access denied to \c path. Either search access in the initial components
 *   or write access to the final directory component (Errno::ACCESS).
 * - the file system does not allow the creation of symbolic links
 *   (Errno::PERMISSION).
 * - Earlier parts of \c path do not exist or \c target is empty
 *   (Errno::NO_ENTRY).
 * - \c path already exists (Errno::EXISTS).
 **/
COSMOS_API void makeSymlink(const std::string_view target, const std::string_view path);

/// Returns the target (content) of the symbolic at \c path.
/**
 * This returns the target path of the symlink present at the given \c path.
 *
 * If an error occurs then a FileError is thrown. Common errors are:
 *
 * - access denied (Errno::ACCESS)
 * - invalid argument if \c path is not a symlink (Errno::INVALID_ARG)
 * - no entry if \c path does not exist (Errno::NO_ENTRY)
 **/
COSMOS_API std::string readSymlink(const std::string_view path);

} // end ns

#endif // inc. guard
