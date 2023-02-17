#include <chrono>
#include <iostream>

#include "cosmos/error/ApiError.hxx"
#include "cosmos/time/Clock.hxx"
#include "cosmos/time/StopWatch.hxx"
#include "cosmos/time/time.hxx"

int main() {
	cosmos::MonotonicStopWatch watch(cosmos::MonotonicStopWatch::InitialMark(true));

	std::cerr << "time elapsed: " << watch.elapsed().count() << "ms" << std::endl;

	cosmos::MonotonicClock mclock;

	auto pre_sleep = mclock.now();
	auto sleep_end = pre_sleep + cosmos::MonotonicTime{std::chrono::milliseconds{500}};
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

	auto timeres = mclock.resolution();

	std::cout << "monotonic clock resolution:\n";
	std::cout << timeres.getSeconds() << "s " << timeres.getNanoseconds() << "ns\n";

	try {
		mclock.setTime(pre_sleep);
	} catch (const cosmos::ApiError &e) {
		std::cerr << "failed to set monotonic clock: " << e.what() << std::endl;
	}

	return 0;
}
