#ifndef COSMOS_TERMINAL_HXX
#define COSMOS_TERMINAL_HXX

// Cosmos
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/FileDescriptor.hxx"

namespace cosmos {

//! represents a terminal dimension in characters
struct TermDimension {
	size_t width;
	size_t height;

	TermDimension(size_t w, size_t h) : width(w), height(h) {}
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

protected: // data
	FileDescriptor m_fd;
};

} // end ns

#endif // inc. guard
