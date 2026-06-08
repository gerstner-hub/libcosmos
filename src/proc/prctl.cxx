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

/* expands a PRCTL preprocessor define to its integer form and it's string
 * form for error handling in prctl wrappers */
#define EXPAND_CTL(x) x, #x

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

/*
 * a generic wrapper for prctls which return a boolean value via an `int` out
 * pointer parameter.
 */
bool prctl_get_bool_by_ptr(const int op, const char *label) {
	int attr = 0;

	if (::prctl(op, &attr) < 0) {
		throw ApiError{std::format("prctl({})", label)};
	}

	return attr != 0;
}

/*
 * a generic wrapper for prctls which return a boolean value as return value
 * (< 0 means error, == 0 means false, > 0 means true).
 */
bool prctl_get_bool_by_value(const int op, const char *label) {
	const auto val = ::prctl(op);

	if (val < 0) {
		throw ApiError{std::format("prctl({})", label)};
	}

	return val != 0;
}

void prctl_set_bool_attr(const int op, const char *label, const bool setting) {
	if (::prctl(op, setting ? 1 : 0) < 0) {
		throw ApiError{std::format("prctl({})", label)};
	}
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
	prctl_cap_ambient(EXPAND_CTL(PR_CAP_AMBIENT_CLEAR_ALL));
}

bool get_cap_in_ambient_set(const Capability cap) {
	return prctl_cap_ambient(EXPAND_CTL(PR_CAP_AMBIENT_IS_SET), cap) != 0;
}

void raise_ambient_cap(const Capability cap) {
	prctl_cap_ambient(EXPAND_CTL(PR_CAP_AMBIENT_RAISE), cap);
}

void lower_ambient_cap(const Capability cap) {
	prctl_cap_ambient(EXPAND_CTL(PR_CAP_AMBIENT_LOWER), cap);
}

bool get_child_subreaper() {
	return prctl_get_bool_by_ptr(EXPAND_CTL(PR_GET_CHILD_SUBREAPER));
}

void set_child_subreaper(const bool is_subreaper) {
	prctl_set_bool_attr(EXPAND_CTL(PR_SET_CHILD_SUBREAPER), is_subreaper);
}

bool get_dumpable() {
	return prctl_get_bool_by_value(EXPAND_CTL(PR_GET_DUMPABLE));
}

void set_dumpable(const bool dumpable) {
	prctl_set_bool_attr(EXPAND_CTL(PR_SET_DUMPABLE), dumpable);
}

void set_anon_memory_name(const void *addr, const size_t len,
		const SysString name) {
	if (::prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME,
				addr, len, name.raw()) != 0) {
		throw ApiError{"prctl(PR_SET_VMA, PR_EST_VMA_ANON_NAME)"};
	}
}

std::string get_thread_name() {
	char name[16];

	if (::prctl(PR_GET_NAME, name) < 0) {
		throw ApiError{"prctl(PR_GET_NAME)"};
	}

	return name;
}

void set_thread_name(const SysString name) {
	if (::prctl(PR_SET_NAME, name.raw()) < 0) {
		throw ApiError{"prctl(PR_SET_NAME)"};
	}
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
