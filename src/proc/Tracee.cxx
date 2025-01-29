// cosmos
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/proc/SigInfo.hxx"
#include "cosmos/proc/Tracee.hxx"

// Linux
#include <linux/filter.h>
#ifdef COSMOS_X86
#	include <asm/ldt.h>
#endif

namespace cosmos {

void Tracee::getSeccompFilter(std::vector<struct sock_filter> &instructions, const unsigned long prog_index) const {

	if (instructions.empty()) {
		const auto num_progs = this->request(ptrace::Request::SECCOMP_GET_FILTER, prog_index);
		instructions.resize(*num_progs);
	}

	struct sock_fprog fprog;
	fprog.len = instructions.size();
	fprog.filter = instructions.data();

	const auto num_progs = this->request(ptrace::Request::SECCOMP_GET_FILTER, prog_index, &fprog);

	/*
	 * This ptrace request actually seems to be missing a size check in
	 * the kernel. We do communicate the number of arrays we have in
	 * fprog.len, but the kernel simply copies into the sock_fprog
	 * whatever amount of data is has. This could lead to a SIGSEGV or an
	 * EFAULT return.
	 *
	 * TODO: Needs runtime testing to find out what exactly will happen.
	 *
	 * For now let's just complain if a larger number of programs is
	 * reported than we'd have space for. Although I believe this code
	 * never be reached, due to SIGSEGV or EFAULT return.
	 */

	if (*num_progs < 0 || static_cast<unsigned long>(*num_progs) >= instructions.size()) {
		cosmos_throw (RuntimeError("seccomp filter array size inconsistency"));
	}
}

#ifdef COSMOS_X86
void Tracee::getThreadArea(struct user_desc &desc) const {
	// the entry_number in the struct is actually ignored by the kernel,
	// we need to provide it as `addr` instead.
	this->request(ptrace::Request::GET_THREAD_AREA, desc.entry_number, &desc);
}

void Tracee::setThreadArea(const struct user_desc &desc) {
	this->request(ptrace::Request::SET_THREAD_AREA, desc.entry_number, &desc);
}
#endif // X86

void Tracee::getSyscallInfo(ptrace::SyscallInfo &info) const {
	const auto obtained = this->request(ptrace::Request::GET_SYSCALL_INFO, sizeof(*info.raw()), info.raw());

	if (*obtained < 0 || static_cast<unsigned long>(*obtained) > sizeof(*info.raw())) {
		cosmos_throw (RuntimeError("excess SYSCALL_INFO data, truncation occurred!"));
	}
}

void Tracee::getSigInfo(SigInfo &info) const {
	this->request(ptrace::Request::GETSIGINFO, nullptr, info.raw());
}

void Tracee::setSigInfo(const SigInfo &info) {
	this->request(ptrace::Request::SETSIGINFO, nullptr, info.raw());
}

std::vector<SigInfo> Tracee::peekSigInfo(const ptrace::PeekSigInfo &settings) {
	const auto raw_settings = settings.raw();
	std::vector<SigInfo> ret;
	ret.resize(raw_settings->nr);

	const auto num_entries = this->request(
			ptrace::Request::PEEKSIGINFO, raw_settings, ret.data());

	ret.resize(*num_entries);

	return ret;
}

} // end ns
