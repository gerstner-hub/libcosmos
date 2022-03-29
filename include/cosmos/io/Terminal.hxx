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

/**
 * \brief
 * 	Access to Terminal information
 **/
class COSMOS_API Terminal
{
public:
	explicit Terminal(FileDescriptor fd) : m_fd(fd) {}
	explicit Terminal(const File &f) : m_fd(f.getFD()) {}

	//! returns whether the associated file descriptor is a TTY
	bool isTTY() const;

	//! returns the terminal dimension in character width x height
	TermDimension getSize() const;
	//! sets the terminal dimension according to the given values
	void setSize(const TermDimension &dim);

protected: // data
	FileDescriptor m_fd;
};

} // end ns

#endif // inc. guard
