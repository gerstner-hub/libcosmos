#include <chrono>
#include <iostream>

#include "cosmos/time/Clock.hxx"
#include "cosmos/time/StopWatch.hxx"
#include "cosmos/time/time.hxx"

int main() {
	cosmos::MonotonicStopWatch watch(cosmos::MonotonicStopWatch::InitialMark(true));

	std::cerr << "time elapsed: " << watch.elapsed().count() << "ms" << std::endl;

	cosmos::MonotonicClock mclock;

	auto pre_sleep = mclock.now();
	auto sleep_end = pre_sleep + cosmos::TimeSpec{std::chrono::milliseconds{500}};
	mclock.sleep(sleep_end);

	if (mclock.now() < sleep_end) {
		std::cerr << "sleep time was shorter than requested?!\n";
		return 1;
	}

	pre_sleep = mclock.now();

	cosmos::time::sleep(std::chrono::milliseconds{500});

	sleep_end = mclock.now();
	auto diff_time = sleep_end - pre_sleep;

	if (diff_time.toMilliseconds() < 500) {
		std::cerr << "sleep time was shorter than requested (2)?! -> " << diff_time.toMilliseconds() << "\n";
		return 1;
	}

	return 0;
}
