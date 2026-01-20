// cosmos
#include <cosmos/compiler.hxx>
#include <cosmos/proc/prctl.hxx>

// Test
#include "TestBase.hxx"

class TestPrctl :
		public cosmos::TestBase {

	void runTests() override {
		checkCpuID();
		checkFsGsRegs();
	}

	void checkCpuID() {
#ifdef COSMOS_X86
		START_TEST("GET/SET CPUID");
		RUN_STEP("cpuid-default-enabled", cosmos::prctl::x86::get_cpuid_enabled() == true);
		try {
			cosmos::prctl::x86::set_cpuid_enabled(false);
			RUN_STEP("cpuid-disable-works", cosmos::prctl::x86::get_cpuid_enabled() == false);
		} catch (const cosmos::ApiError &ex) {
			if (!onValgrind()) {
				// on Valgrind we observe EINVAL here
				RUN_STEP("setcpuid-not-supported", ex.errnum() == cosmos::Errno::NO_DEVICE);
			}
		}
#endif
	}

	void checkFsGsRegs() {
#ifdef COSMOS_X86_64
		START_TEST("GET/SET FS/GS register base");
		/*
		 * we cannot really modify these registers without breaking
		 * our own process, so simply try getting and setting the
		 * existing address.
		 */
		const auto orig_fs = cosmos::prctl::x86_64::get_fs_register_base();
		cosmos::prctl::x86_64::set_fs_register_base(orig_fs);
		RUN_STEP("set-fs-register-works", cosmos::prctl::x86_64::get_fs_register_base() == orig_fs);
		cosmos::prctl::x86_64::set_fs_register_base(orig_fs);

		const auto orig_gs = cosmos::prctl::x86_64::get_gs_register_base();
		cosmos::prctl::x86_64::set_gs_register_base(orig_gs);
		RUN_STEP("set-gs-register-works", cosmos::prctl::x86_64::get_gs_register_base() == orig_gs);
		cosmos::prctl::x86_64::set_gs_register_base(orig_gs);
#endif
	}
};

int main(const int argc, const char **argv) {
	TestPrctl test;
	return test.run(argc, argv);
}
