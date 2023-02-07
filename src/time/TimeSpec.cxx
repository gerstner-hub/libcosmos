// C++
#include <type_traits>

// cosmos
#include "cosmos/time/TimeSpec.hxx"

namespace cosmos {

// this is an important property since we want to take advantage of wrapping
// existing struct timespec instances e.g. in FileStatus::getModeTime()
static_assert(sizeof(TimeSpec) == sizeof(struct timespec));

// this is sadly not true, because the type has custom constructors
#if 0
static_assert(std::is_trivial<TimeSpec>::value == true);
#endif

} // end ns
