// POSIX
#include <unistd.h>
#include <sys/ioctl.h>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/io/Terminal.hxx"

namespace cosmos {

bool Terminal::isTTY() const {
	if (::isatty(m_fd) == 1) {
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
	struct winsize ws;
	int rc = ::ioctl(m_fd, TIOCGWINSZ, &ws);
	if (rc != 0) {
		cosmos_throw (ApiError("ioctl(GWINSZ)"));
	}

	return TermDimension(ws.ws_col, ws.ws_row);
}

} // end ns
