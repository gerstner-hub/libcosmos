#pragma once

// Linux
#include <linux/close_range.h>

// C++
#include <optional>
#include <string>
#include <string_view>
#include <utility>

// cosmos
#include <cosmos/SysString.hxx>
#include <cosmos/dso_export.h>
#include <cosmos/error/errno.hxx>
#include <cosmos/fs/DirFD.hxx>
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/fs/types.hxx>
#include <cosmos/types.hxx>

/**
 * @file
 *
 * File system related system calls. These is the lowest and most generic
 * level of file system APIs that can be wrapped further in more comfortable
 * types to use.
 **/

namespace cosmos::fs {

/// Open a file using specific OpenFlags, potentially creating it first using the given `fmode`.
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
		const SysString path, const OpenMode mode,
		const OpenFlags flags, const std::optional<FileMode> fmode = {});

/// Open the given path relative to the given directory file descriptor `dir_fd`.
/**
 * This open variant behaves similar to open(const SysString,,
 * const OpenMode, const OpenFlags, const std::optional<FileMode>).
 * The following differences exist:
 *
 * - if `path` is an absolute path then `dir_fd` is ignored and the
 *   behaviour is identical to the other open variants.
 * - if `path` is a relative path and `dir_fd` is an invalid file
 *   descriptor then the open fails (this can explicitly be used to
 *   enforce absolute path specifications.
 * - if `path` is a relative path and `dir_fd` is a valid directory file
 *   descriptor, then the path is looked up relative to `dir_fd`. `dir_fd`
 *   needs to be opened with OpenMode::READ_ONLY or with
 *   OpenFlag::PATH. The special DirFD value cosmos::AT_CWD can be
 *   used to open files relative to the current working directory.
 **/
COSMOS_API FileDescriptor open_at(
		const DirFD dir_fd, const SysString path,
		const OpenMode mode, const OpenFlags flags,
		const std::optional<FileMode> fmode = {});

enum class CloseRangeFlag : unsigned int {
	/// Instead of closing, mark all matching file descriptors as CLOEXEC
	CLOEXEC = CLOSE_RANGE_CLOEXEC,
	/// Unshare specified file descriptors before closing to avoid race conditions with other threads.
	UNSHARE = CLOSE_RANGE_UNSHARE
};

/// Flags used in cosmos::fs::close_range().
using CloseRangeFlags = BitMask<CloseRangeFlag>;

/// Close a range of file descriptor numbers in the current process.
/**
 * This call closes all file descriptor numbers in the range [first, last],
 * where last is included in the range.
 *
 * This is mostly useful before executing a child process to get rid of any
 * file descriptors not marked with the close-on-exec file descriptor flag.
 * The call is a lot more efficient then a loop in userspace calling `close()`
 * on every single file descriptor.
 *
 * You can specify FileNum::MAX_FD for `last` if you want to close all file
 * descriptors starting from a given range. This is also the default for `last`.
 *
 * This system call can fail on out of memory conditions or if the maximum
 * number of file descriptors is exceeded in combination with
 * CloseRangeFlag::UNSHARE.
 **/
COSMOS_API void close_range(const FileNum first,
		const FileNum last = FileNum::MAX_FD,
		const CloseRangeFlags flags = CloseRangeFlags{});

/// Safely create a temporary file and return it's file descriptor and path.
/**
 * `_template` needs to be a template for the path to use for the temporary
 * file. It can be an absolute or a relative path. The path determines the
 * directory where the temporary file will be created in. The template also
 * needs to contain a basename on which the actual file path will be based.
 * You can place a pair of "{}" in the basename to mark the position in the
 * path where a unique random still will be inserted. The last occurrence of
 * "{}" will be used for this. If no such substring is found in the basename
 * then the unique random still will be added as a suffix to the basename.
 *
 * It is an error to use a zero-length basename which will cause an exception
 * to be thrown.
 *
 * The `flags` argument can specify optional additional open flags to be
 * applied when opening the temporary file. The implementation will implicitly
 * use OpenMode::READ_WRITE and OpenFlag::CREATE and
 * OpenFlag::EXCLUSIVE. These should *not* be set in `flags`. The file
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
		const SysString _template, const OpenFlags flags = OpenFlags{OpenFlag::CLOEXEC});

/// Safely create a temporary directory and return it's path.
/**
 * This is similar to make_tempfile(). `_template` is the relative or
 * absolute path to use as a template for the to be created directory. The
 * basename of the path will be expanded with a random string. An empty
 * basename is not allowed.
 *
 * The created directory will receive a mode of 0700.
 *
 * The returned string contains the expanded `_template`. It is the caller's
 * responsibility to remove the directory from the file system again at the
 * appropriate time.
 *
 * \see TempDir for a convenience type that transparently handles removal of
 * the temporary directory.
 **/
COSMOS_API std::string make_tempdir(const SysString _template);

/// Creates a named pipe at the given file system location.
/**
 * A named pipe is identical to a cosmos::Pipe, only that is has a visible
 * name in the file system. Unrelated processes can exchange data via the pipe
 * this way.
 *
 * The file system entry carries the usual UNIX permissions that will be
 * initialized by combining `mode` with the calling process's umask.
 *
 * Opening named pipes will block both for reading and writing as long as no
 * communication partner exists. When opening a named pipe in non-blocking
 * mode will succeed for OpenMode::READ_ONLY even if no writer is present yet.
 * For OpenMode::WRITE_ONLY an error Errno::NXIO is returned in the
 * non-blocking case. As a special case on Linux when opening the pipe with
 * OpenMode::READ_WRITE will succeed regardless of blocking or non-blocking
 * mode. This allows to open the pipe for writing while there are no readers
 * available.
 *
 * The underlying system call for this is `mknod()` which will never follow
 * symlinks or reuse existing files. Instead an error is thrown if the path
 * already exists in any form.
 **/
COSMOS_API void make_fifo(const SysString path, const FileMode mode);

/// Creates a named pipe relative to the given directory descriptor.
/**
 * This behaves like make_fifo with the usual _at semantics:
 *
 * - if `path` is absolute then `dir_fd` is ignored.
 * - if `path` is relative then it is interpreted relative to `dir_fd`.
 * - if `path` is relative and `dir_fd` has the special value cosmos::AT_CWD
 *   then it is interpreted relative to the current working directory.
 **/
COSMOS_API void make_fifo_at(const DirFD dir_fd, const SysString path,
		const FileMode mode);

/// Sets the process's file creation mask.
/**
 * The file creation mask is a process wide attribute that determines an upper
 * limit of the file permission bits that are set on newly created files and
 * directories. Most prominently this affects files created via open() and
 * directories created via mkdir(). Even if more open permissions are
 * specified in these system calls, the bits will be switched off if they are
 * also present in the process's umask. The file permission bits are
 * calculated as (`<mode> & ~umask`) i.e. bits that are set in the umask will be
 * set to zero during file creation.
 *
 * Since this is a process wide attribute it will affect all threads in the
 * process and can thus cause race conditions. If necessary, you should set the
 * umask in the main thread of a program early on.
 *
 * Only the lower 9 bits of `mode` will be taken into account (i.e.
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
 * process's umask you need to read the proc file system in `/proc/<pid>/status`
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
 * This function will not follow symlinks i.e. if `path` refers to a
 * dangling symlink then it will still return `true`.
 *
 * If the condition cannot be exactly determined, because an error
 * different than "ENOENT" is returned by the operating system then an
 * exception is thrown.
 *
 * This function does not determine the file *type* i.e. it could be also a
 * directory, socket etc.
 **/
COSMOS_API bool exists_file(const SysString path);

/// Removes the file object found at `path`.
/**
 * This function removes the file object found at `path` from the file
 * system. Processes that already have opened the file can continue using it,
 * but the name is removed from the file system.
 *
 * This call does not work with directories, use remove_dir() for them instead.
 *
 * If the `path` does not exist then this is considered an error and an
 * exception is thrown.
 **/
COSMOS_API void unlink_file(const SysString path);

/// Removes the file object found at `path` relative to `dir_fd`.
/**
 * This behaves similar to unlink_file():
 *
 * - if path is an absolute path then `dir_fd` is ignored and the call is
 *   equivalent to unlink_file().
 * - if path is a relative path and `dir_fd` has the special value
 *   cosmos::AT_CWD, then `path` is looked up relative to the current working
 *   directory, equivalent to unlink_file().
 * - if path is a relative path and `dir_fd` is a valid open directory file
 *   descriptor then `path` is looked up relative to that directory.
 **/
COSMOS_API void unlink_file_at(const DirFD dir_fd, const SysString path);

/// Change the calling process's current working directory to `path`.
/**
 * On error a FileError exception is thrown.
 **/
COSMOS_API void change_dir(const SysString path);

/// Returns the process's current working directory.
/**
 * This call can fail e.g. on out of memory conditions or if the CWD has been
 * unlinked. An ApiError is thrown in such cases.
 **/
COSMOS_API std::string get_working_dir();

/// Find the full path to the executable program `exec_base`.
/**
 * This function looks in all directories listed in the PATH environment
 * variable for an executable named `exec_base`. If one is found then the
 * full path to this executable is returned.
 *
 * If `exec_base` contains path separators then it is only checked whether the
 * path (symlinks are followed) is accessible and executable and returns it
 * unmodified if this is the case.
 *
 * If the program cannot be found or is not accessible then {} is returned.
 * This function does not throw exceptions, on error {} is returned.
 **/
COSMOS_API std::optional<std::string> which(const std::string_view exec_base) noexcept;

/// Creates a directory at the given location.
/**
 * This attempts to create a single new directory at the given `path`. The
 * parent directory must already exist, otherwise a FileError is thrown.
 *
 * The given `mode` determines the permissions of the newly created
 * directory. The permissions are modified by the process's umask
 * (mode = mode * & ~umask)
 **/
COSMOS_API void make_dir(const SysString path, const FileMode mode);

/// Creates a directory at the location relative to `dir_fd`.
/**
 * This relates to make_dir() the same way unlink_file_at() relates to
 * unlink_file().
 *
 * \see unlink_file_at().
 **/
COSMOS_API void make_dir_at(const DirFD dir_fd, const SysString path, const FileMode mode);

/// Removes an empty directory at the given location.
/**
 * The directory must exist and must be empty for the call to succeed. On
 * error a FileError is thrown.
 **/
COSMOS_API void remove_dir(const SysString path);

/// Removes an empty directory relative to `dir_fd`.
/**
 * This relates to remove_dir() the same way unlink_file_at() relates to
 * unlink_file().
 *
 * \see unlink_file_at().
 **/
COSMOS_API void remove_dir_at(const DirFD dir_fd, const SysString path);

/// Creates a directory, potentially creating multiple directory components.
/**
 * This is similar to make_dir() but also creates possibly missing parent
 * directory elements in `path`. All created directory components will
 * receive the given `mode`.
 *
 * If any of the path components cannot be created (or accessed) then an
 * FileError is thrown. This could mean that some of the paths have been
 * created in the error case, but not the full path.
 *
 * \return Errno::NO_ERROR if the full path (and thus at least the final
 *         directory component) was created, Errno::EXISTS if the directory
 *         was already existing.
 **/
COSMOS_API Errno make_all_dirs(const SysString path, const FileMode mode);

/// Recursively removes all directory content in `path`.
/**
 * This function recursively removes all content in `path`. It is expected
 * that as least `path` itself exists and is a directory, otherwise an
 * exception is thrown.
 *
 * If an error occurs while removing any of the descendant path elements then
 * a FileError is thrown and the state of the directory tree is undefined.
 * Note that using this function with concurrently file system access from
 * within the calling or another process in the system can cause race
 * conditions that leads to undefined behaviour.
 **/
COSMOS_API void remove_tree(const SysString path);

/// Changes the FileMode of the given path.
/**
 * Attempts to change the FileMode associated with the file object found at
 * the given path. Symlinks will be followed.
 *
 * To change the mode of symlinks use change_owner_nofollow(). To avoid symlinks
 * use File::open() with OpenFlag::NOFOLLOW. Obtain a FileStatus() for the
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
COSMOS_API void change_mode(const SysString path, const FileMode mode);

/// Changes the FileMode of the given open file descriptor.
/**
 * This behaves the same as change_mode(const SysString, FileMode) with the
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
 * `path`. If `path` refers to a symbolic link then the target of the link
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
COSMOS_API void change_owner(const SysString path, const UserID uid, const GroupID gid = GroupID::INVALID);

/// Change numerical owner and/or group ID of the given open file descriptor.
/**
 * This behaves the same as change_owner(const SysString, UserID, GroupID)
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
 * This is a convenience function on top of change_owner(const SysString,
 * UserID, GroupID). It looks up the numerical UserID of `user` and the
 * numerical GroupID of `group`.
 *
 * To skip changing the owner of user or group simply pass an empty
 * SysString as respective parameter.
 *
 * Additional errors that can occur are the ones described in PasswdInfo(),
 * GroupInfo() and RuntimeError() in case the username or group do not
 * exist.
 **/
COSMOS_API void change_owner(const SysString path, const SysString user,
		const SysString group = {});

/// Change owner and/or group of the given file descriptor by user name and/or group name.
/**
 * This is a convenience function on top of change_owner(FileDescriptor,
 * UserID, GroupID). The description of change_owner(const SysString,
 * const SysString, const SysString) applies here as well.
 **/
COSMOS_API void change_owner(const FileDescriptor fd, const SysString user,
		const SysString group = {});

/// Convenience wrapper of change_owner() to change only the group of a file.
inline void change_group(const FileDescriptor fd, const GroupID id) {
	change_owner(fd, UserID::INVALID, id);
}

/// Convenience wrapper of change_owner() to change only the group of a file.
inline void change_group(const FileDescriptor fd, const SysString group) {
	change_owner(fd, {}, group);
}

/// Convenience wrapper of change_owner() to change only the group of a file.
inline void change_group(const SysString path, const GroupID id) {
	change_owner(path, UserID::INVALID, id);
}

/// Convenience wrapper of change_owner() to change only the group of a file.
inline void change_group(const SysString path, const SysString group) {
	change_owner(path, {}, group);
}

/// Changes owner and/or group of the given path while not following symlinks.
/**
 * This behaves the same as change_owner(const SysString, const UserID,
 * const GroupID) with the exception that if the final path component refers
 * to a symbolic link that the ownership of the link is changed, not that of
 * the target.
 *
 * If `path` is not a symlink then it's owner will still be changed and no
 * error is thrown.
 **/
COSMOS_API void change_owner_nofollow(const SysString path, const UserID uid,
		const GroupID gid = GroupID::INVALID);

/// Changes owner and/or group of the given path while not following symlinks.
/**
 * \see change_owner_nofollow(const SysString, const UserID, const GroupID)
 **/
COSMOS_API void change_owner_nofollow(const SysString path, const SysString user,
		const SysString group = {});

/// Creates a symbolic link at `path` pointing to `target`.
/**
 * A symbolic link is simply a pointer to another file system location as
 * provided in `target`. Leading ".." components in `target` will refer to
 * the parent directories of `path`.
 *
 * The permission bits (FileMode) of a symlink are ignored by Linux. The
 * user and group ownership are ignored when dereferencing a symlink, except
 * for when the protected_symlinks feature is enabled (see proc(5); the
 * feature is enabled on most Linux systems these days). The ownership is also
 * relevant for renaming or removing a symlink.
 *
 * If an error occurs then a FileError is thrown. Common errors are:
 *
 * - access denied to `path`. Either search access in the initial components
 *   or write access to the final directory component (Errno::ACCESS).
 * - the file system does not allow the creation of symbolic links
 *   (Errno::PERMISSION).
 * - Earlier parts of `path` do not exist or `target` is empty
 *   (Errno::NO_ENTRY).
 * - `path` already exists (Errno::EXISTS).
 **/
COSMOS_API void make_symlink(const SysString target, const SysString path);

/// Creates a symbolic link relative to `dir_fd` pointing to `target`.
/**
 * This behaves just like make_symlink(). It relates to make_symlink() the
 * same way unlink_file_at() relates to unlink_file().
 **/
COSMOS_API void make_symlink_at(const SysString target, const DirFD dir_fd,
		const SysString path);

/// Returns the target (content) of the symbolic link at `path`.
/**
 * This returns the target path of the symlink present at the given `path`.
 *
 * If an error occurs then a FileError is thrown. Common errors are:
 *
 * - access denied (Errno::ACCESS)
 * - invalid argument if `path` is not a symlink (Errno::INVALID_ARG)
 * - no entry if `path` does not exist (Errno::NO_ENTRY)
 **/
COSMOS_API std::string read_symlink(const SysString path);

/// Returns the target (content) of the symbolic link `path` relative to `dir_fd`.
/**
 * This relates to read_symlink() the same way unlink_file_at() relates to
 * unlink_file().
 *
 * \see unlink_file_at().
 **/
COSMOS_API std::string read_symlink_at(const DirFD dir_fd, const SysString path);

/// Creates a new (hard) link of the file found in `old_path` at `new_path`.
/**
 * Hard links only work on the same file system. An attempt to create a hard
 * link across different mounts will result in an ApiError with
 * Errno::CROSS_DEVICE.
 *
 * If `new_path` already exists then it will not be overwritten but
 * Errno::EXISTS will be thrown.
 *
 * Hard links don't work for directories, if `old_path` refers to one then
 * Errno::PERMISSION will be thrown.
 *
 * Furthermore a range of errors related to lack of permissions, lack of
 * memory, lack of disk space, problems in path resolution and so on can
 * occur.
 *
 * On success both names will refer to the same file and it cannot be
 * determined any more which was was the "original".
 **/
COSMOS_API void link(const SysString old_path, const SysString new_path);

/// Creates a new (hard) link based on lookups relative to `old_dir` and `new_dir`.
/**
 * This behaves similar to link(). For `old_path` and `new_path` the usual
 * at() API rules apply:
 *
 * - if the path is absolute then related DirFD is ignored.
 * - else if the DirFD has the special value cosmos::AT_CWD then path is
 *   interpreted relative to the current working directory.
 * - else the path is interpreted relative to the directory represented by
 *   DirFD.
 *
 * `follow_old` determines whether symlink's encountered at `old_path` will
 * be resolved or not.
 **/
COSMOS_API void linkat(const DirFD old_dir, const SysString old_path,
		const DirFD new_dir, const SysString new_path,
		const FollowSymlinks follow_old = FollowSymlinks{false});

/// Special variant of linkat() that can link arbitrary file descriptors at a new location.
/**
 * Contrary to linkat() this call can give the specified file descriptor a new
 * name at `new_dir` / `new_path`, without specifying a source name. This
 * generally only works for files that have a non-zero link count. This does
 * not work for directory file descriptors.
 *
 * As a special case this *does* work for file descriptors opened with
 * OpenFlag::PATH and for temporary files opened with
 * OpenFlag::TMPFILE but without OpenFlag::EXCLUSIVE (in this case a
 * link count of 0 is accepted).
 *
 * Note, however, that this call requires the CAP_DAC_READ_SEARCH capability,
 * for security reasons. Starting with kernel version 6.10 the requirements
 * are relaxed. If \c fd has been opened by the same process using the same
 * credentials as the caller has, then the linkat will succeed. The
 * credentials must not have been changed (and changed back) in the meantime.
 *
 * An alternative is to use regular linkat() and the /proc file system, see
 * `man 2 linkat`.
 **/
COSMOS_API void linkat_fd(const FileDescriptor fd, const DirFD new_dir,
		const SysString new_path);

/// Performs the same as linkat_fd() using linkat() and the /proc file system.
/**
 * To avoid the permission issues that linkat_fd() has this variant of the
 * linkat call uses a workaround based on the /proc file system to achieve the
 * same result, see `man 2 linkat`.
 **/
COSMOS_API void linkat_proc_fd(const FileDescriptor fd, const DirFD new_dir,
		const SysString new_path);

/// Changes the file size of the file referred to by `fd` to `length` bytes.
/**
 * If `length` is smaller than the current file size, then the extra data
 * stored in the file is lost. If `length` is larger than the current file
 * size, then the extra data reads as zero bytes.
 *
 * This operation requires the file to be opened for writing. A successful
 * operation causes the inode of the file to be updated.
 **/
COSMOS_API void truncate(const FileDescriptor fd, off_t length);

/// Changes the file size of the file found at the given `path` to `length` bytes.
/**
 * Behaves just like truncate(const FileDescriptor, off_t), with the
 * difference that the operation is performed on the given path. The file must
 * be writable.
 **/
COSMOS_API void truncate(const SysString path, off_t length);

/// set of parameters for copy_file_range().
struct CopyFileRangeParameters {
	FileDescriptor in;
	FileDescriptor out;
	size_t len;
	std::optional<off_t> off_in;
	std::optional<off_t> off_out;
};

/// Copy data between files directly in the kernel.
/**
 * This operation allows for an efficient copy operation between two file
 * descriptors, without routing data through userspace. Additionally, if both
 * file descriptors belong to the same file system, this system call allows
 * to optimize the copy on lower levels e.g. by employing copy-on-write
 * techniques.
 *
 * The output file descriptor must not be opened with OpenFlag::APPEND,
 * otherwise an error is thrown.
 *
 * As usual partial I/O can occur. The call attempts to copy `pars.len` bytes
 * and the number of actually copied bytes is returned from this call. If the
 * input file descriptor reached the end of file then 0 is returned. The
 * remaining number of bytes is updated in `pars.len`. This way the same
 * parameter structure can be used to continue the operation until it is
 * complete.
 *
 * If an input offset is supplied then reading from the input file descriptor
 * starts at this position and the file descriptor's offset it not altered.
 * The provided offset is updated by the number of bytes copied. If no input
 * offset is supplied then reading starts from the file descriptor's offset
 * and the offset is altered accordingly. The same holds true for the output
 * offset.
 *
 * There are two other system calls on Linux that perform similar tasks, but
 * have restrictions:
 *
 * - splice(): this is limited to one of the FDs being a pipe
 * - sendfile(): this used to be limited to writing to sockets FDs. Also it
 *   only supports an input FD offset, no output FD offset.
 **/
COSMOS_API size_t copy_file_range(CopyFileRangeParameters &pars);

/// Copy data between files directly in the kernel.
/**
 * This is a simplified version of copy_file_range(CopyFileRangeParameters&)
 * that does not use explicit offsets. Instead the current file descriptor
 * offsets will be used and updated.
 **/
COSMOS_API size_t copy_file_range(
		const FileDescriptor fd_in, const FileDescriptor fd_out, const size_t len);

/// Different access checks that can be performed in check_access().
enum class AccessCheck : int {
	 READ_OK = R_OK, ///< Read access is allowed.
	WRITE_OK = W_OK, ///< Write access is allowed.
	 EXEC_OK = X_OK  ///< Execution is allowed.
};

using AccessChecks = BitMask<AccessCheck>;

/// Check file access permissions of `path`.
/**
 * This checks whether `path` can be accessed by the calling process,
 * depending on the provided `checks` bitmask. If `checks` has no bits set,
 * then this only checks for file existence. Otherwise it is checked whether
 * `path` is accessible for read, write and/or execution, depending on the
 * bits set in `checks`.
 *
 * For determining access permissions the caller's real UID and GID are used.
 *
 * If the check succeeds, then call returns normally. Otherwise a FileError is
 * thrown, containing the specific error reason. Typical errors will be one of
 * the following:
 *
 * - Errno::ACCESS: either the requested `checks` would be denied to the file,
 *   or search permission is denied for one of the components of the prefix of
 *   `path`.
 * - Errno::NO_ENTRY: a component of `path` does not exist or is a dangling
 *   symbolic link.
 *
 * Note that this call is often better replaced by direct open of a path, to
 * prevent any time-of-check/time-of-use race conditions.
 **/
COSMOS_API void check_access(const SysString path, const AccessChecks checks = {});

/// Extra flags that influence the behaviour of check_access_at(), and check_access_fd().
enum class AccessFlag : int {
	EFFECTIVE_CREDS = AT_EACCESS,         ///< use the caller's effective UID and GID for the access check.
	NO_FOLLOW       = AT_SYMLINK_NOFOLLOW ///< don't resolve symlinks in `path` but check access to the link itself.
};

using AccessFlags = BitMask<AccessFlag>;

/// Check file access permissions of `path` relative to `dir_fd`.
/**
 * This behaves similar to `check_access()` with the following differences:
 *
 * - if `path` is a relative path, then it is interpreted relative to
 *   `dir_fd`, instead of relative to the caller's CWD.
 * - if `path` is absolute then `dir_fd` is ignored.
 * - if `dir_fd` is cosmos::AT_CWD, then `path` is again interpreted relative
 *   to the caller's CWD, like check_access() does.
 *
 * The `flags` argument further influences the behaviour of the check.
 **/
COSMOS_API void check_access_at(const DirFD dir_fd, const SysString path,
		const AccessChecks checks = {}, const AccessFlags flags = {});

/// Check file access permissions of the already open file descriptor `fd`.
/**
 * This call may be used on a file descriptor opened with OpenFlag::PATH,
 * which is likely also the main purpose of this variant of `check_access()`.
 **/
COSMOS_API void check_access_fd(const FileDescriptor fd, const AccessChecks check = {},
		const AccessFlags flags = {});

/// Flags used with flock().
enum class LockOperation : int {
	LOCK_SHARED    = LOCK_SH, ///< place a shared lock of which multiple may exist at the same time (for reading)
	LOCK_EXCLUSIVE = LOCK_EX, ///< place an exclusive lock of which only one may exist at the same time (for writing)
	UNLOCK         = LOCK_UN  ///< remove an existing lock (regardless of shared or exclusive)
};

enum class LockFlag : int {
	LOCK_NONBLOCK = LOCK_NB ///< don't block if a lock cannot be placed, throw ApiError with Errno::WOULD_BLOCK instead.
};

/// Additional flags influencing flock behaviour.
using LockFlags = BitMask<LockFlag>;

/// Apply or remove an advisory lock on the given file descriptor.
/**
 * This type of locking is advisory only i.e. the participating processes need
 * to cooperate with each other. A process with sufficient privileges can
 * still modify the file without owning a lock.
 *
 * The lock is associated with the open file description, which means the
 * lock is inherited to child processes via fork() and to duplicated file
 * descriptors. Only once all file descriptors referring to the lock have been
 * closed, will the lock automatically be released. An UNLOCK operation on any
 * file descriptor referring to the open file description will release the
 * lock for all the other file descriptors as well.
 *
 * A process may only hold one type of lock on a file. If the file is already
 * locked, then a subsequent lock can be used to convert the lock type into a
 * different one (i.e. shared to exclusive and vice versa).
 *
 * Locks from this call are preserved across execve() (if a file descriptor
 * referring to the lock is passed through execve()).
 *
 * The OpenMode of the file descriptor is not relevant for locking the file
 * using this API.
 *
 * This API has restrictions and side effects on network file systems like
 * CIFS and NFS:
 *
 * - on older kernels the locking happened only for the local machine
 * - on newer kernels the lock can be transparently implemented via the
 *   `fcntl()` byte-range locking, which has different semantics (e.g. for
 *   placing write locks, the file descriptor has to be open for writing).
 **/
COSMOS_API void flock(const FileDescriptor fd, const LockOperation operation, const LockFlags flags = {});

} // end ns
