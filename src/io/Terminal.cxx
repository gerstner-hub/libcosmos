// POSIX
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/io/Terminal.hxx"

namespace cosmos {

bool Terminal::isTTY() const {
	if (::isatty(m_fd.raw()) == 1) {
		return true;
	}

	switch (errno)
	{
	case ENOTTY: break;
	default: cosmos_throw (ApiError("isTTY:"));
	}

	return false;
}

TermDimension Terminal::getSize() const {
	TermDimension ws;
	int rc = ::ioctl(m_fd.raw(), TIOCGWINSZ, &ws);
	if (rc != 0) {
		cosmos_throw (ApiError("ioctl(GWINSZ)"));
	}

	return ws;
}

void Terminal::setSize(const TermDimension &dim) {
	int rc = ::ioctl(m_fd.raw(), TIOCSWINSZ, &dim);
	if (rc != 0) {
		cosmos_throw (ApiError("ioctl(SWINSZ)"));
	}
}

void Terminal::sendBreak(int duration) {
	if (::tcsendbreak(m_fd.raw(), duration) != 0) {
		cosmos_throw (ApiError("tcsendbreak"));
	}
}

} // end ns
