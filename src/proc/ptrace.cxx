// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/UsageError.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/proc/ptrace.hxx>
#include <cosmos/utils.hxx>

namespace cosmos::ptrace {

static_assert(sizeof(SyscallInfo::EntryInfo) == sizeof(SyscallInfo::RawEntryInfo),
		"SyscallInfo::EntryInfo size mismatch");
static_assert(sizeof(SyscallInfo::ExitInfo) == sizeof(SyscallInfo::RawExitInfo),
		"SyscallInfo::ExitInfo size mismatch");
static_assert(sizeof(SyscallInfo::SeccompInfo) == sizeof(SyscallInfo::SeccompInfo),
		"SyscallInfo::SeccompInfo size mismatch");
static_assert(sizeof(uint64_t) == sizeof(__u64),
		"mismatch between uint64_t and __64");

namespace {

	const char* to_label(const Request req) {
		switch(req) {
			case Request::TRACEME: return "TRACEME";
			case Request::PEEKDATA: return "PEEKDATA";
			case Request::PEEKTEXT: return "PEEKTEXT";
			case Request::PEEKUSER: return "PEEKUSER";
			case Request::POKEDATA: return "POKEDATA";
			case Request::POKEUSER: return "POKEUSER";
#ifdef PTRACE_GETREGS
			case Request::GETREGS: return "GETREGS";
#endif
#ifdef PTRACE_GETFPREGS
			case Request::GETFPREGS: return "GETFPREGS";
#endif
			case Request::GETREGSET: return "GETREGSET";
#ifdef PTRACE_SETREGS
			case Request::SETREGS: return "SETREGS";
#endif
#ifdef PTRACE_SETFPREGS
			case Request::SETFPREGS: return "SETFPREGS";
#endif
			case Request::SETREGSET: return "SETREGSET";
			case Request::GETSIGINFO: return "GETSIGINFO";
			case Request::SETSIGINFO: return "SETSIGINFO";
			case Request::PEEKSIGINFO: return "PEEKSIGINFO";
			case Request::GETSIGMASK: return "GETSIGMASK";
			case Request::SETSIGMASK: return "SETSIGMASK";
			case Request::SETOPTIONS: return "SETOPTIONS";
			case Request::GETEVENTMSG: return "GETEVENTMSG";
			case Request::CONT: return "CONT";
			case Request::SYSCALL: return "SYSCALL";
			case Request::SINGLESTEP: return "SINGLESTEP";
#ifdef PTRACE_SET_SYSCALL
			case Request::SET_SYSCALL: return "SET_SYSCALL";
#endif
#ifdef PTRACE_SYSEMU
			case Request::SYSEMU: return "SYSEMU";
			case Request::SYSEMU_SINGLESTEP: return "SYSEMU_SINGLESTEP";
#endif
			case Request::LISTEN: return "LISTEN";
			case Request::KILL: return "KILL";
			case Request::INTERRUPT: return "INTERRUPT";
			case Request::ATTACH: return "ATTACH";
			case Request::SEIZE: return "SEIZE";
			case Request::SECCOMP_GET_FILTER: return "SECCOMP_GET_FILTER";
			case Request::DETACH: return "DETACH";
#ifdef PTRACE_GET_THREAD_AREA
			case Request::GET_THREAD_AREA: return "GET_THREAD_AREA";
#endif
#ifdef PTRACE_SET_THREAD_AREA
			case Request::SET_THREAD_AREA: return "SET_THREAD_AREA";
#endif
			case Request::GET_SYSCALL_INFO: return "GET_SYSCALL_INFO";
			default: return "?unknown?";
		}
	}
}

std::optional<long> trace(const Request req, const ProcessID pid, void *addr, void *data) {
	const auto is_peek =
		req == Request::PEEKDATA ||
		req == Request::PEEKTEXT ||
		req == Request::PEEKUSER;

	if (is_peek) {
		// PEEK requests have difficult error handling, we need to
		// explicitly reset the errno.
		reset_errno();
	}

	const auto ret = ::ptrace(
			static_cast<__ptrace_request>(to_integral(req)),
			to_integral(pid), addr, data);

	if (ret == -1) {
		if (is_peek && !is_errno_set()) {
			// we actually peeked a -1
			return ret;
		}

		throw ApiError{sprintf("ptrace(%d, %s, ...)", to_integral(pid), to_label(req))};
	}

	// only these ptrace requests return meaningful data, otherwise just
	// zero on success.
	if (is_peek || req == Request::SECCOMP_GET_FILTER || req == Request::GET_SYSCALL_INFO) {
		return ret;
	}

	return std::nullopt;
}

void traceme() {
	// all arguments except the request are ignored.
	// this is not clearly documented but I checked the kernel code to verify.
	trace(Request::TRACEME, ProcessID{0}, nullptr, nullptr);
}

std::tuple<SignalNr, Event> decode_event(const cosmos::Signal sig) {

	if (!sig.isPtraceEventStop()) {
		throw UsageError{"this is not a ptrace-event-stop signal"};
	}

	const auto raw = to_integral(sig.raw());
	const auto event = cosmos::ptrace::Event{raw >> 8};
	const cosmos::SignalNr signr{raw & 0xff};

	return std::make_tuple(signr, event);
}

} // end ns
