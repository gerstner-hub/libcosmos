// cosmos
#include <cosmos/fs/types.hxx>
#include <cosmos/formatting.hxx>

std::ostream& operator<<(std::ostream &o, const cosmos::FileMode mode) {
	o << mode.symbolic() << " (" << cosmos::OctNum{cosmos::to_integral(mode.raw()), 4} << ")";
	return o;
}

std::ostream& operator<<(std::ostream &o, const cosmos::FileType type) {
	o << type.symbolic();
	return o;
}

std::ostream& operator<<(std::ostream &o, const cosmos::OpenFlags flags) {
	using Flag = cosmos::OpenFlag;
	bool first = true;

	for (const auto &pair: {
			std::make_pair(Flag::APPEND,    "APPEND"),
			              {Flag::ASYNC,     "ASYNC"},
				      {Flag::CLOEXEC,   "CLOEXEC"},
				      {Flag::CREATE,    "CREATE"},
				      {Flag::DIRECT,    "DIRECT"},
				      {Flag::DIRECTORY, "DIRECTORY"},
				      {Flag::DSYNC,     "DSYNC"},
				      {Flag::EXCLUSIVE, "EXCLUSIVE"},
				      {Flag::NOATIME,   "NOATIME"},
				      {Flag::NO_CONTROLLING_TTY, "NO_CONTROLLING_TTY"},
				      {Flag::NOFOLLOW,  "NOFOLLOW"},
				      {Flag::NONBLOCK,  "NONBLOCK"},
				      {Flag::PATH,      "PATH"},
				      {Flag::SYNC,      "SYNC"},
				      {Flag::TMPFILE,   "TMPFILE"},
				      {Flag::TRUNCATE,  "TRUNCATE"}
	}) {
		auto [flag, label] = pair;

		if (flags[flag]) {
			if (first)
				first = false;
			else
				o << ", ";

			o << label;
		}
	}

	return o;
}
