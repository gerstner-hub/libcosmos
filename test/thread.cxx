// C++
#include <iostream>
#include <thread>
#include <chrono>

// cosmos
#include "cosmos/cosmos.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/thread/PosixThread.hxx"
#include "cosmos/thread/pthread.hxx"
#include "cosmos/thread/thread.hxx"
#include "cosmos/time/Clock.hxx"

// Test
#include "TestBase.hxx"

using ThreadArg = cosmos::pthread::ThreadArg;
using ExitValue = cosmos::pthread::ExitValue;

class ThreadTest :
		public cosmos::TestBase {

	void runTests() override {
		testIDs();
		emptyTest();
		simpleTest();
		exitTest();
		normalEntryTest();
		tryJoinTest();
		timedJoinTest();
		detachedTest();
	}

protected: // functions

	void testIDs() {
		START_TEST("thread ids");
		auto mytid = cosmos::thread::get_tid();

		std::cout << "my TID is " << static_cast<pid_t>(mytid) << std::endl;
		RUN_STEP("verify-main-thread", cosmos::thread::is_main_thread());

		RUN_STEP("pthread-id-equals", cosmos::pthread::get_id() == cosmos::pthread::get_id());
	}

	void emptyTest() {
		START_TEST("empty thread");
		cosmos::PosixThread thread;
		RUN_STEP("empty-not-joinable", !thread.joinable());
	}

	void simpleTest() {
		START_TEST("simple thread");
		m_was_running = false;
		cosmos::PosixThread thread{
			{std::bind(&ThreadTest::simpleEntry, this, std::placeholders::_1)},
				ThreadArg{815}, "simplethread"};
		auto tid = thread.id();

		RUN_STEP("has-different-id", tid != cosmos::pthread::get_id());
		RUN_STEP("is-joinable", thread.joinable());
		RUN_STEP("us-not-the-thread", !thread.isCallerThread());
		RUN_STEP("proper-name", thread.name().find("simplethread") != std::string::npos);

		auto res = thread.join();

		auto tid2 = cosmos::pthread::ID{static_cast<pthread_t>(res)};

		RUN_STEP("returned-it-matches", tid == tid2);

		RUN_STEP("thread-was-running", m_was_running);
	}

	void exitTest() {
		START_TEST("exit test");
		cosmos::PosixThread thread{
			{std::bind(&ThreadTest::exitEntry, this, std::placeholders::_1)},
				ThreadArg{4711}, "exitthread"};
		
		auto res = thread.join();

		RUN_STEP("exit-value-matches", res == ExitValue{4711});
	}

	void normalEntryTest() {
		START_TEST("normal thread");
		m_normal_thread = cosmos::PosixThread{
			{std::bind(&ThreadTest::normalEntry, this)}, "normal-thread"};

		auto res = m_normal_thread.join();

		RUN_STEP("zero-exit-value", res == ExitValue{0});

		cosmos::PosixThread lambda_thread{[]() { std::cout << "Hello from a lambda thread\n";}};
		lambda_thread.join();
	}

	void tryJoinTest() {
		START_TEST("try join");
		cosmos::PosixThread thread{[]() {sleep(3);}};
		RUN_STEP("immediate-join-fails", !thread.tryJoin().has_value());

		sleep(4);

		RUN_STEP("late-join-succeeds", thread.tryJoin().has_value());
	}

	void timedJoinTest() {
		START_TEST("timed join");
		auto clock = cosmos::RealTimeClock{};
		cosmos::PosixThread thread{[]() {sleep(3);}};

		auto maxwait = clock.now() + cosmos::RealTime{std::chrono::milliseconds{1000}};

		RUN_STEP("immediate-join-fails", !thread.joinTimed(maxwait).has_value());

		sleep(2);

		maxwait = clock.now() + cosmos::RealTime{std::chrono::milliseconds{1000}};

		RUN_STEP("late-join-succeeds", thread.joinTimed(maxwait).has_value());
	}

	void detachedTest() {
		START_TEST("detached thread");
		m_was_running = false;
		{
			cosmos::PosixThread thread{
				[this]() { sleep(1); std::cout << "a detached thread\n"; m_was_running = true;}
			};
			thread.detach();

			RUN_STEP("detached-not-joinable", !thread.joinable());
		}

		sleep(2);

		RUN_STEP("detached-was-still-running", m_was_running);
	}

	ExitValue simpleEntry(ThreadArg arg) {
		m_was_running = true;

		if (arg != ThreadArg{815}) {
			std::cerr << "Received unexpected thread argument\n";
			finishTest(false);
		}

		return ExitValue{(intptr_t)cosmos::pthread::get_id().raw()};
	}

	ExitValue exitEntry(ThreadArg arg) {
		cosmos::pthread::exit(static_cast<ExitValue>(arg));	
		return ExitValue{1234};
	}

	void normalEntry() {
		RUN_STEP("thread-is-caller-thread", m_normal_thread.isCallerThread());
	}

protected:
	bool m_was_running = false;
	cosmos::PosixThread m_normal_thread;
};

int main(const int argc, const char **argv) {
	ThreadTest test;
	return test.run(argc, argv);
}
