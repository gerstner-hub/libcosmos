// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/proc/limits.hxx>
#include <cosmos/utils.hxx>

namespace cosmos::proc {

namespace {

/* the glibc wrapper for prlimit uses an actually enum type for the
 * resource specified, thus we need to cast to the expected type to
 * avoid compilation errors or warnings */
using ResourceT = decltype(RLIMIT_AS);

}

LimitSpec get_limit(const LimitType type, const ProcessID pid) {
	LimitSpec ret;

	if (::prlimit(cosmos::to_integral(pid), static_cast<ResourceT>(type), nullptr, &ret) == 0)
		return ret;

	throw ApiError{"prlimit()"};
}

LimitSpec set_limit(const LimitType type, const LimitSpec &spec,
		const ProcessID pid) {

	LimitSpec ret;

	if (::prlimit(cosmos::to_integral(pid), static_cast<ResourceT>(type), &spec, &ret) == 0) {
		return ret;
	}

	throw ApiError{"prlimit()"};
}

} // end ns
