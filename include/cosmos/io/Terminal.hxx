#ifndef COSMOS_TERMINAL_HXX
#define COSMOS_TERMINAL_HXX

// Cosmos
#include "cosmos/ostypes.hxx"
#include "cosmos/fs/File.hxx"

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
class Terminal
{
public:
	explicit Terminal(FileDesc fd) : m_fd(fd) {}
	explicit Terminal(const File &f) : m_fd(f.getFD()) {}

	//! returns whether the associated file descriptor is a TTY
	bool isTTY() const;

	//! returns the terminal dimension in character width x height
	TermDimension getSize() const;

protected: // data
	FileDesc m_fd;
};

} // end ns

#endif // inc. guard
