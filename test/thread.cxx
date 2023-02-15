// stdlib
#include <iostream>
#include <thread>
#include <chrono>


// cosmos
#include "cosmos/cosmos.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/thread/PosixThread.hxx"
#include "cosmos/thread/pthread.hxx"
#include "cosmos/thread/thread.hxx"

using ThreadArg = cosmos::pthread::ThreadArg;
using ExitValue = cosmos::pthread::ExitValue;

class ThreadTest {
public:
	int run() {
		emptyTest();
		simpleTest();
		exitTest();
		normalEntryTest();
		tryJoinTest();
		timedJoinTest();
		detachedTest();
		return m_res;
	}

protected: // functions

	void emptyTest() {
		cosmos::PosixThread thread;
		if (thread.joinable()) {
			complain("empty thread is joinable?!");
		}
	}

	void simpleTest() {
		m_was_running = false;
		cosmos::PosixThread thread{{std::bind(&ThreadTest::simpleEntry, this, std::placeholders::_1)}, ThreadArg{815}, "simplethread"};
		auto tid = thread.id();

		if (tid == cosmos::pthread::get_id()) {
			complain("main thread equals new thread?!");
		}

		if (!thread.joinable()) {
			complain("My thread isn't joinable?!");
		}

		if (thread.isCallerThread()) {
			complain("I am my own thread?!");
		}

		if (thread.name().find("simplethread") == std::string::npos) {
			complain("my thread lost its name?!");
		}

		auto res = thread.join();

		auto tid2 = cosmos::pthread::ID{static_cast<pthread_t>(res)};

		if (tid != tid2) {
			complain("thread ID comparison failed");
		} else {
			praise("thread IDs match");
		}

		if (!m_was_running) {
			complain("thread didn't actually run?!");
		}
	}

	void exitTest() {
		cosmos::PosixThread thread{{std::bind(&ThreadTest::exitEntry, this, std::placeholders::_1)}, ThreadArg{4711}, "exitthread"};
		
		auto res = thread.join();

		if (res != ExitValue{4711}) {
			complain("pthread::exit didn't work?!");
			m_res = 1;
		} else {
			praise("pthread::exit did the right thing");
		}
	}

	void normalEntryTest() {
		m_normal_thread = cosmos::PosixThread{{std::bind(&ThreadTest::normalEntry, this)}, "normal-thread"};

		auto res = m_normal_thread.join();

		if (res != ExitValue{0}) {
			complain("normal entry thread returned non-zero value?!");
		}

		cosmos::PosixThread lambda_thread{[]() { std::cout << "Hello from a lambda thread\n";}};
		lambda_thread.join();
	}

	void tryJoinTest() {
		cosmos::PosixThread thread{[]() {sleep(3);}};
		if (thread.tryJoin().has_value()) {
			complain("tryJoin() worked right away?!");
		} else {
			praise("first tryJoin() failed");
		}

		sleep(4);

		if (!thread.tryJoin()) {
			complain("tryJoin() didn't work in the end?!");
		} else {
			praise("final tryJoin() worked");
		}
	}

	void timedJoinTest() {
		auto clock = cosmos::PosixThread::Clock();
		cosmos::PosixThread thread{[]() {sleep(3);}};

		auto maxwait = clock.now() + cosmos::TimeSpec{std::chrono::milliseconds{1000}};

		if (thread.joinTimed(maxwait).has_value()) {
			complain("timedJoin() worked right away?!");
		} else {
			praise("first timedJoin() failed");
		}

		sleep(2);

		maxwait = clock.now() + cosmos::TimeSpec{std::chrono::milliseconds{1000}};

		if (!thread.joinTimed(maxwait)) {
			complain("timedJoin() didn't work in the end?!");
		} else {
			praise("final timedJoin() worked");
		}
	}

	void detachedTest() {
		m_was_running = false;
		{
			cosmos::PosixThread thread{[this]() { sleep(1); std::cout << "a detached thread\n"; m_was_running = true;}};
			thread.detach();

			if (thread.joinable()) {
				complain("detached thread is still joinable?!");
			}
		}

		sleep(2);

		if (!m_was_running) {
			complain("detached thread was not running?!");
		}
	}

	void complain(std::string_view text) {
		std::cerr << text << "\n";
		m_res = 1;
	}

	void praise(std::string_view text) {
		std::cout << text << "\n";
	}

	ExitValue simpleEntry(ThreadArg arg) {
		m_was_running = true;

		if (arg != ThreadArg{815}) {
			std::cerr << "Received unexpected thread argument\n";
			m_res = 1;
		} else {
			std::cout << "thread argument matches\n";
		}

		return ExitValue{(intptr_t)cosmos::pthread::get_id().raw()};
	}

	ExitValue exitEntry(ThreadArg arg) {
		cosmos::pthread::exit(static_cast<ExitValue>(arg));	
		return ExitValue{1234};
	}

	void normalEntry() {
		if (!m_normal_thread.isCallerThread()) {
			complain("I don't really belong to my thread object?!");
		} else {
			praise("I belong to my thread object");
		}
	}

protected:
	bool m_was_running = false;
	int m_res = 0;
	cosmos::PosixThread m_normal_thread;
};

int main() {
	cosmos::Init ci;

	auto mytid = cosmos::thread::get_tid();

	std::cout << "my TID is " << static_cast<pid_t>(mytid) << std::endl;
	if (!cosmos::thread::is_main_thread()) {
		std::cerr << "I'm not the main thread?!" << std::endl;
		return 1;
	} else {
		std::cout << "I am the main thread\n";
	}

	if (cosmos::pthread::get_id() != cosmos::pthread::get_id()) {
		std::cerr << "My own ID differs towards itself?!\n";
		return 1;
	} else {
		std::cout << "pthread::get_id() == pthread::get_id()\n";
	}

	ThreadTest tt;

	return tt.run();
}
