// C++
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <variant>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/UsageError.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/private/cosmos.hxx>
#include <cosmos/thread/PosixThread.hxx>

using namespace cosmos::pthread;

namespace cosmos {

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

		const auto res = ::pthread_create(
			&thread,
			nullptr /* keep default attributes */,
			&thread_entry,
			reinterpret_cast<void*>(ctx)
		);

		if (const auto error = Errno{res}; error != Errno::NO_ERROR) {
			throw cosmos::ApiError("pthread_create()", error);
		}

	}
} // end anon ns

PosixThread::PosixThread(PosixEntry entry, pthread::ThreadArg arg, const std::string_view name) :
		m_name{buildName(name, ++num_threads)} {

	m_pthread = pthread_t{};
	try {
		create_thread(m_pthread.value(), new Context{entry, arg});
	} catch(...) {
		m_pthread.reset();
	}
}

PosixThread::PosixThread(Entry entry, const std::string_view name) :
		m_name{buildName(name, ++num_threads)} {
	m_pthread = pthread_t{};
	try {
		create_thread(m_pthread.value(), new Context{entry, pthread::ThreadArg{0}});
	} catch(...) {
		m_pthread.reset();
	}
}

PosixThread::PosixThread(PosixThread &&other) noexcept {
	*this = std::move(other);
}

PosixThread::~PosixThread() {
	if (joinable()) {
		fatal_error(sprintf("Thread %s destroyed but not joined!", name().c_str()));
	}
}

PosixThread& PosixThread::operator=(PosixThread &&other) noexcept {
	if (joinable()) {
		fatal_error("moving into not-yet-joined thread");
	}
	m_name = other.m_name;
	m_pthread = *(other.m_pthread);

	other.m_pthread.reset();
	other.m_name.clear();
	return *this;
}

void PosixThread::assertJoinConditions() {
	if (!joinable()) {
		throw UsageError{"Attempted to join non-joinable thread (empty or detached)"};
	} else if (isCallerThread()) {
		throw UsageError{"Attempted to join self"};
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

	if (const auto error = Errno{join_res}; error != Errno::NO_ERROR) {
		throw ApiError{"pthread_join()", error};
	}

	reset();

	return pthread::ExitValue{reinterpret_cast<intptr_t>(res)};
}

std::optional<pthread::ExitValue> PosixThread::tryJoin() {
	assertJoinConditions();

	void *res = nullptr;
	const auto join_res = ::pthread_tryjoin_np(*m_pthread, &res);

	if (const auto error = Errno{join_res}; error != Errno::NO_ERROR) {
		if (error == Errno::BUSY) {
			// cannot be joined yet
			return {};
		}

		throw ApiError{"pthread_tryjoin_np()", error};
	}

	reset();

	return pthread::ExitValue{reinterpret_cast<intptr_t>(res)};
}

std::optional<pthread::ExitValue> PosixThread::joinTimed(const RealTime ts) {
	assertJoinConditions();

	void *res = nullptr;
	const auto join_res = ::pthread_timedjoin_np(*m_pthread, &res, &ts);

	if (const auto error = Errno{join_res}; error != Errno::NO_ERROR) {
		if (error == Errno::TIMEDOUT) {
			// couldn't join in time
			return {};
		}

		throw ApiError{"pthread_timedjoin_np()", error};
	}

	reset();

	return pthread::ExitValue{reinterpret_cast<intptr_t>(res)};
}

void PosixThread::detach() {
	// NOTE: in theory it is valid that a thread detaches itself, so let's
	// not use assertJoinConditions() here ATM.
	if (!joinable()) {
		throw UsageError{"Attempted to detach a non-joinable thread (empty or already detached)"};
	}

	const auto res = ::pthread_detach(*m_pthread);

	if (const auto error = Errno{res}; error != Errno::NO_ERROR) {
		throw ApiError{"pthread_detach()", error};
	}

	reset();
}

std::string PosixThread::buildName(const std::string_view name, size_t nr) const {
	if (!name.empty())
		return std::string{name};
	return std::string{"thread<" + std::to_string(nr) + ">"};
}

} // end ns
