#ifndef COSMOS_PRIVATE_SOCKOPTS_HXX
#define COSMOS_PRIVATE_SOCKOPTS_HXX

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/RangeError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/fs/FileDescriptor.hxx"

namespace cosmos {

/**
 * @file
 *
 * Generic helpers to getting and setting socket options.
 **/

using cosmos::to_integral;

namespace {

	/// Get a T value socket option and return the number of bytes written.
	template <typename T>
	socklen_t getsockopt(
			FileDescriptor sock, const OptLevel lvl,
			const OptName name, T &out) {
		static_assert(std::is_pointer<T>::value != true,
				"this doesn't work for pointer types");
		socklen_t len = sizeof(T);

		const auto res = ::getsockopt(
				to_integral(sock.raw()),
				to_integral(lvl), 
				to_integral(name),
				&out,
				&len);

		if (res != 0) {
			if (cosmos::get_errno() == cosmos::Errno::RANGE) {
				/*
				 * some options indicate with this error that
				 * that a larger buffer is needed. Throw a
				 * special error containing the suggested
				 * length.
				 */
				cosmos_throw (RangeError("getsockopt", len));
			} else {
				cosmos_throw (ApiError("getsockopt"));
			}
		}

		return len;
	}

	/// Get a fixed size primitive value socket option and return it.
	template <typename T>
	T getsockopt(
			FileDescriptor sock, const OptLevel lvl,
			const OptName name) {
		T out;
		const auto len = getsockopt(sock, lvl, name, out);
		// there exist options where the returned data is dynamic in
		// size, so we cannot judge generically whether a short option
		// len is okay or not.
		if (len != sizeof(T)) {
			cosmos_throw (RuntimeError("short getsockopt read"));
		}

		return out;
	}

	/// Get a variable length socket option using an out pointer.
	template <typename T>
	socklen_t getsockopt(
			FileDescriptor sock, const OptLevel lvl,
			const OptName name, T *ptr, socklen_t ptr_len) {

		const auto res = ::getsockopt(
				to_integral(sock.raw()),
				to_integral(lvl), 
				to_integral(name),
				ptr,
				&ptr_len);

		if (res != 0) {
			cosmos_throw (ApiError("getsockopt"));
		}

		return ptr_len;
	}

	/// Set a socket option using a fixed size type T value.
	template <typename T>
	void setsockopt(
			FileDescriptor sock, const OptLevel lvl,
			const OptName name, const T val) {
		static_assert(std::is_pointer<T>::value != true,
				"this doesn't work for pointer types");
		const auto res = ::setsockopt(
				to_integral(sock.raw()),
				to_integral(lvl),
				to_integral(name),
				&val,
				socklen_t{sizeof(T)});

		if (res != 0) {
			cosmos_throw (ApiError("setsockopt"));
		}
	}

	/// Set a socket option using a variable length pointer of the given len in bytes.
	template <typename T>
	void setsockopt(
			FileDescriptor sock, const OptLevel lvl,
			const OptName name, const T *ptr, socklen_t len) {
		const auto res = ::setsockopt(
				to_integral(sock.raw()),
				to_integral(lvl),
				to_integral(name),
				ptr,
				len);

		if (res != 0) {
			cosmos_throw (ApiError("setsockopt"));
		}
	}

} // end anon ns
} // end ns

#endif // inc. guard
