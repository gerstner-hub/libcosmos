// C++
#include <chrono>
#include <iostream>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/time/Clock.hxx>
#include <cosmos/time/StopWatch.hxx>
#include <cosmos/time/time.hxx>

// Test
#include "TestBase.hxx"

class TimeTest :
		public cosmos::TestBase {

	void runTests() override {
		testWatch();
		testClock();
	}

	void testWatch() {
		START_TEST("stopwatch");
		cosmos::MonotonicStopWatch watch{cosmos::MonotonicStopWatch::InitialMark(true)};

		std::cout << "time elapsed: " << watch.elapsed().count() << "ms" << std::endl;
	}

	void testClock() {
		START_TEST("clock2");
		cosmos::MonotonicClock mclock;

		auto pre_sleep = mclock.now();
		auto sleep_end = pre_sleep + cosmos::MonotonicTime{std::chrono::milliseconds{500}};
		mclock.sleep(sleep_end);

		RUN_STEP("abs-sleep-long-enough", mclock.now() >= sleep_end);

		pre_sleep = mclock.now();

		cosmos::time::sleep(std::chrono::milliseconds{500});

		sleep_end = mclock.now();
		auto diff_time = sleep_end - pre_sleep;

		RUN_STEP("rel-sleep-long-enough", diff_time.toMilliseconds() >= 500);

		auto timeres = mclock.resolution();

		std::cout << "monotonic clock resolution:\n";
		std::cout << timeres.getSeconds() << "s " << timeres.getNanoSeconds() << "ns\n";

		EXPECT_EXCEPTION("setting-monoclock-fails", mclock.setTime(pre_sleep));
	}
};

int main(const int argc, const char **argv) {
	TimeTest  test;
	return test.run(argc, argv);
}
