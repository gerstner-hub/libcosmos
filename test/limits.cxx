// cosmos
#include <cosmos/proc/LimitSettings.hxx>
#include <cosmos/proc/limits.hxx>
#include <cosmos/proc/process.hxx>

// Test
#include "TestBase.hxx"

class LimitsTest :
		public cosmos::TestBase {
public:

	using LimitInt = cosmos::LimitSpec::LimitInt;

	void runTests() override {
		testOwnLimits();
		testChildLimits();
	}

	void printLimit(const cosmos::LimitSpec &spec) {
		if (const auto soft = spec.getSoftLimit(); soft == cosmos::LimitSpec::INFINITY) {
			std::cout << "INFINITY";
		} else {
			std::cout << soft;
		}

		std::cout << " / ";

		if (const auto hard = spec.getHardLimit(); hard == cosmos::LimitSpec::INFINITY) {
			std::cout << "INFINITY";
		} else {
			std::cout << hard;
		}
	}

	void testOwnLimits() {
		START_TEST("Test operating on own process resource limits");

		constexpr LimitInt SOFT_LIMIT = 1000;
		constexpr LimitInt HARD_LIMIT = 2000;

		cosmos::LimitSpec spec;
		cosmos::LimitSettings settings;
		spec.setSoftLimit(SOFT_LIMIT);
		spec.setHardLimit(HARD_LIMIT);
		spec = settings.setProcLimit(spec);

		std::cout << "old NPROC limits: ";
		printLimit(spec);
		spec = settings.getProcLimit();
		std::cout << "\n";
		std::cout << "new NPROC limits: ";
		printLimit(spec);
		std::cout << "\n";

		RUN_STEP("new-soft-limit-matches", spec.getSoftLimit() == SOFT_LIMIT);
		RUN_STEP("new-hard-limit-matches", spec.getHardLimit() == HARD_LIMIT);
	}

	void testChildLimits() {
		START_TEST("Test operating on child process resource limits");
		if (auto child = cosmos::proc::fork(); child) {
			constexpr LimitInt SOFT_LIMIT = 500;
			constexpr LimitInt HARD_LIMIT = 1000;
			cosmos::LimitSpec spec;
			cosmos::LimitSettings settings{*child};
			spec.setSoftLimit(SOFT_LIMIT);
			spec.setHardLimit(HARD_LIMIT);
			spec = settings.setCpuTimeLimit(spec);
			std::cout << "old child CPU time limit: ";
			printLimit(spec);
			std::cout << "\n";
			spec = settings.getCpuTimeLimit();
			std::cout << "new child CPU time limit: ";
			printLimit(spec);
			std::cout << "\n";

			RUN_STEP("new-child-soft-limit-matches", spec.getSoftLimit() == SOFT_LIMIT);
			RUN_STEP("new-child-hard-limit-matches", spec.getHardLimit() == HARD_LIMIT);

			spec = cosmos::LimitSettings{}.getCpuTimeLimit();

			RUN_STEP("own-soft-limit-untouched", spec.getSoftLimit() != SOFT_LIMIT);
			RUN_STEP("own-hard-limit-untouched", spec.getHardLimit() != HARD_LIMIT);

			cosmos::signal::send(*child, cosmos::signal::KILL);
		} else {
			cosmos::signal::pause();
			cosmos::proc::exit(cosmos::ExitStatus::SUCCESS);
		}
	}
};

int main(const int argc, const char **argv) {
	LimitsTest test;
	return test.run(argc, argv);
}
