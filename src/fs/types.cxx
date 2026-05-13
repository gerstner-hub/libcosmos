// cosmos
#include <cosmos/formatters.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/fs/types.hxx>

namespace cosmos {

const char* get_label(const OpenFlag flag) {
	using enum OpenFlag;
	switch (flag) {
		case APPEND:    return "APPEND";
		case ASYNC:     return "ASYNC";
		case CLOEXEC:   return "CLOEXEC";
		case CREATE:    return "CREATE";
		case DIRECT:    return "DIRECT";
		case DIRECTORY: return "DIRECTORY";
		case DSYNC:     return "DSYNC";
		case EXCLUSIVE: return "EXCLUSIVE";
		case NOATIME:   return "NOATIME";
		case NO_CONTROLLING_TTY: return "NO_CONTROLLING_TTY";
		case NOFOLLOW:  return "NOFOLLOW";
		case NONBLOCK:  return "NONBLOCK";
		case PATH:      return "PATH";
		case SYNC:      return "SYNC";
		case TMPFILE:   return "TMPFILE";
		case TRUNCATE:  return "TRUNCATE";
		default:        return "???";
	}
}

}

std::ostream& operator<<(std::ostream &o, const cosmos::FileMode mode) {
	o << std::format("{}", mode);
	return o;
}

std::ostream& operator<<(std::ostream &o, const cosmos::FileType type) {
	o << type.symbolic();
	return o;
}

std::ostream& operator<<(std::ostream &o, const cosmos::OpenFlags flags) {
	o << std::format("{}", flags);
	return o;
}
