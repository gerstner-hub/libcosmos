// Cosmos
#include "cosmos/thread/Mutex.hxx"
#include "cosmos/thread/Condition.hxx"
#include "cosmos/time/Clock.hxx"
#include "cosmos/Init.hxx"

// C++
#include <iostream>

int main()
{
	cosmos::Init init;

	try {
		/*
		 * lacking actualy threads yet this is a bit of a over
		 * simplified test, but still better than nothing.
		 */
		cosmos::Mutex lock;

		lock.lock();
		lock.unlock();

		cosmos::Condition cond(lock);
		cond.signal();
		cond.broadcast();

		cosmos::ConditionMutex condmux;
		cosmos::Condition::Clock clock;
		auto starttime = clock.now();
		auto endtime = starttime + cosmos::TimeSpec(5);

		condmux.lock();
		auto wait_res = condmux.waitTimed(endtime);

		if (wait_res != cosmos::Condition::WaitTimedRes::TIMED_OUT) {
			std::cerr << "got signaled?!" << std::endl;
			return 1;
		}
		condmux.unlock();

		auto time_spent = clock.now() - starttime;

		if (time_spent.getSeconds() < 5) {
			std::cerr << "spent not enough time in waitTimed()?! getSeconds() = " << time_spent.getSeconds() << std::endl;
			return 1;
		}
		// be generous with the upper limit
		else if (time_spent.getSeconds() > 60) {
			std::cerr << "spent too much time in waitTimed()?!" << std::endl;
		}
	}
	catch (const cosmos::CosmosError &ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}
}
