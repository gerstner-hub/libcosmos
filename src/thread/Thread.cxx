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

Thread::Thread(IThreadEntry &entry, std::optional<const std::string_view> name) :
	m_state(READY),
	m_request(PAUSE),
	m_entry(entry),
	m_name(name ? *name : "thread<" + std::to_string(g_num_threads) + ">")
{
	const auto error = ::pthread_create(
		&m_pthread,
		nullptr /* keep default attributes */,
		&posixEntry,
		(void*)this /* pass pointer to object instance */
	);

	if (error != 0) {
		m_state = DEAD;
		cosmos_throw (cosmos::ApiError("Unable to create thread"));
	}

	g_num_threads++;
}

Thread::~Thread() {
	g_num_threads--;

	try {
		assert (m_state != RUNNING && m_state != PAUSED);

		if (m_state == READY) {
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

	const auto req = thread.waitForRequest(PAUSE);

	if (req == RUN) {
		thread.run();
	}

	thread.stateEntered(DEAD);

	return nullptr;
}

void Thread::run() {
	stateEntered(RUNNING);

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

	this->issueRequest(EXIT);

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
		if (m_state == s)
			return;
		m_state = s;
	}

	m_state_condition.broadcast();
}

void Thread::waitForState(const Thread::State &s) const {
	MutexGuard g(m_state_condition);

	while (m_state != s) {
		m_state_condition.wait();
	}
}

Thread::Request Thread::waitForRequest(const Thread::Request &old) const {
	MutexGuard g(m_state_condition);

	// wait for some state change away from READY before we actually run
	while (getRequest() == old) {
		m_state_condition.wait();
	}

	return m_request;
}

Thread::Request Thread::enterPause() {
	if (!callerIsThread()) {
		cosmos_throw (UsageError("Foreign thread called"));
	}

	stateEntered(PAUSED);

	auto ret = waitForRequest(PAUSE);

	stateEntered(RUNNING);

	return ret;
}

Thread::ID Thread::getCallerID() {
	return ID(pthread_self());
}

bool Thread::ID::operator==(const Thread::ID &other) const {
	return pthread_equal(this->m_id, other.m_id) != 0;
}

} // end ns
