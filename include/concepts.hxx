#pragma once

// C++
#include <concepts>

// cosmos
#include <cosmos/fs/types.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/thread/thread.hxx>
#include <cosmos/types.hxx>

/**
 * @file
 *
 * This header contains C++20 concept definitions for types found in
 * libcosmos.
 **/

namespace cosmos {
	template <typename T>
	concept IntegralOutput = std::same_as<T, ProcessID> ||
		std::same_as<T, ThreadID> || 
		std::same_as<T, UserID> ||
		std::same_as<T, GroupID> ||
		std::same_as<T, SignalNr> ||
		std::same_as<T, FileNum>;

} // end ns cosmos

