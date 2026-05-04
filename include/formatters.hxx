#pragma once

// C++
#include <format>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/errno.hxx>
#include <cosmos/error/ResolveError.hxx>
#include <cosmos/proc/ChildCloner.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/utils.hxx>

/**
 * @file
 *
 * This header contains custom formatter implementations for types found
 * libcosmos.
 **/

template<>
struct std::formatter<cosmos::Errno> :
		public std::formatter<std::string> {

	auto format(const cosmos::Errno &err, format_context &context) const {
		return std::formatter<std::string>::format(
			std::format("{} ({})",
				cosmos::ApiError::msg(err),
				cosmos::to_integral(err)),
			context
		);
	}
};

template<>
struct std::formatter<cosmos::ResolveError::Code> :
		public std::formatter<std::string> {

	auto format(
		const cosmos::ResolveError::Code &code, format_context &context) const {
		return std::formatter<string>::format(
			std::format("{}", cosmos::format(code)),
			context
		);
	}
};

template<>
struct std::formatter<cosmos::ChildCloner> :
		public std::formatter<std::string> {

	auto format(const cosmos::ChildCloner &cloner,
			format_context &context) const {
		return std::formatter<string>::format(
			std::format("{}", cloner.info()),
			context
		);
	}
};

template<>
struct std::formatter<cosmos::ExitStatus> :
		public std::formatter<std::string> {

	auto format(const cosmos::ExitStatus &status,
			format_context &context) const {

		auto status_label = [](const cosmos::ExitStatus _status) -> const char* {
			switch (_status) {
				case cosmos::ExitStatus::INVALID: return "(INVALID)";
				case cosmos::ExitStatus::SUCCESS: return "(SUCCESS)";
				case cosmos::ExitStatus::FAILURE: return "(FAILURE)";
				default: return " (other)";
			}
		};

		/// this could be annotated with a special character so that it is
		/// clear right away that this is about an exit status
		return std::formatter<string>::format(
			std::format("{} {}",
				cosmos::to_integral(status),
				status_label(status)
			),
			context
		);
	}
};
