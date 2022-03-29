#ifndef COSMOS_TERMINAL_HXX
#define COSMOS_TERMINAL_HXX

// POSIX
#include <sys/ioctl.h>

// Cosmos
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/FileDescriptor.hxx"

namespace cosmos {

//! represents a terminal dimension in characters
struct TermDimension : winsize {

	TermDimension(size_t cols = 0, size_t rows = 0) {
		ws_col = cols;
		ws_row = rows;
	}

	unsigned short getCols() const { return ws_col; }
	unsigned short getRows() const { return ws_row; }
};

/// Access to Terminal information and ioctls
/**
 * This simply wraps a FileDescriptor for performing terminal related ioctls
 * on it. It will not take ownership of the file descriptor i.e. it will never
 * be closed by this class.
 **/
class COSMOS_API Terminal
{
public:
	Terminal() {}
	explicit Terminal(FileDescriptor fd) : m_fd(fd) {}
	explicit Terminal(const File &f) : m_fd(f.getFD()) {}

	void setFD(const File &f) {
		m_fd = f.getFD();
	}

	void setFD(FileDescriptor fd) {
		m_fd = fd;
	}

	//! returns whether the associated file descriptor is a TTY
	bool isTTY() const;

	//! returns the terminal dimension in character width x height
	TermDimension getSize() const;
	//! sets the terminal dimension according to the given values
	void setSize(const TermDimension &dim);

	/// sends a stream of zero bits for a certain duration
	/**
	 * If duration is zero then the stream will last between 0.25 and 0.50
	 * seconds. If non-zero then the stream will last for an
	 * implementation defined time (on Linux the given duration in
	 * milliseconds).
	 **/
	void sendBreak(int duration);

protected: // data
	FileDescriptor m_fd;
};

} // end ns

#endif // inc. guard
