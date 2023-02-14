#ifndef COSMOS_POSIXTHREAD_HXX
#define COSMOS_POSIXTHREAD_HXX

// C++
#include <functional>
#include <optional>
#include <string>
#include <string_view>

// cosmos
#include "cosmos/thread/pthread.hxx"
#include "cosmos/time/Clock.hxx"
#include "cosmos/time/TimeSpec.hxx"

namespace cosmos {

/// A class representing a basic POSIX thread.
/**
 * Threads are created at construction time already and enter the specified
 * entry function right away. There is no further modeling of the thread
 * state beyond the joined state.
 *
 * A PosixThread can either be empty or in a joinable state. An empty thread
 * has no reasources associated and no operations can be performed on it. Only
 * in the joinable state can another thread perform a join operation which
 * will block until the other thread exits. After the join operation is
 * complete the state of the object will become empty again. A thread that
 * exits before somebody joins it is still in the joinable state. A joinable
 * thread *must* be joined before the associated PosixThread object is
 * destroyed or move-assigned to.
 *
 * A thread that is created in joinable state can be detached. This causes the
 * thread object to become empty but the associated thread will continue
 * running independently. No other thread needs to (or can) join a detached
 * thread and its resources will be cleaned up automatically once the detached
 * thread exits.
 *
 * This class currently supports two types of entry points for threads:
 *
 * - PosixEntry: This is a thin wrapper around the low level POSIX thread
 *   entry function which receives a single void* and returns a single void*.
 *   The return value can be collected at join time. This entry variant can be
 *   used to interact with threads from other libraries or for very simple
 *   thread operations. Since the argument and return types aren't safe (need
 *   to be casted) it should only be used if really necessary.
 * - Entry: This is a plain `void (void)` entry function. To pass data to the
 *   thread use e.g. std::bind to make the thread enter a member function, use
 *   a lambda with captures or similar.
 *
 * PosixThread is a move-only type. It cannot be copied. The ownership can be
 * transfer via std::move but be carefuly that a thread that is not yet joined
 * cannot be moved into, this will cause std::abort() to be called.
 **/
class COSMOS_API PosixThread {

	// forbid copy-assignment
	PosixThread(const PosixThread&) = delete;
	PosixThread& operator=(const PosixThread&) = delete;

public: // types

	/// POSIX style entry function with a single input parameter and return value.
	using PosixEntry = std::function<pthread::ExitValue (pthread::ThreadArg)>;
	/// Entry function without parameters for use with member functions or lambdas.
	using Entry = std::function<void (void)>;
	/// The clock type used in joinTimed().
	using Clock = RealtimeClock;

public: // functions
	
	/// Creates an empty thread object.
	/**
	 * This will simply create an empty thread object without invoking any
	 * system calls. Performing any operations on it will fail. joinable()
	 * will return \c false.
	 **/
	PosixThread() noexcept {}

	/// Creates a thread running in the provided PosixEntry function.
	/**
	 * All necessary ressources will be allocated and the thread will
	 * enter the given entry function.
	 *
	 * On error cosmos::ApiError will be thrown.
	 *
	 * \param[in] arg
	 * 	The single parameter passed to the entry function.
	 * \param[in] name
	 * 	An optional friendly name for the thread that is used in
	 * 	logging or possible in operating system facilities to more
	 * 	easily identify threads. If this is not specified then an
	 * 	automatically generated name will be used.
	 **/
	PosixThread(PosixEntry entry, pthread::ThreadArg arg, const std::string_view name = {});

	/// Creates a thread running in the provided simple Entry function.
	/**
	 * \see PosixThread(PosixEntry, pthread::ThreadArg, const string_view)
	 **/
	explicit PosixThread(Entry entry, const std::string_view name = {});

	PosixThread(PosixThread &&other) noexcept;

	PosixThread& operator=(PosixThread &&other) noexcept;

	virtual ~PosixThread();

	/// Returns whether a thread is attached to this object (and needs to be joined).
	bool joinable() const {
		return m_pthread.has_value();
	}

	/// Blocks until the associated thread returns.
	/**
	 * This call is only allowed if joinable() returns \c true i.e. if
	 * currently a joinable thread is attached to this object.
	 *
	 * The call will block forever until the target thread ends execution
	 * which can happen either by the thread returning from its entry
	 * function of by the thread calling pthread::exit().
	 *
	 * The returned value is the one returned from a PosixEntry style
	 * entry function, the vaule provided to pthread::exit() or
	 * ExitValue{0} in any other cases.
	 **/
	pthread::ExitValue join();

	/// Attempts to immediately joins the associated thread.
	/**
	 * This behaves just like join() with the exception that it only polls
	 * once whether joining the thread is currently possible. If this is
	 * the case then the thread state is cleaned up and its return value
	 * returned.  If joining is not possible then nothing is returned and
	 * the thread state is not changed. This call will not block.
	 **/
	std::optional<pthread::ExitValue> tryJoin();

	/// Waits for the associated thread to return for a given time period.
	/**
	 * This behaves similar to tryJoin() with the exception that the call
	 * blocks for a given time period before the operation fails and no
	 * value is returned.
	 *
	 * \warning The clock used for \c ts is the RealtimeClock (see
	 * PosixThread::Clock), although the implementation (glibc) calculates
	 * an offset that will in turn be measured against MonotonicClock. So
	 * the timeout will be (somewhat) unaffected by discontinous changes
	 * to the realtime clock.
	 **/
	std::optional<pthread::ExitValue> joinTimed(const TimeSpec ts);

	/// Detach a joinable thread.
	/**
	 * This call is only allowed if joinable() returns \c true. Otherwise
	 * a UsageError is thrown.
	 *
	 * On success the associated thread is converted into a detached
	 * thread. The object will become an empty thread. The detached thread
	 * will continue running independently and will automatically be
	 * cleaned up once it exits. The detached thread cannot be joined any
	 * more.
	 **/
	void detach();

	/// Returns a friendly name for the thread
	/**
	 * If joinable() returns \c true then this returns a friendly name for
	 * the associated thread. Part of this will be the friendly name
	 * passed at construction time, or an automatically generated name
	 * otherwise.
	 *
	 * If this is an empty thread then an empty string is returned.
	 **/
	const std::string& name() const { return m_name; }

	/// Returns an opaque thread ID object for the thread represented by this object
	/**
	 * This call is only allowed if joinable() returns \c true.
	 *
	 * The returned object serves the purpose of comparing different
	 * threads to each other. Any thread can obtain its own ID by calling
	 * pthread::getID().
	 *
	 * The returned object can also be used to obtain the native thread
	 * handle.
	 **/
	pthread::ID id() const { return pthread::ID{*m_pthread}; }

	/// Returns whether the caller itself is the associated thread
	bool isCallerThread() const {
		return id() == pthread::getID();
	}

protected: // functions
	
	std::string buildName(const std::string_view name, size_t nr) const;

	void assertJoinConditions();

	void reset();

protected: // data
	
	/// POSIX thread handle
	std::optional<pthread_t> m_pthread;

	/// Friendly name of the thread
	std::string m_name;
};

} // end ns

#endif // inc. guard
