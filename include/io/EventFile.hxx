#pragma once

// Linux
#include <sys/eventfd.h>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/fs/FDFile.hxx"

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
 * - with semaphore semantics (\see Flag::SEMAPHORE) upon return from wait()
 *   the value 1 is returned and the counter is decremented by one.
 * - the signal() function adds a value to the counter, thereby potentially
 *   waking up any current waiters.
 *
 * If the counter would overflow due to signal() then the signal() call either
 * blocks until the counter is decremented by another thread or it returns an
 * error if the eventfd is in non-blocking mode (\see Flag::NONBLOCK).
 *
 * Since this is a regular file descriptor the Poller facility can be used to
 * wait for the file descriptor to become readable of writable. Reading
 * corresponds to wait() and writing corresponds to signal().
 **/
class COSMOS_API EventFile :
		protected FDFile {
public: // types

	/// Strong counter type used with the event fd.
	enum class Counter : uint64_t {};

	enum class Flag : int {
		CLOSE_ON_EXEC = EFD_CLOEXEC,  ///< Create the eventfd with the close-on-exec flag set.
		NONBLOCK      = EFD_NONBLOCK, ///< Sets the nonblocking flag upon creation, saving a separate fcntl() call.
		SEMAPHORE     = EFD_SEMAPHORE ///< Use semaphore like semantics.
	};

	using Flags = BitMask<Flag>;

public: // functions

	explicit EventFile(const Counter initval = Counter{0}, const Flags flags = Flags{Flag::CLOSE_ON_EXEC});

	using FDFile::close;
	using FileBase::fd;
	using FileBase::isOpen;

	/// Wait for the counter to become non-zero.
	/**
	 * This potentially blocks until the counter associated with the
	 * eventfd becomes non-zero. Then the current counter value will be
	 * returned and the counter will be reset to zero.
	 *
	 * If Flag::SEMAPHORE is active then only the value of one will be
	 * returned and the counter will be decremented by one.
	 *
	 * If Flag::NONBLOCK is active then no blocking occurs but an
	 * error is thrown if the counter is currently zero.
	 **/
	Counter wait();

	/// Signal the eventfd by adding the given value to the counter.
	/**
	 * This will wake up a potential thread currently blocked in wait().
	 * If an increment larger than 1 is used then either a larger counter
	 * value is returned in wait(), or multiple threads can be waked if
	 * Flag::SEMAPHORE is active.
	 **/
	void signal(const Counter increment = Counter{1});
};

} // end ns
