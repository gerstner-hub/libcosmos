// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/proc/ptrace.hxx>
#include <cosmos/utils.hxx>

namespace cosmos::ptrace {

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

		cosmos_throw (ApiError("ptrace()"));
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

} // end ns
