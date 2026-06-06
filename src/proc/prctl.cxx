// C++
#include <format>
#include <optional>

// cosmos
#include <cosmos/compiler.hxx>
#include <cosmos/error/ApiError.hxx>
#include <cosmos/proc/prctl.hxx>
#include <cosmos/utils.hxx>

// Linux
#ifdef COSMOS_X86
#	include <asm/prctl.h>
#	include <sys/syscall.h>
#	include <unistd.h>
#endif
#include <linux/prctl.h>
#include <sys/prctl.h>

#ifndef COSMOS_X86
/*
 * this is for non x86 platforms to simplify the code below, which should
 * throw an ENOSYS ApiError on non-x86 systems.
 */
#	ifndef ARCH_GET_FS
#		define ARCH_GET_FS 0
#	endif
#	ifndef ARCH_SET_FS
#		define ARCH_SET_FS 0
#	endif
#	ifndef ARCH_GET_GS
#		define ARCH_GET_GS 0
#	endif
#	ifndef ARCH_SET_GS
#		define ARCH_SET_GS 0
#	endif
#	ifndef ARCH_GET_CPUID
#		define ARCH_GET_CPUID 0
#	endif
#	ifndef ARCH_SET_CPUID
#		define ARCH_SET_CPUID 0
#	endif
#endif

namespace cosmos::prctl {

namespace {

template <typename ADDR>
int arch_prctl(int op, ADDR addr, const bool eval_error = true) {
#ifdef COSMOS_X86
	const auto res = syscall(SYS_arch_prctl, op, addr);

	if (eval_error && res != 0) {
		throw ApiError{"arch_prctl()"};
	}

	return res;
#else
	(void)op;
	(void)addr;
	(void)eval_error;
	throw ApiError{"arch_prctl()", Errno::NO_SYS};
#endif
}

int prctl_cap_ambient(const int subop, const char *label,
		const std::optional<Capability> cap = {}) {
	const auto ret = ::prctl(PR_CAP_AMBIENT, subop,
			cap ? to_integral(*cap) : 0, 0, 0);

	if (ret < 0) {
		throw ApiError{std::format("prctl(PR_CAP_AMBIENT, {}", label)};
	}

	return ret;
}

} // end anon ns

bool get_cap_in_bounding_set(const Capability cap) {
	const auto ret = ::prctl(PR_CAPBSET_READ, to_integral(cap));

	if (ret < 0) {
		throw ApiError{"prctl(PR_CAPBSET_READ)"};
	}

	return ret != 0;
}

void drop_cap_from_bounding_set(const Capability cap) {
	if (::prctl(PR_CAPBSET_DROP, to_integral(cap)) != 0) {
		throw ApiError{"prctl(PR_CAPBSET_DROP)"};
	}
}

void drop_all_ambient_caps() {
	prctl_cap_ambient(PR_CAP_AMBIENT_CLEAR_ALL, "PR_CAP_AMBIENT_CLEAR_ALL");
}

bool get_cap_in_ambient_set(const Capability cap) {
	return prctl_cap_ambient(PR_CAP_AMBIENT_IS_SET,
			"PR_CAP_AMBIENT_IS_SET", cap) != 0;
}

void raise_ambient_cap(const Capability cap) {
	prctl_cap_ambient(PR_CAP_AMBIENT_RAISE, "PR_CAP_AMBIENT_RAISE", cap);
}

void lower_ambient_cap(const Capability cap) {
	prctl_cap_ambient(PR_CAP_AMBIENT_LOWER, "PR_CAP_AMBIENT_LOWER", cap);
}

namespace x86 {

bool get_cpuid_enabled() {
	return arch_prctl(ARCH_GET_CPUID, nullptr, false) != 0;
}

void set_cpuid_enabled(const bool on_off) {
	(void)arch_prctl(ARCH_SET_CPUID, on_off ? 0x1 : 0x0);
}

} // end ns x86

namespace x86_64 {

unsigned long get_fs_register_base() {
	unsigned long ret;
	(void)arch_prctl(ARCH_GET_FS, &ret);
	return ret;
}

void set_fs_register_base(const unsigned long addr) {
	(void)arch_prctl(ARCH_SET_FS, addr);
}

unsigned long get_gs_register_base() {
	unsigned long ret;
	(void)arch_prctl(ARCH_GET_GS, &ret);
	return ret;
}

void set_gs_register_base(const unsigned long addr) {
	(void)arch_prctl(ARCH_SET_GS, addr);
}

} // end ns x86_64

} // end ns
