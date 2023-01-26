// Cosmos
#include "cosmos/proc/Process.hxx"
#include "cosmos/proc/SigSet.hxx"
#include "cosmos/errors/ApiError.hxx"

// Linux
#include <sys/types.h>
#include <unistd.h>

namespace cosmos {

ProcessID Process::cachePid() const {
	m_own_pid = ::getpid();
	return m_own_pid;
}

ProcessID Process::cachePPid() const {
	m_parent_pid = ::getppid();
	return m_parent_pid;
}

UserID Process::getRealUserID() const {
	return ::getuid();
}

UserID Process::getEffectiveUserID() const {
	return ::geteuid();
}

namespace {
	void setSignalMask(int op, const sigset_t *set, sigset_t *old) {
		auto res = ::pthread_sigmask(op, set, old);

		if (res == 0)
			return;

		cosmos_throw (ApiError());
	}
}

void Process::blockSignals(const SigSet &s, std::optional<SigSet*> old) {
	setSignalMask(SIG_BLOCK, s.raw(), old ? old.value()->raw() : nullptr);
}

void Process::unblockSignals(const SigSet &s, std::optional<SigSet*> old) {
	setSignalMask(SIG_UNBLOCK, s.raw(), old ? old.value()->raw() : nullptr);
}

void Process::setSigMask(const SigSet &s, std::optional<SigSet*> old) {
	setSignalMask(SIG_SETMASK, s.raw(), old ? old.value()->raw() : nullptr);
}

ProcessID Process::createNewSession() {
	auto res = ::setsid();

	if (res == INVALID_PID) {
		cosmos_throw (ApiError());
	}

	return res;
}

SigSet Process::getSigMask() {
	SigSet ret;
	setSignalMask(SIG_SETMASK, nullptr, ret.raw());
	return ret;
}

Process g_process;

} // end ns
