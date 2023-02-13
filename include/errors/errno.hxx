// C
#include <errno.h>

/**
 * @file
 *
 * This header contains strongly typed wrappers and helper functions around
 * the global errno.
 **/

namespace cosmos {

/// strong enum type representing errno error constants
enum class Errno : int { // errnos are distinct positive `int` values says `man errno.h`
	NO_ERROR = 0, 
	TOOBIG = E2BIG,               /// argument list too long
	ACCESS = EACCES,              /// permission denied
	ADDRESS_IN_USE = EADDRINUSE,  /// network address already in use
	ADDRESS_NOT_AVAILABLE = EADDRNOTAVAIL,
	AF_NOT_SUPPORTED = EAFNOSUPPORT, /// address family not supported (networking)
	AGAIN = EAGAIN,               /// resource unavailable, try again (e.g. non-blocking I/O)
	ALREADY = EALREADY,           /// connection already in progress
	BAD_FD = EBADF,               /// bad file descriptor encountered
	BAD_MSG = EBADMSG,
	BUSY = EBUSY,                 /// device or resource busy
	CANCELED = ECANCELED,         /// operation has been canceled
	NO_CHILD = ECHILD,            /// no child process
	CONN_ABORTED = ECONNABORTED,  /// connection was aborted
	CONN_REFUSED = ECONNREFUSED,  /// connection was refused (e.g. no one listening on port)
	CONN_RESET = ECONNRESET,      /// connection was reset
	DEADLOCK = EDEADLK,           /// resource deadlock would occur
	DEST_ADDR_REQ = EDESTADDRREQ, /// destination address required
	OUT_OF_DOMAIN = EDOM,         /// mathematics argument out of domain of function
	EXISTS = EEXIST,              /// file (already) exists
	FAULT = EFAULT,               /// bad address (provided)
	FILE_TOO_BIG = EFBIG,         /// file too large
	HOST_UNREACHABLE = EHOSTUNREACH, /// host is unreachable
	ID_REMOVED = EIDRM,           /// identifier was removed
	ILLEGAL_SEQ = EILSEQ,         /// illegal byte sequence
	IN_PROGRESS = EINPROGRESS,    /// operation is in progress (but not yet completed)
	INTERRUPTED = EINTR,          /// interrupted function (system call)
	INVALID_ARG = EINVAL,         /// invalid argument encountered
	IO_ERROR = EIO,
	IS_CONNECTED = EISCONN,       /// socket is (already?) connected
	IS_DIRECTORY = EISDIR,        /// file is a directory (unexpectedly)
	LINK_LOOP = ELOOP,            /// too many levels of symlinks
	TOO_MANY_FILES = EMFILE,      /// per-process limit of file descriptors encountered
	TOO_MANY_LINKS = EMLINK,      /// too many links encountered (e.g. file system limit)
	MSG_TOO_LARGE = EMSGSIZE,
	NAME_TOO_LONG = ENAMETOOLONG, /// filename too long
	NETWORK_DOWN = ENETDOWN,      /// network is down (e.g. route lost)
	NETWORK_RESET = ENETRESET,    /// connection aborted by network
	NETWORK_UNREACHABLE = ENETUNREACH, /// network is unreachable (no route to host)
	TOO_MANY_FILES_IN_SYS = ENFILE, /// too many files open system wide
	NO_BUFFER_SPACE = ENOBUFS,    /// no buffer space available
	NO_DATA = ENODATA,            /// no message available
	NO_DEVICE = ENODEV,           /// no such device (e.g. device node for non-existing device)
	NO_ENTRY = ENOENT,            /// no such file or directory (or otherwise an object was not found)
	NOT_EXECUTABLE = ENOEXEC,     /// executable file format error
	NO_LOCKS = ENOLCK,            /// no locks available
	NO_MEMORY = ENOMEM,           /// not enough (kernel) memory available for operation
	NO_MESSAGE = ENOMSG,          /// no message of the desired type
	NO_PROTO_OPT = ENOPROTOOPT,   /// protocol (option) not available
	NO_SPACE = ENOSPC,            /// no space left on device
	NO_STREAM_RESOURCES = ENOSR,  /// no stream reasources
	NO_STREAM = ENOSTR,           /// not a STREAM
	NO_SYS = ENOSYS,              /// function not available (e.g. unimplemented system call)
	NOT_CONNECTED = ENOTCONN,     /// socket is not connected
	NOT_A_DIR = ENOTDIR,          /// not a directory, or a symlink link to a directory
	NOT_EMPTY = ENOTEMPTY,        /// directory not empty
	NOT_RECOVERABLE = ENOTRECOVERABLE, /// state not recoverable
	NOT_A_SOCKET = ENOTSOCK,
	NOT_SUPPORTED = ENOTSUP,      /// not supported
	OP_NOT_SUPPORTED = EOPNOTSUPP,/// operation not supported on socket
	NOT_A_TTY = ENOTTY,           /// not a terminal, or unsupported ioctl
	NXIO = ENXIO,                 /// no such device or address
	OVERFLOW = EOVERFLOW,         /// value too large to be stored in data type
	OWNER_DEAD = EOWNERDEAD,      /// previous owner died
	PERMISSION = EPERM,            /// operation not permitted
	BROKEN_PIPE = EPIPE,
	PROTO_ERR = EPROTO,
	PROTO_NOT_SUPPORTED = EPROTONOSUPPORT,
	PROTO_MISMATCH = EPROTOTYPE,  /// wrong protocol type for socket
	RANGE = ERANGE,               /// result too large
	READ_ONLY_FS = EROFS,
	IS_PIPE = ESPIPE,             /// device does not support seek (e.g. a pipe)
	SEARCH = ESRCH,               /// no such process
	TIMER = ETIME,                /// time expired
	TIMEDOUT = ETIMEDOUT,         /// connection timed out
	WOULD_BLOCK = EWOULDBLOCK,    /// operation would block
	TEXT_FILE_BUSY = ETXTBSY,
	CROSS_DEVICE = EXDEV          /// cross-device link
};

/// wrapper that returns the Errno strongly typed representation of the current \c errno
inline Errno getErrno() { return Errno{errno}; }
inline void resetErrno() { errno = static_cast<int>(Errno::NO_ERROR); }
inline bool isErrnoSet() { return getErrno() != Errno::NO_ERROR; }

} // end ns
