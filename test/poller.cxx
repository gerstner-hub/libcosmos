#include <iostream>

#include "cosmos/io/Poller.hxx"
#include "cosmos/io/Pipe.hxx"
#include "cosmos/fs/StreamFile.hxx"

namespace {
	int res = 0;
}

void testCreateClose() {
	// this tests for the robustness of double-create() and double-close()
	cosmos::Poller poller;

	if (poller.valid()) {
		std::cerr << "poller already valid?!\n";
		res = 1;
	}

	poller.create();

	if (!poller.valid()) {
		std::cerr << "poller create() results in invalid?!\n";
		res = 1;
	}

	poller.create();
	poller.close();

	if (poller.valid()) {
		std::cerr << "closer poller still valid?!\n";
		res = 1;
	}

	poller.close();
}

bool testBasicPoll() {
	cosmos::Poller poller;
	poller.create();
	cosmos::Pipe pp;

	poller.addFD(pp.readEnd(), cosmos::Poller::MonitorMask({cosmos::Poller::MonitorSetting::INPUT}));
	
	auto ready = poller.wait(std::chrono::milliseconds(500));

	if (!ready.empty()) {
		std::cerr << "poller.wait() did not timeout as expected" << std::endl;
		return false;
	}

	cosmos::StreamFile pipe_write(pp.writeEnd(), cosmos::StreamFile::AutoClose(false));

	pipe_write.write("test", 4);

	ready = poller.wait();

	if (ready.size() != 1) {
		std::cerr << "poller.wait() did not return expected INPUT event" << std::endl;
		return false;
	}

	{
		const auto &ev = ready[0];

		if (ev.fd() != pp.readEnd()) {
			std::cerr << "poller.wait() returned bad FD in event" << std::endl;
			return false;
		}

		if (!ev.getEvents().only(cosmos::Poller::Event::INPUT_READY)) {
			std::cerr << "poller.wait() returned unexpected event" << std::endl;
			return false;
		}
	}

	pp.closeWriteEnd();

	ready = poller.wait();

	if (ready.size() != 1) {
		std::cerr << "poller.wait() did not return expected INPUT event" << std::endl;
		return false;
	}

	{
		const auto &ev = ready[0];

		if (ev.fd() != pp.readEnd()) {
			std::cerr << "poller.wait() returned bad FD in event" << std::endl;
			return false;
		}

		if (!ev.getEvents().allOf({cosmos::Poller::Event::INPUT_READY, cosmos::Poller::Event::HANGUP_OCCURED})) {
			std::cerr << "poller.wait() returned unexpected event" << std::endl;
			return false;
		}
	}

	std::cout << "poller.wait() correctly returned event info" << std::endl;

	return true;
}

int main() {
	testCreateClose();
	if (!testBasicPoll())
		res = 1;
	return res;
}
