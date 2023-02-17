// Linux
#include <iostream>

// comos
#include "cosmos/io/Poller.hxx"
#include "cosmos/time/time.hxx"
#include "cosmos/time/TimerFD.hxx"

int main() {
	using TimerFD = cosmos::MonotonicTimerFD;
	TimerFD tfd;

	if (tfd.valid()) {
		std::cerr << "empty timer fd is valid?!\n";
		return 1;
	}

	tfd = TimerFD{TimerFD::defaults};

	if (!tfd.valid()) {
		std::cerr << "default settings timer fd is invalid?\n";
		return 1;
	}

	tfd.close();

	if (tfd.valid()) {
		std::cerr << "timer fd still valid after close?\n";
		return 1;
	}

	tfd = TimerFD{TimerFD::CreateFlags{TimerFD::CreateSettings::NONBLOCK}};

	if (!tfd.valid()) {
		std::cerr << "timer fd with custom settings is invalid?\n";
		return 1;
	}

	tfd.close();

	tfd.create();

	if (!tfd.valid()) {
		std::cerr << "create() didn't create valid timer fd?\n";
		return 1;
	}

	tfd.create();

	if (!tfd.valid()) {
		std::cerr << "second create() didn't create valid timer fd?\n";
		return 1;
	}

	tfd.close();
	tfd.close();

	if (tfd.valid()) {
		std::cerr << "double close resulted in valid timer fd?\n";
	}

	tfd.create();

	TimerFD::TimerSpec ts;
	ts.initial().setSeconds(2);

	tfd.setTime(ts);

	uint64_t ticks = tfd.wait();

	if (ticks != 1) {
		std::cerr << "wait() did not result in a tick?\n";
		return 1;
	}

	cosmos::Poller poller{16};
	poller.addFD(tfd.fd(), cosmos::Poller::MonitorMask{cosmos::Poller::MonitorSetting::INPUT});

	auto events = poller.wait(std::chrono::milliseconds{5000});

	if (!events.empty()) {
		std::cerr << "timer without interval re-ticked?!\n";
		return 1;
	}

	ts.interval().setSeconds(1);

	tfd.setTime(ts);

	cosmos::time::sleep(std::chrono::milliseconds{3000});

	ticks = tfd.wait();

	if (ticks < 2) {
		std::cerr << "timer with interval ticked less than two times?\n";
		return 1;
	}

	poller.delFD(tfd.fd());
	tfd.close();
	tfd.create();
	poller.addFD(tfd.fd(), cosmos::Poller::MonitorMask{cosmos::Poller::MonitorSetting::INPUT});

	tfd.setTime(ts);
	tfd.disarm();
	events = poller.wait(std::chrono::milliseconds{3000});

	if (!events.empty()) {
		std::cerr << "disarmed timer still ticked?\n";
		return 1;
	}

	return 0;
}
