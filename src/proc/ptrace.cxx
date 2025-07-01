// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/UsageError.hxx>
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

		throw ApiError{"ptrace()"};
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
