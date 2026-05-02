#pragma once

// C++
#include <format>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/errno.hxx>
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
