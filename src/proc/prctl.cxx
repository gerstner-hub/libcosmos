// cosmos
#include <cosmos/compiler.hxx>
#include <cosmos/error/ApiError.hxx>
#include <cosmos/proc/prctl.hxx>

// Linux
#ifdef COSMOS_X86
#	include <asm/prctl.h>
#	include <sys/syscall.h>
#	include <unistd.h>
#endif

#ifndef COSMOS_X86
/*
 * for non x86 platforms to simplify the code below, which should thrown an
 * ENOSYS ApiError on on x86 systems.
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

template <typename ADDR>
static int arch_prctl(int op, ADDR addr, const bool eval_error = true) {
#ifdef COSMOS_X86
	const auto res = syscall(SYS_arch_prctl, op, addr);

	if (eval_error && res != 0) {
		throw ApiError{"arch_prctl()"};
	}

	return res;
#else
	throw ApiError{"arch_prctl()", Errno::NO_SYS};
#endif
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
