// stdlib
#include <atomic>
#include <cassert>
#include <iostream>
#include <string>

// cosmos
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/thread/Thread.hxx"

namespace cosmos {

static std::atomic<size_t> g_num_threads = 0;

Thread::Thread(IThreadEntry &entry, const char *name) :
	m_cur_state(READY),
	m_req_state(READY),
	m_entry(entry),
	// we could also use a counter to make unique anonymous
	// threads
	m_name( name ? name : "thread<" + std::to_string(g_num_threads) + ">" )
{
	const auto error = ::pthread_create(
		&m_pthread,
		nullptr /* keep default attributes */,
		&posixEntry,
		(void*)this /* pass pointer to object instance */
	);

	if (error != 0) {
		m_cur_state = DEAD;
		cosmos_throw (cosmos::ApiError("Unable to create thread"));
	}

	g_num_threads++;
}

Thread::~Thread() {
	g_num_threads--;

	try {
		assert (m_cur_state != RUN && m_cur_state != EXIT);

		if (m_cur_state == READY) {
			// the thread was never state so join thread first
			this->join();
		}
	}
	catch (...)
	{
		assert ("thread_destroy_error" == nullptr);
	}
}

void* Thread::posixEntry(void *par) {
	auto &thread = *(reinterpret_cast<Thread*>(par));

	const auto req = thread.waitForRequest (READY);

	if (req == RUN || req == PAUSE) {
		thread.run();
	}

	thread.stateEntered(DEAD);

	return nullptr;
}

void Thread::run() {
	stateEntered(RUN);

	try {
		// enter client function
		m_entry.threadEntry(*this);
	}
	catch (const cosmos::CosmosError &e) {
		std::cerr << "Caught exception in " << __FUNCTION__
			<< ", thread name = \"" << name() << "\".\nException: "
			<< e.what() << "\n";
	}
	catch (...) {
		std::cerr
			<< "Caught unknown exception in " << __FUNCTION__
			<< ", thread name = \"" << name() << "\".\n";
	}
}

void Thread::join() {
	if (callerIsThread()) {
		cosmos_throw (UsageError("Attempted to join self"));
	}

	this->requestState(EXIT);

	void *res = nullptr;
	const auto join_res = ::pthread_join(m_pthread, &res);

	if (join_res != 0) {
		cosmos_throw (ApiError("Failed to join thread"));
	}
}

void Thread::stateEntered(const State &s) {
#ifndef NDEBUG
	if (!callerIsThread()) {
		cosmos_throw (UsageError("Foreign thread modified state"));
	}
#endif

	{
		MutexGuard g(m_state_condition);
		if (m_cur_state == s)
			return;
		m_cur_state = s;
	}

	m_state_condition.broadcast();
}

void Thread::waitForState(const Thread::State &s) const {
	MutexGuard g(m_state_condition);

	while (m_cur_state != s) {
		m_state_condition.wait();
	}
}

Thread::State Thread::waitForRequest(const Thread::State &old) const {
	MutexGuard g(m_state_condition);

	// wait for some state change away from READY before we actually run
	while (getRequestedState() == old) {
		m_state_condition.wait();
	}

	return m_req_state;
}

Thread::State Thread::enterPause() {
	if (!callerIsThread()) {
		cosmos_throw (UsageError("Foreign thread called"));
	}

	stateEntered(PAUSE);

	auto ret = waitForRequest(PAUSE);

	stateEntered(RUN);

	return ret;
}

Thread::ID Thread::getCallerID() {
	return ID(pthread_self());
}

bool Thread::ID::operator==(const Thread::ID &other) const {
	return pthread_equal(this->m_id, other.m_id) != 0;
}

} // end ns
