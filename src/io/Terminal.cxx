// BSD
#include <pty.h>

// POSIX
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/io/Terminal.hxx"

namespace cosmos {

int Terminal::rawFD() const {
	return to_integral(m_fd.raw());
}

bool Terminal::isTTY() const {
	if (::isatty(rawFD()) == 1) {
		return true;
	}

	switch (get_errno()) {
		case Errno::NOT_A_TTY: break;
		default: cosmos_throw (ApiError("isTTY:"));
	}

	return false;
}

TermDimension Terminal::getSize() const {
	TermDimension ws;
	int rc = ::ioctl(rawFD(), TIOCGWINSZ, &ws);
	if (rc != 0) {
		cosmos_throw (ApiError("ioctl(GWINSZ)"));
	}

	return ws;
}

void Terminal::setSize(const TermDimension dim) {
	int rc = ::ioctl(rawFD(), TIOCSWINSZ, &dim);
	if (rc != 0) {
		cosmos_throw (ApiError("ioctl(SWINSZ)"));
	}
}

void Terminal::sendBreak(const std::chrono::milliseconds ms) {
	if (::tcsendbreak(rawFD(), static_cast<int>(ms.count())) != 0) {
		cosmos_throw (ApiError("tcsendbreak"));
	}
}

void Terminal::makeControllingTerminal(bool force) {
	int rc = ::ioctl(rawFD(), TIOCSCTTY, force ? 1 : 0);
	if (rc != 0) {
		cosmos_throw (ApiError("ioctl(TIOCSCTTY)"));
	}
}

std::pair<FileDescriptor, FileDescriptor> openPTY(const std::optional<TermDimension> initial_size) {
	int master, slave;

	// the name parameter is unsafe since there is no standardized
	// dimension limit.
	// the other parameters are for setting initial TTY properties and
	// terminal size.

	const struct winsize *size = initial_size ? &(*initial_size) : nullptr;

	if (::openpty(&master, &slave, nullptr, nullptr, size) < 0) {
		cosmos_throw (ApiError("openpty failed"));
	}

	return {
		cosmos::FileDescriptor(FileNum{master}),
		cosmos::FileDescriptor(FileNum{slave})
	};
}

} // end ns
