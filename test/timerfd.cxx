// C++
#include <iostream>

// cosmos
#include "cosmos/io/Poller.hxx"
#include "cosmos/time/time.hxx"
#include "cosmos/time/TimerFD.hxx"

// Test
#include "TestBase.hxx"

class TimerFdTest :
		public cosmos::TestBase {

	using TimerFD = cosmos::MonotonicTimerFD;

	void runTests() override {
		testValidity();
		testTicks();
	}

	void testValidity() {
		START_TEST("validity");
		TimerFD tfd;

		RUN_STEP("default-not-valid", !tfd.isOpen());

		tfd = TimerFD{TimerFD::defaults};

		RUN_STEP("defsettings-valid", tfd.isOpen());

		tfd.close();

		RUN_STEP("invalid-after-close", !tfd.isOpen());

		tfd = TimerFD{TimerFD::CreateFlags{TimerFD::CreateSettings::NONBLOCK}};

		RUN_STEP("custom-settings-valid", tfd.isOpen());

		tfd.close();

		tfd.create();

		RUN_STEP("create-valid", tfd.isOpen());

		tfd.create();

		RUN_STEP("double-create-valid", tfd.isOpen());

		tfd.close();
		tfd.close();

		RUN_STEP("double-close-invalid", !tfd.isOpen());
	}

	void testTicks() {
		START_TEST("ticks");

		TimerFD tfd;
		tfd.create();

		TimerFD::TimerSpec ts;
		ts.initial().setSeconds(2);

		tfd.setTime(ts);

		uint64_t ticks = tfd.wait();

		RUN_STEP("wait-for-initial-ticks", ticks == 1);

		cosmos::Poller poller{16};
		poller.addFD(tfd.fd(), cosmos::Poller::MonitorMask{cosmos::Poller::MonitorSetting::INPUT});

		auto events = poller.wait(std::chrono::milliseconds{5000});

		RUN_STEP("no-interval-no-retick", events.empty());

		ts.interval().setSeconds(1);

		tfd.setTime(ts);

		cosmos::time::sleep(std::chrono::milliseconds{3000});

		ticks = tfd.wait();

		RUN_STEP("yes-interval-retick", ticks >= 2);

		poller.delFD(tfd.fd());
		tfd.close();
		tfd.create();
		poller.addFD(tfd.fd(), cosmos::Poller::MonitorMask{cosmos::Poller::MonitorSetting::INPUT});

		tfd.setTime(ts);
		tfd.disarm();
		events = poller.wait(std::chrono::milliseconds{3000});

		RUN_STEP("disarm-stops-tick", events.empty());
	}
};

int main(const int argc, const char **argv) {
	TimerFdTest test;
	return test.run(argc, argv);
}
