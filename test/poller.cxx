// C++
#include <iostream>

// cosmos
#include "cosmos/fs/FDFile.hxx"
#include "cosmos/io/Poller.hxx"
#include "cosmos/io/Pipe.hxx"

// Test
#include "TestBase.hxx"

class PollerTest :
		public cosmos::TestBase {

	void runTests() override {
		testCreateClose();
		testBasicPoll();
	}

	void testCreateClose() {
		START_TEST("create/close");
		// this tests for the robustness of double-create() and double-close()
		cosmos::Poller poller;

		RUN_STEP("default-invalid", !poller.valid());

		poller.create();

		RUN_STEP("created-valid", poller.valid());

		// check double-create
		poller.create();
		poller.close();

		RUN_STEP("closed-invalid", !poller.valid());

		poller.close();
	}

	void testBasicPoll() {
		START_TEST("basic polling");
		cosmos::Poller poller;
		poller.create();
		cosmos::Pipe pp;

		poller.addFD(pp.readEnd(), cosmos::Poller::MonitorFlags({cosmos::Poller::MonitorFlag::INPUT}));

		auto ready = poller.wait(std::chrono::milliseconds(500));

		RUN_STEP("verify-no-spurious-event", ready.empty());

		cosmos::FDFile pipe_write{pp.writeEnd(), cosmos::AutoCloseFD{false}};

		pipe_write.write("test", 4);

		ready = poller.wait();

		RUN_STEP("have-input-event", ready.size() == 1);

		{
			const auto &ev = ready[0];

			RUN_STEP("event-fd-matches", ev.fd() == pp.readEnd());
			RUN_STEP("is-input-ready", ev.getEvents().only(cosmos::Poller::Event::INPUT_READY));
		}

		pp.closeWriteEnd();

		ready = poller.wait();

		RUN_STEP("close-event-matches", ready.size() == 1);

		{
			const auto &ev = ready[0];

			RUN_STEP("event-fd-matches", ev.fd() == pp.readEnd());

			RUN_STEP("hangup-input-ready",
					ev.getEvents().allOf(
						{cosmos::Poller::Event::INPUT_READY,
						 cosmos::Poller::Event::HANGUP_OCCURED}
					);
			);
		}

		std::cout << "poller.wait() correctly returned event info" << std::endl;
	}
};

int main(const int argc, const char **argv) {
	PollerTest test;
	return test.run(argc, argv);
}
