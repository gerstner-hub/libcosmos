// C++
#include <fstream>
#include <string>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/private/cosmos.hxx>
#include <cosmos/proc/pidfd.h>
#include <cosmos/proc/signal.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/string.hxx>

// Linux
#include <signal.h>

namespace cosmos::signal {

namespace {

	void set_signal_mask(int op, const sigset_t *set, sigset_t *old) {
		auto res = ::pthread_sigmask(op, set, old);

		if (res == 0)
			return;

		cosmos_throw (ApiError("pthread_sigmask()"));
	}

} // end anon ns

void raise(const Signal s) {
	if (::raise(to_integral(s.raw()))) {
		cosmos_throw (ApiError("raise()"));
	}
}

void send(const ProcessID proc, const Signal s) {
	if (::kill(to_integral(proc), to_integral(s.raw()))) {
		cosmos_throw (ApiError("kill()"));
	}
}

void send(const ProcessID proc, const ThreadID thread, const Signal s) {
	if (::tgkill(to_integral(proc), to_integral(thread), to_integral(s.raw()))) {
		cosmos_throw (ApiError("tgkill()"));
	}
}

void send(const PidFD pidfd, const Signal s) {
	if (!running_on_valgrind) {
		// there's no glibc wrapper for this yet
		//
		// the third siginfo_t argument allows more precise control of the
		// signal auxiliary data, but the defaults are just like kill(), so
		// let's use them for now.
		if (::pidfd_send_signal(to_integral(pidfd.raw()), to_integral(s.raw()), nullptr, 0) != 0) {
			cosmos_throw (ApiError("pidfd_send_signal()"));
		}
	} else {
		// when running on Valgrind then this system isn't covered yet
		// by Valgrind's virtual machine. Implement a fallback to
		// emulate the system call behaviour.
		//
		// we can find out the PID the FD is for by inspect proc
		std::string path{"/proc/self/fdinfo/"};
		path += std::to_string(to_integral(pidfd.raw()));
		std::ifstream fdinfo;
		fdinfo.open(path);

		if (!fdinfo) {
			cosmos_throw (ApiError("open(\"/proc/self/fdinfo/<pidfd>\")"));
		}

		std::string line;
		constexpr std::string_view PID_FIELD{"Pid:"};

		while (std::getline(fdinfo, line).good()) {
			if (!is_prefix(line, PID_FIELD))
				continue;

			auto pid_str = stripped(line.substr(PID_FIELD.size()));
			size_t processed = 0;
			auto pid = std::stoi(pid_str, &processed);

			if (processed < 1) {
				cosmos_throw (RuntimeError("failed to determine PID for PIDFD (valgrind fallback logic)"));
			}

			signal::send(ProcessID{pid}, s);
			return;
		}

		cosmos_throw (RuntimeError("couldn't parse PID for PIDFD (valgrind fallback logic)"));
	}
}

void block(const SigSet &s, std::optional<SigSet*> old) {
	set_signal_mask(SIG_BLOCK, s.raw(), old ? old.value()->raw() : nullptr);
}

void unblock(const SigSet &s, std::optional<SigSet*> old) {
	set_signal_mask(SIG_UNBLOCK, s.raw(), old ? old.value()->raw() : nullptr);
}

void set_sigmask(const SigSet &s, std::optional<SigSet*> old) {
	set_signal_mask(SIG_SETMASK, s.raw(), old ? old.value()->raw() : nullptr);
}

SigSet get_sigmask() {
	SigSet ret;
	set_signal_mask(SIG_SETMASK, nullptr, ret.raw());
	return ret;
}

} // end ns
