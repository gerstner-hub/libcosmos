// C++
#include <cassert>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/fs/INotify.hxx>

namespace cosmos {

INotify::INotify(const size_t io_buffer_size, const InitFlags flags) :
		m_io_buffer_size{io_buffer_size} {
	const auto fd = ::inotify_init1(flags.raw());

	if (fd == -1) {
		throw ApiError{"inotify_init1()"};
	}

	m_notify_file = File{FileDescriptor{FileNum{fd}}, AutoCloseFD{true}};
	m_buffer.reserve(io_buffer_size);
}

int INotify::rawFD() const {
	return to_integral(m_notify_file.fd().raw());
}

INotify::WatchID INotify::addWatch(const SysString path, const EventTypes mask, const WatchFlags flags) {
	const auto id = ::inotify_add_watch(rawFD(),
			path.raw(), mask.raw() | flags.raw());

	if (id == -1) {
		throw ApiError{"inotify_add_watch()"};
	}

	return WatchID{id};
}

void INotify::removeWatch(const WatchID id) {
	if (::inotify_rm_watch(rawFD(), cosmos::to_integral(id)) != 0) {
		throw ApiError{"inotify_rm_watch()"};
	}
}

INotify::Event INotify::readEvent() {
	if (m_offset >= m_buffer.size()) {
		/* we need to read in new data */
		m_buffer.resize(m_io_buffer_size);
		/* in case read() throws this leaves us in a reentrant state */
		m_offset = m_io_buffer_size;
		const auto read = m_notify_file.read(m_buffer.data(), m_buffer.size());
		m_offset = 0;
		assert(read > 0);
		m_buffer.resize(read);
	}

	const auto event_loc = m_buffer.data() + m_offset;
	const auto event_ptr = reinterpret_cast<struct inotify_event*>(event_loc);

	m_offset += sizeof(*event_ptr) + event_ptr->len;

	return Event{event_ptr};
}

std::optional<INotify::Event> INotify::tryReadEvent() {
	try {
		return readEvent();
	} catch (const ApiError &ex) {
		if (ex.errnum() == Errno::AGAIN)
			return std::nullopt;

		throw;
	}
}

} // end ns
