#pragma once

// C++
#include <chrono>
#include <optional>
#include <vector>

// Linux
#include <sys/epoll.h>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/fs/FileDescriptor.hxx>

namespace cosmos {

/// Efficient file descriptor I/O event polling.
/**
 * This class provides a wrapper around the epoll() Linux specific file
 * descriptor monitoring API. The API operates on a file descriptor of its own
 * that references a set of monitored file descriptors.
 *
 * A peculiarity of the API is that it can operate in a level triggered or an
 * edge triggered fashion. The level triggered mode is the one known from
 * classical APIs like select(). It means that a file descriptor will always
 * be signaled as ready if currently one of the monitoring conditions is
 * fulfilled. Edge triggered instead means that the condition is only signaled
 * once for a single event and afterwards only triggered again if additional
 * events occur, regardless of whether data was actually read/written to the
 * monitored file descriptors or not.
 *
 * The edge triggered approach can be more efficient as it requires less
 * system calls on high I/O load. However, it also requires more care taken by
 * the implementation of the userspace application. The general recommendation
 * is that all monitored file descriptors should be operated in non-blocking
 * mode in this case and once an event is signaled the respective file
 * descriptor should be read from / written to until an EAGAIN result is
 * encountered. Only then should the poll API be consulted again for waiting
 * for further events.
 *
 * Special care also is required when file descriptors that are monitored are
 * closed within the application. The kernel monitors open file *descriptions*
 * here, not only open file descriptors. This means if there exist copies of a
 * file descriptor then closing one of the involved file descriptors will not
 * end monitoring of the still open file description.
 *
 * File descriptors signaled as being ready for non-blocking I/O could still
 * be blocking for example in case of networking sockets, e.g. if a received
 * packet has an invalid checksum and therefore nothing to return to
 * userspace. To avoid such scenarios the application should use non-blocking
 * file descriptors and react to EAGAIN results upon read/write.
 *
 * The Poller FD is created, as usual, with the O_CLOEXEC flag set. Explicitly
 * re-enable the flag should you require inheritance to unrelated sub
 * processes.
 **/
class COSMOS_API Poller {
public: // types

	/// Flags used to declare interest in specific events and options in addFD() and modFD().
	enum class MonitorFlag : uint32_t {
		/// Monitor for read() operation becoming possible
		INPUT          = EPOLLIN,
		/// Monitor for write() operation becoming possible
		OUTPUT         = EPOLLOUT,
		/// Monitor for stream socket peer closed or shut down the write half of the connection (data may still be pending)
		SOCKET_HANGUP  = EPOLLRDHUP,
		/// Monitor for exceptional conditions occuring on the file descriptor, depending on the actual file type
		EXCEPTIONS     = EPOLLPRI,
		/// Operate in edge triggered mode instead of level triggered (which is the default)
		EDGE_TRIGGERED = EPOLLET,
		/// Only report events once, then disable monitoring until this flag is set again using modFD()
		ONESHOT        = EPOLLONESHOT,
		/// If the process has the CAP_BLOCK_SUSPEND capability then the system won't enter a suspend state until the process that received this event calls wait() again.
		STAY_AWAKE     = EPOLLWAKEUP
	};

	using MonitorFlags = BitMask<MonitorFlag>;

	/// Flags found in PollEvent that indicate the events that occurred on a file descriptor.
	enum class Event : uint32_t {
		/// \see MonitorFlag::INPUT
		INPUT_READY       = EPOLLIN,
		/// \see MonitorFlag::OUTPUT
		OUTPUT_READY      = EPOLLOUT,
		/// \see MonitorFlag::SOCKET_HANGUP
		SOCKET_HANGUP     = EPOLLRDHUP,
		/// \see MonitorFlag::EXCEPTIONS
		EXCEPTION_OCCURED = EPOLLPRI,
		/// An error condition occurred on the file descriptor (this is also reported for the write end of a pipe, if the read end is closed). This event is always reported independently of MonitorFlag.
		ERROR_OCCURED     = EPOLLERR,
		/// Socket or pipe peer has hung up. Data may still be pending though. This event is always reported independently of MonitorFlags.
		HANGUP_OCCURED    = EPOLLHUP
	};

	using EventMask = BitMask<Event>;

	/// A single poll event as returned by wait().
	struct PollEvent : protected epoll_event {
		friend class Poller;
	public:

		/// The file descriptor this event refers to.
		FileDescriptor fd() const { return FileDescriptor{FileNum{(this->data).fd}}; }

		auto getEvents() const { return EventMask{static_cast<std::underlying_type<Event>::type>(this->events)}; }
	};

public: // functions

	/// Creates a yet invalid Poller instance.
	Poller() {}

	/// Creates a Poller instance ready for use.
	/**
	 * \see create()
	 **/
	explicit Poller(size_t max_events) {
		create(max_events);
	}

	/// Calls close()
	~Poller();

	/// Avoid copying due to the file descriptor member.
	Poller(const Poller&) = delete;
	Poller& operator=(const Poller&) = delete;

	/// Actually create the poll file descriptor backing this object.
	/**
	 * If the file descriptor already exists this does nothing.
	 *
	 * If the creation fails then an ApiError is thrown.
	 *
	 * \param[in] max_events Maximum number of events that can be reported
	 * with a single call to wait().
	 **/
	void create(size_t max_events = 16);

	/// Closes a previously create()'d poll file descriptor again.
	/**
	 * Any monitoring that was setup previously will be dropped. A future
	 * call to create() can reestablish the Poller functionality.
	 *
	 * In the (rare) case that closing fails then an ApiError is thrown.
	 *
	 * If currently no valid poll file descriptor exist then this function
	 * does nothing.
	 **/
	void close();

	/// Returns whether currently a valid poll file descriptor exists.
	bool valid() const { return m_poll_fd.valid(); }

	/// Start monitoring the given file descriptor using the given settings.
	/**
	 * If currently no valid poll FD exists then this will throw an
	 * ApiError exception.
	 *
	 * Adding the same file descriptor twice also causes an error. Use
	 * modFD() to modify monitoring settings for FDs already monitored.
	 **/
	void addFD(const FileDescriptor fd, const MonitorFlags flags);

	/// Modify monitoring settings for an already monitored descriptor.
	/**
	 * If currently no valid poll FD exists then this will throw an
	 * ApiError exception.
	 **/
	void modFD(const FileDescriptor fd, const MonitorFlags flags);

	/// Remove a file descriptor from the set of monitored files.
	/**
	 * If the given file descriptor is not currently monitored then this
	 * will throw an ApiError.
	 **/
	void delFD(const FileDescriptor fd);

	/// Wait for one of the monitored events to be ready.
	/**
	 * \param[in] timeout An optional timeout to apply after which the
	 * call will return even if no events are ready. An empty vector is
	 * returned in the timeout case.
	 *
	 * \return The range of events that occurred, or an empty vector if the
	 * timeout occurred.
	 **/
	std::vector<PollEvent> wait(const std::optional<std::chrono::milliseconds> timeout = {});

protected: // functions

	int rawPollFD() const;

protected: // data

	FileDescriptor m_poll_fd;
	std::vector<PollEvent> m_events;
};

} // end ns
