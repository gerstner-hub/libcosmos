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
		checkBoundingCaps();
		checkAmbientCaps();
		checkSubReaper();
		checkDumpable();
		checkThreadName();
		checkNoNewPrivs();
		checkParentDeathSig();
		checkPTracer();
		checkSecureBits();
		checkClearTidAddr();
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

	void checkBoundingCaps() {
		const auto dac_over_in_set = cosmos::prctl::get_cap_in_bounding_set(cosmos::Capability::DAC_OVERRIDE);

		RUN_STEP("get-cap-in-bounding-set-works", true);
		std::cout << "DAC_OVERRIDE in bounding set? " << dac_over_in_set << "\n";

		try {
			cosmos::prctl::drop_cap_from_bounding_set(cosmos::Capability::DAC_OVERRIDE);
			RUN_STEP("drop-cap-from-bounding-set-works", true);
		} catch (const cosmos::ApiError &ex) {
			RUN_STEP("drop-cap-from-bounding-set-eperm", ex.errnum() == cosmos::Errno::PERMISSION);
		}
	}

	void checkAmbientCaps() {
		START_TEST("ambient capabilities");

		const auto in_amb = cosmos::prctl::get_cap_in_ambient_set(cosmos::Capability::KILL);

		RUN_STEP("get-cap-in-ambient-set-works", true);
		std::cout << "CAP_KILL in ambient set? " << in_amb << "\n";

		try {
			cosmos::prctl::raise_ambient_cap(cosmos::Capability::KILL);
			RUN_STEP("raise-ambient-cap-works", true);
		} catch (const cosmos::ApiError &ex) {
			RUN_STEP("raise-ambient-cap-eperm", ex.errnum() == cosmos::Errno::PERMISSION);
		}

		cosmos::prctl::lower_ambient_cap(cosmos::Capability::KILL);

		RUN_STEP("drop-ambient-cap-works", true);

		cosmos::prctl::drop_all_ambient_caps();

		RUN_STEP("drop-all-ambient-cap-works", true);
	}

	void checkSubReaper() {
		START_TEST("child subreaper");

		auto subreaper = cosmos::prctl::get_child_subreaper();

		RUN_STEP("not-a-subreaper-by-default", !subreaper);

		cosmos::prctl::set_child_subreaper(true);

		subreaper = cosmos::prctl::get_child_subreaper();

		RUN_STEP("change-subreaper-works", subreaper == true);
	}

	void checkDumpable() {
		START_TEST("dumpable attr");

		const auto old_dumpable = cosmos::prctl::get_dumpable();

		cosmos::prctl::set_dumpable(!old_dumpable);

		const auto new_dumpable = cosmos::prctl::get_dumpable();

		RUN_STEP("change-dumpable-works", old_dumpable != new_dumpable);
	}

	void checkThreadName() {
		START_TEST("thread name attr");

		cosmos::prctl::set_thread_name("test-name");

		RUN_STEP("new-thread-name-matches",
				cosmos::prctl::get_thread_name() == "test-name");
	}

	void checkNoNewPrivs() {
		START_TEST("no new privs");

		auto no_new_privs = cosmos::prctl::get_no_new_privs();

		RUN_STEP("default-off", !no_new_privs);
		
		cosmos::prctl::set_no_new_privs();

		no_new_privs = cosmos::prctl::get_no_new_privs();

		RUN_STEP("enabling-works", no_new_privs);
	}

	void checkParentDeathSig() {
		START_TEST("parent death sig");

		auto death_sig = cosmos::prctl::get_parent_death_signal();

		RUN_STEP("default-unset", death_sig == cosmos::SignalNr::NONE);

		cosmos::prctl::set_parent_death_signal(cosmos::SignalNr::USR1);

		death_sig = cosmos::prctl::get_parent_death_signal();

		RUN_STEP("set-works", death_sig == cosmos::SignalNr::USR1);
	}

	void checkPTracer() {
		START_TEST("ptracer pid");

		cosmos::prctl::set_any_ptracer();

		RUN_STEP("set-any-ptracer-nothrow", true);
	}

	void checkSecureBits() {
		START_TEST("secure bits");

		const auto bits = cosmos::prctl::get_secure_bits();

		RUN_STEP("securebits-empty", bits.none());

		/*
		 * we cannot test setting these bits, since this requires
		 * special privileges.
		 */
	}

	void checkClearTidAddr() {
		START_TEST("clear tid addr");

		const auto addr = cosmos::prctl::get_clear_child_tid_addr();
		(void)addr;

		RUN_STEP("get-clear-child-tid-addr-works", true);
	}
};

int main(const int argc, const char **argv) {
	TestPrctl test;
	return test.run(argc, argv);
}
