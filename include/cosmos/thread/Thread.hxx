#ifndef COSMOS_THREAD_HXX
#define COSMOS_THREAD_HXX

// POSIX
#include <pthread.h>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/thread/Condition.hxx"
#include "cosmos/thread/IThreadEntry.hxx"

namespace cosmos
{

/**
 * \brief
 * 	A class representing a POSIX thread and its lifecycle
 * \details
 * 	There's a simple lifecycle modelled for the thread. Refer to
 * 	Thread::State for more details.
 *
 * 	The thread is created during construction time but only enters the
 * 	specified functions after start() has been called.
 **/
class Thread
{
	// forbid copy-assignment
	Thread(const Thread&) = delete;
	Thread& operator=(const Thread&) = delete;

public: // types

	/**
	 * \brief
	 * 	Possible states for the thread
	 * \details
	 * 	Possible lifecycles of a Thread are as follows:
	 *
	 * 	DEAD (thread construction error)
	 *
	 * 	DEAD -> READY -> DEAD (thread was constructed but never started)
	 *
	 * 	DEAD -> READY -> RUN -> EXIT -> DEAD (thread was constructed,
	 * 	started, has exited and was joined)
	 **/
	enum State
	{
		// thread is created but has not yet been started by the client
		READY,
		// runs and performs operation
		RUN,
		// stop operation and exit
		EXIT,
		// thread is pausing execution
		PAUSE,
		// thread never was successfully created or has exited and was joined
		DEAD
	};

	//! thread IDs for comparison
	class ID {
	protected: // functions
		friend class Thread;
		explicit ID(pthread_t id) : m_id(id) {}

	public: // functions

		//! compares two thread IDs for equality
		bool operator==(const ID &other) const;
		bool operator!=(const ID &other) const {
			return !(*this == other);
		}

	protected: // data
		pthread_t m_id;
	};

public: // functions

	/**
	 * \brief
	 * 	Creates a thread
	 * \details
	 *	All ressources will be allocated and the thread will be ready
	 *	to perform client tasks.
	 *
	 *	On error cosmos::ApiError will be thrown.
	 * \param[in] name
	 * 	An optional friendly name for the thread that is used in
	 * 	logging or possible in operating system facilities to more
	 * 	easily identify threads.
	 **/
	explicit Thread(IThreadEntry &entry, const char *name = nullptr);

	virtual ~Thread();

	State getCurState() const { return this->m_cur_state; }

	State getRequestedState() const { return this->m_req_state; }

	//! make the thread enter the client function
	void start() { this->requestState( RUN ); }

	/**
	 * \brief
	 * 	Mark the thread state as EXIT
	 * \details
	 * 	If the thread is currently inside client code then the client
	 * 	code is responsible for reacting to this state change.
	 **/
	void requestExit() { this->requestState( EXIT ); }

	void requestPause() { this->requestState( PAUSE ); }

	void requestRun() { this->requestState( RUN ); }

	//! waits until thread leaves the client function and terminates, sets
	//! state to EXIT
	void join();

	//! returns a friendly name for the thread
	const std::string& name() const { return m_name; }

	/**
	 * \brief
	 * 	Wait for the thread reaching the given state
	 * \details
	 * 	Take care not to wait for impossible state conditions,
	 * 	otherwise this call will block forever.
	 **/
	void waitForState(const State &s) const;

	//! returns an opaque thread ID object for the thread represented by
	//! this object
	ID getID() const { return ID(m_pthread); }

	//! returns an opaque thread ID object for the calling thread
	static ID getCallerID();

	//! enter a Pause state, if requested
	State enterPause();


protected: // functions

	//! changes the requested thread state to \c s and signals the
	//! condition to wake up a possibly waiting thread
	void requestState(const State &s)
	{
		{
			MutexGuard g(m_state_condition);
			m_req_state = s;
		}

		m_state_condition.signal();
	}

	void stateEntered(const State &s);

	bool callerIsThread() const
	{
		return getID() == getCallerID();
	}

	State waitForRequest(const State &old) const;

	//! POSIX / C entry function for thread. From here the virtual
	//! threadEntry() will be called
	static void* posixEntry(void *par);

	//! actual C++ entry point
	void run();

private: // data

	//! POSIX thread handle
	pthread_t m_pthread;

	//! the current state the thread is in
	State m_cur_state;

	//! requested state for the thread
	State m_req_state;

	//! for waiting for changes to m_cur_state and m_req_state
	ConditionMutex m_state_condition;

	//! The interface for the thread to run in
	IThreadEntry &m_entry;

	//! friendly name of the thread
	std::string m_name;
};

} // end ns

#endif // inc. guard
