#include <iostream>

#include "cosmos/time/Clock.hxx"

int main() {
	cosmos::MonotonicStopWatch watch(cosmos::MonotonicStopWatch::InitialMark(true));

	std::cerr << "time elapsed: " << watch.elapsed().count() << "ms" << std::endl;

	return 0;
}
