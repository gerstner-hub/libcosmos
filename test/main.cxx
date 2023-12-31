//  cosmos
#include "cosmos/main.hxx"

// Test
#include "TestBase.hxx"

class MainTest :
		public cosmos::TestBase {
	
	void runTests() override {
		testArgvCorrect();
		testExitByReturn();
		testExitByThrowStatus();
		testExitByException();
		testExitByCtorException();
	}

	void testArgvCorrect() {
		class ArgvMain :
				public cosmos::MainContainerArgs {
		protected:
			cosmos::ExitStatus main(const std::string_view argv0, const cosmos::StringViewVector &args) override {
				if (argv0 != "prog") {
					return cosmos::ExitStatus::FAILURE;
				}

				if (args.size() != 2) {
					return cosmos::ExitStatus::FAILURE;
				} else if (args[0] != "first") {
					return cosmos::ExitStatus::FAILURE;
				} else if (args[1] != "second") {
					return cosmos::ExitStatus::FAILURE;
				}

				return cosmos::ExitStatus::SUCCESS;
			}
		};

		START_TEST("test-argv-correct");

		const char *args[] = {"prog", "first", "second"};

		int res = cosmos::main<ArgvMain>(cosmos::num_elements(args), args);
		RUN_STEP("argv-test-succeeded", res == 0);
	}

	void testExitByReturn() {
		class ReturningMain :
				public cosmos::MainNoArgs {
		protected:
			cosmos::ExitStatus main() override {
				return cosmos::ExitStatus{5};
			}
		};

		START_TEST("test-exit-by-return");

		int res = cosmos::main<ReturningMain>(0, nullptr);
		RUN_STEP("exit-code-matches", res == 5);
	}

	void testExitByThrowStatus() {
		class ThrowStatusMain :
				public cosmos::MainNoArgs {
		protected:
			cosmos::ExitStatus main() override {
				throw cosmos::ExitStatus{10};
			}
		};

		START_TEST("test-exit-by-throw-status");

		int res = cosmos::main<ThrowStatusMain>(0, nullptr);
		RUN_STEP("exit-code-matches", res == 10);
	}

	void testExitByException() {
		class ThrowingMain :
				public cosmos::MainNoArgs {
		protected:
			cosmos::ExitStatus main() override {
				cosmos_throw(cosmos::ApiError(cosmos::Errno::PERMISSION));
				return cosmos::ExitStatus::SUCCESS;
			}
		};

		START_TEST("test-exit-by-throw-exception");

		int res = cosmos::main<ThrowingMain>(0, nullptr);
		RUN_STEP("exit-code-matches", res != 0);
	}

	void testExitByCtorException() {
		class CtorThrowingMain :
				public cosmos::MainNoArgs {
		public:
			CtorThrowingMain() {
				cosmos_throw(cosmos::ApiError(cosmos::Errno::RANGE));
			}
		protected:
			cosmos::ExitStatus main() override {
				return cosmos::ExitStatus::SUCCESS;
			}
		};

		START_TEST("test-exit-by-throw-in-ctor");

		int res = cosmos::main<CtorThrowingMain>(0, nullptr);
		RUN_STEP("exit-code-matches", res != 0);

	}
};

int main(const int argc, const char **argv) {
	MainTest test;
	return test.run(argc, argv);
}
