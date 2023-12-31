#pragma once

// POSIX
#include <sys/ioctl.h>

// C++
#include <chrono>
#include <utility>
#include <optional>

// Cosmos
#include "cosmos/fs/FileBase.hxx"
#include "cosmos/fs/FileDescriptor.hxx"

/**
 * @file
 *
 * This header contains types and helper concerned with terminal / TTY
 * features of the operating system.
 **/

namespace cosmos {

/// Represents a terminal dimension in characters.
struct TermDimension :
		winsize {

	explicit TermDimension(size_t cols = 0, size_t rows = 0) {
		ws_col = cols;
		ws_row = rows;
	}

	unsigned short cols() const { return ws_col; }
	unsigned short rows() const { return ws_row; }
};

/// Access to Terminal information and ioctls.
/**
 * This simply wraps a FileDescriptor for performing terminal related ioctls
 * on it. It will not take ownership of the file descriptor i.e. it will never
 * be closed by this class.
 **/
class COSMOS_API Terminal {
public:
	Terminal() {}
	explicit Terminal(FileDescriptor fd) : m_fd(fd) {}
	explicit Terminal(const FileBase &f) : m_fd(f.fd()) {}

	void setFD(const FileBase &f) {
		m_fd = f.fd();
	}

	void setFD(FileDescriptor fd) {
		m_fd = fd;
	}

	/// Returns whether the associated file descriptor is a TTY.
	bool isTTY() const;

	/// Returns the terminal dimension in character width x height.
	TermDimension getSize() const;
	/// Sets the terminal dimension according to the given values.
	void setSize(const TermDimension dim);

	/// Sends a stream of zero bits for a certain duration.
	/**
	 * If \c ms is zero then the stream will last between 0.25 and 0.50
	 * seconds. If it is non-zero then the stream will last for an
	 * implementation defined time (on Linux the given duration in
	 * milliseconds).
	 **/
	void sendBreak(const std::chrono::milliseconds ms);

	/// Attempt to make the terminal the controlling terminal of the current process.
	/**
	 * This only works if the current process is a session leader and does
	 * not yet have a controlling terminal.
	 *
	 * If the caller has CAP_SYS_ADMIN capability and \c force is true
	 * then the terminal is "stolen" and all processes that had this
	 * terminal as controlling terminal before, lose it.
	 *
	 * On error an exception is thrown by this call.
	 **/
	void makeControllingTerminal(bool force = false);

protected: // functions

	int rawFD() const;

protected: // data

	FileDescriptor m_fd;
};

/// Creates a new pseudo terminal device and returns master/slave file descriptors for it.
/**
 * A pseudo terminal is a virtual terminal where the slave end behaves like an
 * actual terminal device and can be passed to applications that expect one.
 * The master end drives the application using the slave end.
 *
 * Any writes to the master end will appear as input from a keyboard to the
 * slave, any writes to the slave end will appear as output data from a
 * program on the master end.
 *
 * See openpty(2) and pty(7) man pages for more information.
 *
 * \param[in] initial_size if provided then this will be the initial PTY
 *            dimension registered by the kernel. See Terminal::setSize().
 *
 * \return a pair of the master and slave file descriptor belonging to the new
 *         PTY. The caller is responsible for managing the lifetime of the
 *         returned file descriptors (i.e. closing them at the appropriate
 *         time). On error an ApiError exception is thrown.
 **/
COSMOS_API std::pair<cosmos::FileDescriptor, cosmos::FileDescriptor> openPTY(
		const std::optional<TermDimension> initial_size = {});

} // end ns
