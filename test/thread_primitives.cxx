// C++
#include <iostream>

// Cosmos
#include <cosmos/cosmos.hxx>
#include <cosmos/thread/Mutex.hxx>
#include <cosmos/thread/Condition.hxx>
#include <cosmos/time/Clock.hxx>

// Test
#include "TestBase.hxx"

class ThreadPrimTest :
		public cosmos::TestBase {

	void runTests() override {
		START_TEST("thread primitives");
		/*
		 * lacking actual threads yet this is a bit of a over
		 * simplified test, but still better than nothing.
		 */
		cosmos::Mutex lock;

		lock.lock();
		lock.unlock();

		cosmos::Condition cond(lock);
		cond.signal();
		cond.broadcast();

		cosmos::ConditionMutex condmux;
		cosmos::MonotonicClock clock;
		auto starttime = clock.now();
		auto endtime = starttime + cosmos::MonotonicTime{5};

		condmux.lock();
		auto wait_res = condmux.waitTimed(endtime);

		RUN_STEP("timedout-no-signaled", wait_res == cosmos::Condition::WaitTimedRes::TIMED_OUT);
		condmux.unlock();

		auto time_spent = clock.now() - starttime;

		RUN_STEP("enough-time-spent-in-wait", time_spent.getSeconds() >= 5);
		// be generous with the upper limit
		RUN_STEP("returned-from-wait-in-time", time_spent.getSeconds() <= 60);
	}
};

int main(const int argc, const char **argv) {
	ThreadPrimTest test;
	return test.run(argc, argv);
}
