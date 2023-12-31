// C++
#include <iostream>
#include <cassert>

// cosmos
#include "cosmos/utils.hxx"

// Test
#include "TestBase.hxx"

class MiscTest :
		public cosmos::TestBase {

	void runTests() override {
		testRanges();
		testNumElements();
		testInList();
		testResGuard();
	}

	void testRanges() {
		START_TEST("in_range");

		RUN_STEP("in-range", cosmos::in_range(10, 5, 15));
		RUN_STEP("out-of-range", !cosmos::in_range(10, 15, 20));
		RUN_STEP("inclusiveness", cosmos::in_range(10, 10, 10));
		RUN_STEP("lower-border", cosmos::in_range(10, 10, 15));
		RUN_STEP("upper-border", cosmos::in_range(10, 5, 10));

		size_t unsig = 3;
		RUN_STEP("unsigned-out-of-range", !cosmos::in_range(unsig, 10, 20));
	}

	void testNumElements() {
		START_TEST("num_elements");
		const int ARR[5] = {1, 2, 3, 4, 5};
		RUN_STEP("correct-num-elements", cosmos::num_elements(ARR) == 5);
	}

	void testInList() {
		START_TEST("in_list");

		const int i = 5;

		RUN_STEP("in-list", cosmos::in_list(i, {1, 5, 20}));
		RUN_STEP("not-in-list", !cosmos::in_list(i, {1, 20}));
	}

	struct CharPtrGuard :
			public cosmos::ResourceGuard<char*> {
		explicit CharPtrGuard(char *p) :
			ResourceGuard(p, [](char *_p) { delete[] _p; })
		{}
	};

	void testResGuard() {
		START_TEST("resource guard");

		{
			char *stuff = new char[500];
			CharPtrGuard stuff_guard(stuff);
			RUN_STEP("auto-delete", true);
		}

		{
			char *stuff = new char[500];
			CharPtrGuard stuff_guard(stuff);
			stuff_guard.disarm();
			delete[] stuff;
			RUN_STEP("manual-delete", true);
		}
	}
};

int main(const int argc, const char **argv) {
	MiscTest test;
	return test.run(argc, argv);
}
