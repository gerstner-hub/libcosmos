// C++
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <variant>

// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/thread/PosixThread.hxx"

using namespace cosmos::pthread;

namespace cosmos {

namespace pthread {

bool ID::operator==(const ID &other) const {
	return ::pthread_equal(this->m_id, other.m_id) != 0;
}

ID get_id() {
	return ID{::pthread_self()};
}

/// Ends the calling thread immediately
void exit(const ExitValue val) {
	::pthread_exit(reinterpret_cast<void*>(val));
	// should never happen
	std::abort();
}

} // end ns pthread

namespace {

	std::atomic<size_t> num_threads = 0;

	struct Context {
		std::variant<PosixThread::PosixEntry, PosixThread::Entry> entry;
		ThreadArg arg;
	};

	Context fetch_context(void *par) {
		auto ctx = reinterpret_cast<Context*>(par);
		auto ret = *ctx;
		delete ctx;
		return ret;
	}

	void* thread_entry(void *par) {

		Context ctx = fetch_context(par);
			
		if (std::holds_alternative<PosixThread::PosixEntry>(ctx.entry)) {
			auto entry = std::get<PosixThread::PosixEntry>(ctx.entry);
			auto ret = entry(ctx.arg);
			return reinterpret_cast<void*>(ret);
		} else {
			auto entry = std::get<PosixThread::Entry>(ctx.entry);
			entry();
			return nullptr;
		}
	}

	void create_thread(pthread_t &thread, Context *ctx) {

		const auto error = ::pthread_create(
			&thread,
			nullptr /* keep default attributes */,
			&thread_entry,
			reinterpret_cast<void*>(ctx)
		);

		if (error != 0) {
			cosmos_throw (cosmos::ApiError("Unable to create pthread"));
		}

	}
} // end anon ns

PosixThread::PosixThread(PosixEntry entry, pthread::ThreadArg arg, const std::string_view name) :
		m_name{buildName(name, ++num_threads)} {
	
	pthread_t thread;
	create_thread(thread, new Context{entry, arg});
	m_pthread = thread;
}

PosixThread::PosixThread(Entry entry, const std::string_view name) :
		m_name{buildName(name, ++num_threads)} {
	pthread_t thread;
	create_thread(thread, new Context{entry, pthread::ThreadArg{0}});
	m_pthread = thread;
}

PosixThread::PosixThread(PosixThread &&other) noexcept {
	*this = std::move(other);
}

PosixThread::~PosixThread() {
	if (joinable()) {
		std::cerr << "[libcosmos] FATAL: Thread " << name() << " destroyed but not joined!\n";
		std::abort();
	}
}

PosixThread& PosixThread::operator=(PosixThread &&other) noexcept {
	if (joinable()) {
		std::cerr << "[libcosmos] FATAL: moving into not-yet-joined thread\n";
		std::abort();
	}
	m_name = other.m_name;
	m_pthread = *(other.m_pthread);

	other.m_pthread.reset();
	other.m_name.clear();
	return *this;
}

void PosixThread::assertJoinConditions() {
	if (!joinable()) {
		cosmos_throw (UsageError("Attempted to join non-joinable thread (empty or detached)"));
	} else if (isCallerThread()) {
		cosmos_throw (UsageError("Attempted to join self"));
	}
}

void PosixThread::reset() {
	if (m_pthread) {
		num_threads--;
		m_pthread.reset();
		m_name.clear();
	}
}

pthread::ExitValue PosixThread::join() {
	assertJoinConditions();

	void *res = nullptr;
	const auto join_res = ::pthread_join(*m_pthread, &res);

	if (join_res != 0) {
		cosmos_throw (ApiError("Failed to join pthread"));
	}

	reset();

	return pthread::ExitValue{reinterpret_cast<intptr_t>(res)};
}

std::optional<pthread::ExitValue> PosixThread::tryJoin() {
	assertJoinConditions();

	void *res = nullptr;
	const auto join_res = ::pthread_tryjoin_np(*m_pthread, &res);

	if (auto err = Errno{join_res}; err != Errno::NO_ERROR) {
		if (err == Errno::BUSY) {
			// cannot be joined yet
			return {};
		}

		cosmos_throw (ApiError(err));
	}

	reset();

	return pthread::ExitValue{reinterpret_cast<intptr_t>(res)};
}

std::optional<pthread::ExitValue> PosixThread::joinTimed(const RealTime ts) {
	assertJoinConditions();

	void *res = nullptr;
	const auto join_res = ::pthread_timedjoin_np(*m_pthread, &res, &ts);

	if (auto err = Errno{join_res}; err != Errno::NO_ERROR) {
		if (err == Errno::TIMEDOUT) {
			// couldn't join in time
			return {};
		}

		cosmos_throw (ApiError("Failed to join pthread"));
	}

	reset();

	return pthread::ExitValue{reinterpret_cast<intptr_t>(res)};
}

void PosixThread::detach() {
	// NOTE: in theory it is valid that a thread detaches itself, so let's
	// not use assertJoinConditions() here ATM.
	if (!joinable()) {
		cosmos_throw (UsageError("Attempted to detach a non-joinable thread (empty or already detached)"));
	}

	const auto res = pthread_detach(*m_pthread);

	if (res != 0) {
		cosmos_throw (ApiError("Failed to detach pthread"));
	}

	reset();
}

std::string PosixThread::buildName(const std::string_view name, size_t nr) const {
	if (!name.empty())
		return std::string{name};
	return std::string{"thread<" + std::to_string(nr) + ">"};
}

} // end ns
