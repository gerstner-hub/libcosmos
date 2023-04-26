#ifndef COSMOS_FILESYSTEM_HXX
#define COSMOS_FILESYSTEM_HXX

// C++
#include <optional>
#include <string>
#include <string_view>
#include <utility>

// cosmos
#include "cosmos/error/errno.hxx"
#include "cosmos/dso_export.h"
#include "cosmos/fs/DirFD.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/fs/types.hxx"

/**
 * @file
 *
 * File system related system calls. These is the lowest and most generic
 * level of file system APIs that can be wrapped further in more comfortable
 * types to use.
 **/

namespace cosmos::fs {

/// Open a file using specific OpenFlags, potentially creating it first using the given \c fmode.
/**
 * \warning If used for creating a file, then you need to specify
 * also the FileMode in that case. An exception will the thrown if
 * this condition is violated.
 *
 * The returned FileDescriptor object does not manage the lifetime of the file
 * descriptor, you have to take care of closing it at the appropriate time
 * yourself!
 **/
COSMOS_API FileDescriptor open(
		const std::string_view path, const OpenMode mode,
		const OpenFlags flags, const std::optional<FileMode> fmode = {});

/// Open the given path relative to the given directory file descriptor \c dir_fd.
/**
 * This open variant behaves similar to open(const std::string_view,
 * const OpenMode, const OpenFlags, const std::optional<FileMode>).
 * The following differences exist:
 *
 * - if \c path is an absolute path then \c dir_fd is ignored and the
 *   behaviour is identical to the other open variants.
 * - if \c path is a relative path and \c dir_fd is an invalid file
 *   descriptor then the open fails (this can explicitly be used to
 *   enforce absolute path specifications.
 * - if \c path is a relative path and \c dir_fd is a valid directory file
 *   descriptor, then the path is looked up relative to \c dir_fd. \c dir_fd
 *   needs to be opened with OpenMode::READ_ONLY or with
 *   OpenSettings::PATH. The special DirFD value cosmos::AT_CWD can be
 *   used to open files relative to the current working directory.
 **/
COSMOS_API FileDescriptor open_at(
		const DirFD dir_fd, const std::string_view path,
		const OpenMode mode, const OpenFlags flags,
		const std::optional<FileMode> fmode = {});

/// Safely create a temporary file and return it's file descriptor and path.
/**
 * \c _template needs to be a template for the path to use for the temporary
 * file. It can be an absolute or a relative path. The path determines the
 * directory where the temporary file will be created in. The template also
 * needs to contain a basename on which the actual file path will be based.
 * You can place a pair of "{}" in the basename to mark the position in the
 * path where a unique random still will be inserted. The last occurence of
 * "{}" will be used for this. If no such substring is found in the basename
 * then the unique random still will be added as a suffix to the basename.
 *
 * It is an error to use a zero-length basename which will cause an exception
 * to be thrown.
 *
 * The \c flags argument can specify optional additional open flags to be
 * applied when opening the temporary file. The implementation will implicitly
 * use OpenMode::READ_WRITE and OpenSettings::CREATE and
 * OpenSettings::EXCLUSIVE. These should *not* be set in \c flags. The file
 * will have the permissions ModeT{0600}.
 *
 * On success this call returns a pair consisting of the newly opened file
 * descriptor corresponding to the temporary file, and the expanded filename
 * under which the temporary file has been created.
 *
 * It is the caller's responsibility to correctly close the returned
 * FileDescriptor and to delete the temporary file (using unlink_file()) once
 * it is no longer needed. The cosmos::TempFile class takes care of these
 * tasks, if necessary.
 **/
COSMOS_API std::pair<FileDescriptor, std::string> make_tempfile(
		const std::string_view _template, const OpenFlags flags = OpenFlags{});

/// Safely create a temporary directory and return it's path.
/**
 * This is similar to make_tempfile(). \c _template is the relative or
 * absolute path to use as a template for the to be created directory. The
 * basename of the path will be expanded with a random string. An empty
 * basename is not allowed.
 *
 * The created directory will receive a mode of 0700.
 *
 * The returned string contains the expanded \c _template. It is the caller's
 * responsibility to remove the directory from the file system again at the
 * appropriate time.
 *
 * \see TempDir for a convenience type that transparently handles removal of
 * the temporary directory.
 **/
COSMOS_API std::string make_tempdir(const std::string_view _template);

/// Sets the process's file creation mask.
/**
 * The file creation mask is a process wide attribute that determines an upper
 * limit of the file permission bits that are set on newly created files and
 * directories. Most prominently this affects files created via open() and
 * directories created via mkdir(). Even if more open permissions are
 * specified in these system calls, the bits will be switched off if they are
 * also present in the process's umask. The file permission bits are
 * calculated as (<mode> & ~umask) i.e. bits that are set in the umask will be
 * set to zero during file creation.
 *
 * Since this is a process wide attribute it will affect all threads in the
 * process and can thus cause race conditions. If necessary you should the the
 * umask in the main thread of a program early on.
 *
 * Only the lower 9 bits of \c mode will be taken into account (i.e.
 * owner/group/other permissions bits). If any other bits are set then an
 * UsageError exception is thrown.
 *
 * The umask is inherited across fork() and is not changed during execve().
 *
 * Other system calls that make use of the umask are the creation of POSIX IPC
 * objects (message queues, semaphores, shared memory), named pipes, and UNIX
 * domain sockets. It is *not* used by SYSV IPC objects.
 *
 * \return The umask that was previously in effect. To only read the current
 * process's umask you need to read the proc file system in /proc/<pid>/status
 * (umask is available there since Linux 4.7).
 **/
COSMOS_API FileMode set_umask(const FileMode mode);

/// Returns whether the given file system object exists.
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
COSMOS_API bool exists_file(const std::string_view path);

/// Removes the file object found at \c path.
/**
 * This function removes the file object found at \c path from the file
 * system. Processes that already have opened the file can continue using it,
 * but the name is removed from the file system.
 *
 * This call does not work with directories, use remove_dir() for them instead.
 *
 * If the \c path does not exist then this is considered an error and an
 * exception is thrown.
 **/
COSMOS_API void unlink_file(const std::string_view path);

/// Removes the file object found at \c path relative to \c dir_fd.
/**
 * This behaves similar to unlink_file():
 *
 * - if path is an absolute path then \c dir_fd is ignored and the call is
 *   equivalent to unlink_file().
 * - if path is a relative path and \c dir_fd has the special value
 *   cosmos::AT_CWD, then \c path is looked up relative to the current working
 *   directory, equivalent to unlink_file().
 * - if path is a relative path and \c dir_fd is a valid open directory file
 *   descriptor then \c path is looked up relative to that directory.
 **/
COSMOS_API void unlink_file_at(const DirFD dir_fd, const std::string_view path);

/// Change the calling process's current working directory to \c path.
/**
 * On error a FileError exception is thrown.
 **/
COSMOS_API void change_dir(const std::string_view path);

/// Returns the process's current working directory.
/**
 * This call can fail e.g. on out of memory conditions or if the CWD has been
 * unlinked. An ApiError is thrown in such cases.
 **/
COSMOS_API std::string get_working_dir();

/// Find the full path to the executable program \c exec_base.
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

/// Creates a directory at the given location.
/**
 * This attempts to create a single new directory at the given \c path. The
 * parent directory must already exist, otherwise a FileError is thrown.
 *
 * The given \c mode determines the permissions of the newly created
 * directory. The permissions are modified by the process's umask
 * (mode = mode * & ~umask)
 **/
COSMOS_API void make_dir(const std::string_view path, const FileMode mode);

/// Creates a directory at the location relative to \c dir_fd.
/**
 * This relates to make_dir() the same way unlink_file_at() relates to
 * unlink_file().
 *
 * \see unlink_file_at().
 **/
COSMOS_API void make_dir_at(const DirFD dir_fd, const std::string_view path, const FileMode mode);

/// Removes an empty directory at the given location.
/**
 * The directory must exist and must be empty for the call to succeed. On
 * error a FileError is thrown.
 **/
COSMOS_API void remove_dir(const std::string_view path);

/// Removes an empty directory relative to \c dir_fd.
/**
 * This relates to remove_dir() the same way unlink_file_at() relates to
 * unlink_file().
 *
 * \see unlink_file_at().
 **/
COSMOS_API void remove_dir_at(const DirFD dir_fd, const std::string_view path);

/// Creates a directory, potentially creating multiple directory components.
/**
 * This is similar to make_dir() but also creates possibly missing parent
 * directory elements in \c path. All created directory components will
 * receive the given \c mode.
 *
 * If any of the path components cannot be created (or accessed) then an
 * FileError is thrown. This could mean that some of the paths have been
 * created in the error case, but not the full path.
 *
 * \return Errno::NO_ERROR if the full path (and thus at least the final
 *         directory component) was created, Errno::EXISTS if the directory
 *         was already existing.
 **/
COSMOS_API Errno make_all_dirs(const std::string_view path, const FileMode mode);

/// Recursively removes all directory content in \c path.
/**
 * This function recursively removes all content in \c path. It is expected
 * that as least \c path itself exists and is a directory, otherwise an
 * exception is thrown.
 *
 * If an error occurs while removing any of the descendant path elements then
 * a FileError is thrown and the state of the directory tree is undefined.
 * Note that using this function with concurrently file system access from
 * within the calling or another process in the system can cause race
 * conditions that leads to undefined behaviour.
 **/
COSMOS_API void remove_tree(const std::string_view path);

/// Changes the FileMode of the given path.
/**
 * Attempts to change the FileMode associated with the file object found at
 * the given path. Symlinks will be followed.
 *
 * To change the mode of symlinks use change_owner_nofollow(). To avoid symlinks
 * use File::open() with OpenSettings::NOFOLLOW. Obtain a FileStatus() for the
 * open FileDescriptor and then use change_mode(FileDescriptor, FileMode)
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
COSMOS_API void change_mode(const std::string_view path, const FileMode mode);

/// Changes the FileMode of the given open file descriptor.
/**
 * This behaves the same as change_mode(std::string_view, FileMode) with the
 * exception that the file associated with the given file descriptor will be
 * affected instead of a to-be-opened path.
 *
 * Additional common errors that can occur:
 *
 * - bad file descriptor (Errno::BAD_FD)
 **/
COSMOS_API void change_mode(const FileDescriptor fd, const FileMode mode);

/// Change numerical owner and/or group ID of a file path.
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
COSMOS_API void change_owner(const std::string_view path, const UserID uid, const GroupID gid = GroupID::INVALID);

/// Change numerical owner and/or group ID of the given open file descriptor.
/**
 * This behaves the same as change_owner(std::string_view, UserID, GroupID)
 * with the exception that the file associated with the given file descriptor
 * will be affected instead of a to-be-opened path.
 *
 * Additional common errors that can occur:
 *
 * - bad file descriptor (Errno::BAD_FD)
 **/
COSMOS_API void change_owner(const FileDescriptor fd, const UserID uid, const GroupID gid = GroupID::INVALID);

/// Change owner and/or group of the given path by user name and/or group name.
/**
 * This is a convenience function on top of change_owner(std::string_view,
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
COSMOS_API void change_owner(const std::string_view path, const std::string_view user,
		const std::string_view group = {});

/// Change owner and/or group of the given file descriptor by user name and/or group name.
/**
 * This is a convenience function on top of change_owner(FileDescriptor,
 * UserID, GroupID). The description of change_owner(std::string_view,
 * std::string_view, std::string_view) applies here as well.
 **/
COSMOS_API void change_owner(const FileDescriptor fd, const std::string_view user,
		const std::string_view group = {});

/// Convenience wrapper of change_owner() to change only the group of a file.
inline void change_group(const FileDescriptor fd, const GroupID id) {
	change_owner(fd, UserID::INVALID, id);
}

/// Convenience wrapper of change_owner() to change only the group of a file.
inline void change_group(const FileDescriptor fd, const std::string_view group) {
	change_owner(fd, {}, group);
}

/// Convenience wrapper of change_owner() to change only the group of a file.
inline void change_group(const std::string_view path, const GroupID id) {
	change_owner(path, UserID::INVALID, id);
}

/// Convenience wrapper of change_owner() to change only the group of a file.
inline void change_group(const std::string_view path, const std::string_view group) {
	change_owner(path, {}, group);
}

/// Changes owner and/or group of the given path while not following symlinks.
/**
 * This behaves the same as change_owner(const std::string_view, const UserID,
 * const GroupID) with the exception that if the final path component refers
 * to a symbolic link that the ownership of the link is changed, not that of
 * the target.
 *
 * If \c path is not a symlink then it's owner will still be changed and no
 * error is thrown.
 **/
COSMOS_API void change_owner_nofollow(const std::string_view path, const UserID uid,
		const GroupID gid = GroupID::INVALID);

/// Changes owner and/or group of the given path while not following symlinks.
/**
 * \see change_owner_nofollow(const std::string_view, const UserID, const GroupID)
 **/
COSMOS_API void change_owner_nofollow(const std::string_view path, const std::string_view user,
		const std::string_view group = {});

/// Creates a symbolic link at \c path pointing to \c target.
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
COSMOS_API void make_symlink(const std::string_view target, const std::string_view path);

/// Creates a symbolic link relative to \c dir_fd pointing to \c target.
/**
 * This behaves just like make_symlink(). It relates to make_symlink() the
 * same way unlink_file_at() relates to unlink_file().
 **/
COSMOS_API void make_symlink_at(const std::string_view target, const DirFD dir_fd,
		const std::string_view path);

/// Returns the target (content) of the symbolic link at \c path.
/**
 * This returns the target path of the symlink present at the given \c path.
 *
 * If an error occurs then a FileError is thrown. Common errors are:
 *
 * - access denied (Errno::ACCESS)
 * - invalid argument if \c path is not a symlink (Errno::INVALID_ARG)
 * - no entry if \c path does not exist (Errno::NO_ENTRY)
 **/
COSMOS_API std::string read_symlink(const std::string_view path);

/// Returns the target (content) of the symbolic link \c path relative to \c dir_fd.
/**
 * This relates to read_symlink() the same way unlink_file_at() relates to
 * unlink_file().
 *
 * \see unlink_file_at().
 **/
COSMOS_API std::string read_symlink_at(const DirFD dir_fd, const std::string_view path);

/// Creates a new (hard) link of the file found in \c old_path at \c new_path.
/**
 * Hard links only work on the same file system. An attempt to create a hard
 * link across different mounts will result in an ApiError with
 * Errno::CROSS_DEVICE.
 *
 * If \c new_path already exists then it will not be overwritten but
 * Errno::EXISTS will be thrown.
 *
 * Hard links don't work for directories, if \c old_path refers to one then
 * Errno::PERMISSION will be thrown.
 *
 * Furthermore a range of errors related to lack of permissions, lack of
 * memory, lack of disk space, problems in path resolution and so on can
 * occur.
 *
 * On success both names will refer to the same file and it cannot be
 * determined any more which was was the "original".
 **/
COSMOS_API void link(const std::string_view old_path, const std::string_view new_path);

/// Creates a new (hard) link based on lookups relative to \c old_dir and \c new_dir.
/**
 * This behaves similar to link(). For \c old_path and \c new_path the usual
 * at() API rules apply:
 *
 * - if the path is absolute then related DirFD is ignored.
 * - else if the DirFD has the special value cosmos::AT_CWD then path is
 *   interpreted relative to the current working directory.
 * - else the path is interpreted relative to the directory represented by
 *   DirFD.
 *
 * \c follow_old determines whether symlink's encountered at \c old_path will
 * be resolved or not.
 **/
COSMOS_API void linkat(const DirFD old_dir, const std::string_view old_path,
		const DirFD new_dir, const std::string_view new_path,
		const FollowSymlinks follow_old = FollowSymlinks{false});

/// Special variant of linkat() that can link arbitrary file descriptors at a new location.
/**
 * Contrary to linkat() this call can give the specified file descriptor a new
 * name at \c new_dir / \c new_path, without specifying a source name. This
 * generally only works for files that have a non-zero link count. This does
 * not work for directory file descriptors.
 *
 * As a special case this *does* work for file descriptors opened with
 * OpenSettings::PATH and for temporary files opened with
 * OpenSettings::TMPFILE but without OpenSettings::EXCLUSIVE (in this case a
 * link count of 0 is accepted). Note, however, that this call requires the
 * CAP_DAC_READ_SEARCH capability, for security reasons.
 *
 * An alternative is to use regular linkat() and the /proc file system, see
 * `man 2 linkat`.
 **/
COSMOS_API void linkat_fd(const FileDescriptor fd, const DirFD new_dir,
		const std::string_view new_path);

/// Performs the same as linkat_fd() using linkat() and the /proc file system.
/**
 * To avoid the permission issues that linkat_fd() has this variant of the
 * linkat call uses a workaround based on the /proc file system to achieve the
 * same result, see `man 2 linkat`.
 **/
COSMOS_API void linkat_proc_fd(const FileDescriptor fd, const DirFD new_dir,
		const std::string_view new_path);

} // end ns

#endif // inc. guard
