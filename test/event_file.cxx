// C++
#include <iostream>

// cosmos
#include "cosmos/io/EventFile.hxx"

// Test
#include "TestBase.hxx"

class EventFileTest :
		public cosmos::TestBase {

	void runTests() override {
		testRegularEvents();
		testSemaphoreEvents();
		testNonblockingEvents();
	}

	void testRegularEvents() {
		START_TEST("testing regular event I/O");
		constexpr auto count = cosmos::EventFile::Counter{50};
		cosmos::EventFile ef;
		ef.signal(count);
		const auto retcount = ef.wait();

		RUN_STEP("returned count matches", retcount == count);
	}

	void testSemaphoreEvents() {
		START_TEST("testing semaphore style event I/O");
		cosmos::EventFile ef{
			cosmos::EventFile::Counter{0},
			cosmos::EventFile::Flags{cosmos::EventFile::Flag::SEMAPHORE}};
		ef.signal(cosmos::EventFile::Counter{50});
		RUN_STEP("semaphore wait returns only one", ef.wait() == cosmos::EventFile::Counter{1});
		RUN_STEP("semaphore wait returns only one", ef.wait() == cosmos::EventFile::Counter{1});
	}

	void testNonblockingEvents() {
		START_TEST("testing non-blocking event I/O");
		constexpr auto INITCOUNT = cosmos::EventFile::Counter{50};
		cosmos::EventFile ef{
			INITCOUNT,
			cosmos::EventFile::Flags{cosmos::EventFile::Flag::NONBLOCK}};

		auto retcount = ef.wait();
		RUN_STEP("returned initcount matches", retcount == INITCOUNT);

		try {
			retcount = ef.wait();
		} catch (const cosmos::ApiError &ex) {
			RUN_STEP("nonblocking-wait causes EAGAIN", ex.errnum() == cosmos::Errno::AGAIN);
		}
	}
};

int main(const int argc, const char **argv) {
	EventFileTest test;
	return test.run(argc, argv);
}
