// C
#include <stdlib.h>
#include <limits.h>

// C++
#include <filesystem>

// cosmos
#include <cosmos/error/FileError.hxx>
#include <cosmos/fs/filesystem.hxx>
#include <cosmos/limits.hxx>
#include <cosmos/fs/path.hxx>
#include <cosmos/string.hxx>

namespace cosmos::fs {

std::string normalize_path(const std::string_view path) {
	std::string ret;

	size_t slashes = 0;
	size_t start = 0;
	auto end = path.find('/');

	while (start < path.size()) {
		const auto component = path.substr(start, end - start);

		if (component.size() > 0) {

			// path starts without a leading /, so it's a relative
			// path, add the CWD
			if (!slashes && ret.empty()) {
				ret.append(get_working_dir());
			}

			// remove the last component
			if (component == "..") {
				// don't remove the root directory
				if (ret.size() > 1) {
					ret.erase(ret.rfind('/'));
				}
				slashes++;
			// references to the CWD are simply ignored
			} else if (component == ".") {
				slashes++;
			} else {
				// add exactly this component
				ret.push_back('/');
				ret.append(component);
			}
		} else {
			slashes++;
		}

		start = end != path.npos ? end + 1 : end;
		end = path.find('/', start);
	}

	if (slashes && ret.empty())
		ret.push_back('/');

	return ret;
}

std::string canonicalize_path(const SysString path) {
	// implementing this on foot is non-trivial so rely on realpath() for
	// the time being.
	std::string ret;
	ret.resize(max::PATH);
	if (::realpath(path.raw(), ret.data()) == nullptr) {
		cosmos_throw (FileError(path, "realpath()"));
	}

	ret.resize(std::strlen(ret.data()));
	return ret;
}

} // end ns
