// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/io/EventFile.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

EventFile::EventFile(const Counter initval, const Flags flags) {
	auto fd = ::eventfd(to_integral(initval), flags.raw());

	if (fd == -1) {
		throw ApiError{"eventfd()"};
	}

	this->open(FileDescriptor{FileNum{fd}}, AutoCloseFD{true});
}

EventFile::Counter EventFile::wait() {
	Counter ret;
	// I don't believe short reads are possible with eventfds, so use
	// regular read() instead of readAll().
	const auto bytes = this->read(&ret, sizeof(Counter));

	if (bytes != sizeof(ret)) {
		throw RuntimeError{"short eventfd read?!"};
	}

	return ret;
}

void EventFile::signal(const Counter increment) {
	const auto bytes = this->write(&increment, sizeof(increment));

	if (bytes != sizeof(increment)) {
		throw RuntimeError{"short eventfd write?!"};
	}
}

} // end ns
