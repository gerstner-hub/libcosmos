#pragma once

// C++
#include <format>
#include <map>
#include <vector>

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

template <>
struct std::formatter<cosmos::SignalNr> :
		public std::formatter<std::string> {
	auto format(const cosmos::SignalNr nr, format_context &context) const {
		return std::formatter<string>::format(
			std::format("{}", cosmos::to_integral(nr)),
			context
		);
	}
};

template <>
struct std::formatter<cosmos::Signal> :
		public std::formatter<std::string> {
	auto format(const cosmos::Signal sig, format_context &context) const {
		return std::formatter<string>::format(
			std::format("{} ({})",
				sig.name(), sig.raw()
			),
			context
		);
	}
};

template <>
struct std::formatter<cosmos::ChildState> :
		public std::formatter<std::string> {
	auto format(const cosmos::ChildState &info, format_context &context) const {
		auto out = context.out();
		using Event = cosmos::ChildState::Event;

		switch (info.event) {
			case Event::EXITED: {
				out = std::format_to(out, "Child exited with {}", *info.status);
				break;
			} case Event::KILLED: {
				out = std::format_to(out, "Child killed by {}", *info.signal);
				break;
			} case Event::DUMPED: {
				out = std::format_to(out, "Child killed by {} (dumped core)", *info.signal);
				break;
			} case Event::TRAPPED: {
			     	out = std::format_to(out, "Child trapped");
				break;
			} case Event::STOPPED: {
			      	out = std::format_to(out, "Child stopped by {}", *info.signal);
				break;
			} case Event::CONTINUED: {
				out = std::format_to(out, "Child continued by {}", *info.signal);
				break;
			} default: {
				out = std::format_to(out, "Bad ChildState");
				break;
			}
		}

		return out;
	}
};

template<typename T>
struct std::formatter<std::vector<T>> :
		public std::formatter<std::string> {
	auto format(const std::vector<T> &vec, format_context &context) const {
		auto out = context.out();

		size_t first = 0;

		for (const auto &val: vec) {
			if (first++) {
				out = std::format_to(out, ", ");
			}

			out = std::format_to(out, "{}", val);
		}

		return out;
	}
};

template<typename K, typename V>
struct std::formatter<std::map<K, V>> :
		public std::formatter<std::string> {
	auto format(const std::map<K, V> &map, format_context &context) const {
		auto out = context.out();

		for (const auto &pair: map) {
			out = std::format_to(out, "{}: {}\n", pair.first, pair.second);
		}

		return out;
	}
};
