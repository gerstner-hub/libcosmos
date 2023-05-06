#ifndef COSMOS_EVENTFILE_HXX
#define COSMOS_EVENTFILE_HXX

// Linux
#include <sys/eventfd.h>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/fs/StreamFile.hxx"

namespace cosmos {

/// Wrapper around an eventfd FileDescriptor.
/**
 * An eventfd is a lightweight event object using file descriptor
 * representation. An unsigned 8 byte counter is associated with the eventfd
 * that controls the event operation.
 *
 * This type manages creation and the lifetime of the underlying file
 * descriptor and provides an I/O API tailored towards the speical event file
 * semantics.
 *
 * The event semantics are as follows:
 *
 * - if the counter is zero any wait() on it will block until the counter is
 *   increment by another thread.
 * - upon return from wait() regular eventfd semantics cause the current
 *   counter value to be returned and the counter is reset to zero.
 * - with semaphore semantics (\see Settings::SEMAPHORE) upon return from wait()
 *   the value 1 is returned and the counter is decremented by one.
 * - the signal() function adds a value to the counter, thereby potentially
 *   waking up any current waiters.
 * 
 * If the counter would overflow due to signal() then the signal() call either
 * blocks until the counter is decremented by another thread or it returns an
 * error if the eventfd is in non-blocking mode (\see Settings::NONBLOCK).
 *
 * Since this is a regular file descriptor the Poller facility can be used to
 * wait for the file descriptor to become readable of writable. Reading
 * corresponds to wait() and writing corresponds to signal().
 **/
class COSMOS_API EventFile {
public: // types

	/// Strong counter type used with the event fd.
	enum class Counter : uint64_t {};

	enum class Settings : int {
		CLOSE_ON_EXEC = EFD_CLOEXEC,  /// Create the eventfd with the close-on-exec flag set.
		NONBLOCK      = EFD_NONBLOCK, /// Sets the nonblocking flag upon creation, saving a separate fcntl() call.
		SEMAPHORE     = EFD_SEMAPHORE /// Use semaphore like semantics.
	};

	using Flags = BitMask<Settings>;

public: // functions

	explicit EventFile(const Counter initval = Counter{0}, const Flags flags = Flags{Settings::CLOSE_ON_EXEC});

	EventFile(const EventFile &) = delete;
	EventFile& operator=(const EventFile&) = delete;

	EventFile(EventFile &&other) {
		*this = std::move(other);
	}

	EventFile& operator=(EventFile &&other) {
		m_file = std::move(other.m_file);
		return *this;
	}

	void close();

	bool isOpen() {
		return m_file.isOpen();
	}

	/// Wait for the counter to become non-zero.
	/**
	 * This potentially blocks until the counter associated with the
	 * eventfd becomes non-zero. Then the current counter value will be
	 * returned and the counter will be reset to zero.
	 *
	 * If Settings::SEMAPHORE is active then only the value of one will be
	 * returned and the counter will be decremented by one.
	 *
	 * If Settings::NONBLOCK is active then no blocking occurs but an
	 * error is thrown if the counter is currently zero.
	 **/
	Counter wait();

	/// Signal the eventfd by adding the given value to the counter.
	/**
	 * This will wake up a potential thread currently blocked in wait().
	 * If an increment larger than 1 is used then either a larger counter
	 * value is returned in wait(), or multiple threads can be waked if
	 * Settings::SEMAPHORE is active.
	 **/
	void signal(const Counter increment = Counter{1});

	/// Direct access to the underlying file descriptor e.g. for use with Poller.
	FileDescriptor fd() const {
		return m_file.fd();
	}

	/// Provides direct access to the StreamFile API of the eventfd.
	/**
	 * The caller shouldn't close the return file, it should only be used
	 * for I/O purposes.
	 **/
	StreamFile& streamFile() {
		return m_file;
	}

protected: // data

	StreamFile m_file;
};

} // end ns

#endif // inc. guard
