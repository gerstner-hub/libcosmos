// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/private/cosmos.hxx>
#include <cosmos/random.hxx>

namespace cosmos {

size_t get_random(void *buffer, const size_t amount, const GetRandomFlags flags) {
	ssize_t res;

	while (true) {
		res = ::getrandom(buffer, amount, flags.raw());
		if (res >= 0) {
			return res;
		} else if (auto_restart_syscalls && errno == EINTR) {
			continue;
		}

		throw ApiError{"getrandom()"};
	}
}

} // end ns
