#ifndef COSMOS_THREAD_HXX
#define COSMOS_THREAD_HXX

// POSIX
#include <pthread.h>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/thread/Condition.hxx"
#include "cosmos/thread/IThreadEntry.hxx"

namespace cosmos {

/// A class representing a POSIX thread and its lifecycle
/**
 * There's a simple lifecycle modelled for the thread. Furthermore a simple
 * cooperative state model. Refer to Thread::State for more details.
 *
 * The thread is created during construction time but only enters the
 * specified entry function after start() has been called.
 **/
class COSMOS_API Thread {
	// forbid copy-assignment
	Thread(const Thread&) = delete;
	Thread& operator=(const Thread&) = delete;

public: // types

	/// Possible states for the thread
	/**
	 * Possible lifecycles of a Thread are as follows:
	 *
	 * DEAD (thread construction error)
	 *
	 * DEAD -> READY -> DEAD (thread was constructed but never started)
	 *
	 * DEAD -> READY -> RUNNING -> DEAD (thread was constructed, started, has exited and was joined)
	 *
	 * [RUNNING -> PAUSED -> RUNNING]: entering pause and continuing to run
	 **/
	enum State {
		/// Thread has been created but not yet started
		READY,
		/// Has entered the entry function and is operating
		RUNNING,
		// thread is pausing execution
		PAUSED,
		// thread never was successfully created or has exited and was joined
		DEAD
	};

	/// Available state change requests
	enum Request {
		/// Ask the thread call enterPause()
		PAUSE,
		/// Ask the thread to return from the entry function
		EXIT,
		/// Ask the thread to start the operation
		RUN
	};

	/// Thread IDs for comparison of different threads
	class ID {
		friend class Thread;
	protected: // functions

		explicit ID(pthread_t id) : m_id(id) {}

	public: // functions

		/// Compares two thread IDs for equality
		bool operator==(const ID &other) const;
		bool operator!=(const ID &other) const {
			return !(*this == other);
		}

	protected: // data

		pthread_t m_id;
	};

public: // functions

	/// Creates a thread
	/**
	 * All ressources will be allocated and the thread will be ready to
	 * perform client tasks.
	 *
	 * On error cosmos::ApiError will be thrown.
	 *
	 * \param[in] name
	 * 	An optional friendly name for the thread that is used in
	 * 	logging or possible in operating system facilities to more
	 * 	easily identify threads. If this is not specified then an
	 * 	automatically generated name will be used.
	 **/
	explicit Thread(IThreadEntry &entry, const char *name = nullptr);

	virtual ~Thread();

	/// Returns the current thread lifecycle state
	State getState() const { return this->m_state; }

	/// Returns the currently requested cooperative thread state
	Request getRequest() const { return this->m_request; }

	/// Make the thread enter its entry function
	void start() { this->issueRequest(RUN); }

	/// Request the thread to EXIT
	/**
	 * If the thread is currently inside client code then the client code
	 * is responsible for reacting to this state change.
	 **/
	void requestExit() { this->issueRequest(EXIT); }

	/// Request the thread to PAUSE
	void requestPause() { this->issueRequest(PAUSE); }

	/// Request the thread to START or continue from a PAUSE
	void requestRun() { this->issueRequest(RUN); }

	/// Waits until thread leaves the entry function and terminates
	/**
	 * After returning from this the thread state is EXIT
	 **/
	void join();

	/// Returns a friendly name for the thread
	const std::string& name() const { return m_name; }

	/// Wait for the thread reaching the given state
	/**
	 * Take care not to wait for state conditions that will never be
	 * reached, otherwise this call will block forever.
	 **/
	void waitForState(const State &s) const;

	/// Returns an opaque thread ID object for the thread represented by this object
	ID getID() const { return ID(m_pthread); }

	/// Returns an opaque thread ID object for the calling thread
	static ID getCallerID();

	/// Enter a PAUSE state
	/**
	 * This function may only be called by the thread created from this
	 * object. Otherwise an exception is thrown.
	 *
	 * The thread will be pausing even if nobody requested it to PAUSE. It
	 * will wait for a change of the request state.
	 **/
	Request enterPause();

protected: // functions

	/// Request a state change
	/**
	 * The requested thread state will be change to \p r and the condition
	 * will be signaled to wake up a possibly waiting thread.
	 **/
	void issueRequest(const Request &r) {
		{
			MutexGuard g(m_state_condition);
			m_request = r;
		}

		m_state_condition.signal();
	}

	/// To be called by the associated thread to anounce state changes
	void stateEntered(const State &s);

	/// Returns whether the caller is the associated thread
	bool callerIsThread() const {
		return getID() == getCallerID();
	}

	/// Waits until a new request is issued, returns the new request state
	Request waitForRequest(const Request &old) const;

	/// The C level entry function for the thread
	/**
	 * This function will call run() once the thread is asked to be
	 * start()ed.
	 **/
	static void* posixEntry(void *par);

	/// The C++ entry point of the thread
	void run();

private: // data

	/// POSIX thread handle
	pthread_t m_pthread;

	/// The current state the thread is in
	State m_state;

	/// Current requested state for the thread
	Request m_request;

	/// For waiting for changes to m_state and m_request
	ConditionMutex m_state_condition;

	/// The interface for the thread to run in
	IThreadEntry &m_entry;

	/// Friendly name of the thread
	std::string m_name;
};

} // end ns

#endif // inc. guard
