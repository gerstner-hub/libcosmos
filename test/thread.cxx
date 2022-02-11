// stdlib
#include <iostream>
#include <thread>
#include <chrono>


// cosmos
#include "cosmos/thread/Thread.hxx"

class ThreadUser :
	public cosmos::IThreadEntry
{
public:
	bool wasRunning() const { return m_was_running; }
protected:
	void threadEntry(cosmos::Thread &t) override {
		m_was_running = true;

		if (t.getID() != t.getCallerID()) {
			std::cerr << "thread ID comparison failed" << std::endl;
			abort();
		}

		while (t.enterPause() == t.RUN) {
			std::cout << "thread " << t.name() << " running" << std::endl;
			std::this_thread::sleep_for(std::chrono::microseconds(500));
		}

	}
protected:
	bool m_was_running = false;
};

int main()
{
	ThreadUser tu;
	int res = 0;

	{
		cosmos::Thread th(tu, "testthread");

		if (th.name() != "testthread") {
			std::cerr << "th.name() returned unexpected value" << std::endl;
			res = 1;
		}

		if (th.getCurState() != cosmos::Thread::READY) {
			std::cerr << "initial thread state is unexpected" << std::endl;
			res = 1;
		}

		th.requestExit();
		th.join();

		if (tu.wasRunning() != false) {
			std::cerr << "thread running flag has unexpected value" << std::endl;
			res = 1;
		}

		if (th.getCurState() != cosmos::Thread::DEAD) {
			std::cerr << "joined thread has unexpected state" << std::endl;
			res = 1;
		}
	}

	cosmos::Thread th2(tu);

	if (th2.getID() == cosmos::Thread::getCallerID()) {
		std::cerr << "thread unexpectedly equals main thread!" << std::endl;
		res = 1;
	}

	th2.start();

	th2.waitForState(th2.RUN);

	if (th2.getCurState() != cosmos::Thread::RUN) {
		std::cerr << "running thread has unexpected state" << std::endl;
		res = 1;
	}

	th2.requestPause();
	th2.waitForState(th2.PAUSE);

	if (th2.getCurState() != cosmos::Thread::PAUSE) {
		std::cerr << "pausing thread has unexpected state" << std::endl;
		res = 1;
	}

	th2.requestExit();

	th2.join();

	if (tu.wasRunning() != true) {
		std::cerr << "thread running flag has unexpected value (2)" << std::endl;
		res = 1;
	}

	return res;
}
