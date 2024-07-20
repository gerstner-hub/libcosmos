#pragma once

// glibc
#include <endian.h>
#include <byteswap.h>

// Linux
#include <arpa/inet.h>

// cosmos
#include <cosmos/utils.hxx>

/**
 * @file
 *
 * Helper types and functions for dealing with byte order (endianness) of
 * unsigned integers of different sizes.
 **/

namespace cosmos::net {

/// Differentiation between different endianness storage format.
enum class Endian {
	/// Little endian. Lower value bits are stored first.
	LITTLE,
	/// Big endian. Higher value bits are stored first.
	BIG
};

// TODO: in C++20 the <bits> header provides language builtin support for
// checking the endianness during compile time, so the preprocessor defines
// will no longer be necessary.

#if __BYTE_ORDER == __LITTLE_ENDIAN
/// The byte order setting for the current host.
constexpr auto our_endian = Endian::LITTLE;
#elif __BYTE_ORDER == __BIG_ENDIAN
/// The byte order setting for the current host.
constexpr auto our_endian = Endian::BIG;
#else
#	error "failed to determine endianness"
#endif

constexpr auto foreign_endian = (our_endian == Endian::LITTLE ? Endian::BIG : Endian::LITTLE);

template <Endian endian>
struct EndianTraits {
};

template <>
struct EndianTraits<Endian::LITTLE> {
	static constexpr Endian other = Endian::BIG;
};

template <>
struct EndianTraits<Endian::BIG> {
	static constexpr Endian other = Endian::LITTLE;
};

// TODO: using C++20 std::is_constant_evaluated() we can perform byte order
// swapping transparently during compile time already, if possible.
//
// a template recursion algorithm can be used to perform the byte order
// swapping for compile time.
//
// NOTE: older gcc/gblic doesn't work with `constexpr` here, so drop it for
// the time being.

/// Return the reversed byte order for the given 16 bit value.
inline uint16_t swap_byte_order(uint16_t value) {
	return bswap_16(value);
}

/// Return the reversed byte order for the given 32 bit value.
inline uint32_t swap_byte_order(uint32_t value) {
	return bswap_32(value);
}

/// Return the reversed byte order for the given 64 bit value.
inline uint64_t swap_byte_order(uint64_t value) {
	return bswap_64(value);
}

/// Return the network byte order version of `host`.
template <typename T>
inline T to_network_order(T host) {
	if (our_endian == Endian::BIG) {
		return host;
	} else {
		return swap_byte_order(host);
	}
}

/// Return the host byte order version of `network`.
template <typename T>
inline T to_host_order(T network) {
	if (our_endian == Endian::BIG) {
		return network;
	} else {
		return swap_byte_order(network);
	}
}

/*
 * these enum types are used as strong types for unsigned integers that carry
 * raw data potentially in a foreign endianness. This is used by
 * EndianNumber::raw().
 */

enum class RawLittleInt16 : uint16_t {};
enum class RawLittleInt32 : uint32_t {};
enum class RawLittleInt64 : uint64_t {};
enum class RawBigInt16 : uint16_t {};
enum class RawBigInt32 : uint32_t {};
enum class RawBigInt64 : uint64_t {};
using RawNetInt16 = RawBigInt16;
using RawNetInt32 = RawBigInt32;
using RawNetInt64 = RawBigInt64;

template <typename UINT, Endian endian>
struct RawIntTraits {
};

template <>
struct RawIntTraits<uint16_t, Endian::LITTLE> {
	using Int = RawLittleInt16;
};
template <>
struct RawIntTraits<uint32_t, Endian::LITTLE> {
	using Int = RawLittleInt32;
};
template <>
struct RawIntTraits<uint64_t, Endian::LITTLE> {
	using Int = RawLittleInt64;
};
template <>
struct RawIntTraits<uint16_t, Endian::BIG> {
	using Int = RawBigInt16;
};
template <>
struct RawIntTraits<uint32_t, Endian::BIG> {
	using Int = RawBigInt32;
};
template <>
struct RawIntTraits<uint64_t, Endian::BIG> {
	using Int = RawBigInt64;
};

/// An endianness aware unsigned integer.
/**
 * This type stores an unsigned primitive integer in the given byte order
 * endianness. The interface takes host byte order on input and returns host
 * byte order on output. Only the raw() function returns the unmodified value,
 * possibly in a foreign byte order, as a strong enum type obtained from
 * RawIntTraits.
 **/
template <typename T, Endian endianness>
class EndianNumber {
public: // types

	using RawInt = typename RawIntTraits<T, endianness>::Int;

public: // functions

	// TODO: leave member undefined e.g. for use with placement new or performance, or zero-initialize?
	constexpr EndianNumber() = default;

	/// Construct the number from a raw integer in the correct byte order.
	constexpr EndianNumber(const RawInt rint) :
		m_egg{to_integral(rint)} {
	}

	/// Constructs the number from a native integer that will possibly be converted into the correct byte order.
	constexpr EndianNumber(const T egg) :
		m_egg{toTargetEndianness(egg)} {
	}

	/// Constructs the number from an EndianNumber of differing Endian type.
	constexpr EndianNumber(const EndianNumber<T, EndianTraits<endianness>::other> other) :
		m_egg{toTargetEndianness(other.toHost())} {
	}

	void setFromHost(const T egg) {
		m_egg = toTargetEndianness(egg);
	}

	constexpr T toHost() const {
		if (our_endian == endianness) {
			return m_egg;
		} else {
			return swap_byte_order(m_egg);
		}
	};

	operator T() const {
		return toHost();
	}

	RawInt raw() const {
		return RawInt{m_egg};
	}

protected: // functions

	constexpr static T toTargetEndianness(const T egg) {
		if (our_endian == endianness) {
			return egg;
		} else {
			return swap_byte_order(egg);
		}
	}

protected: // data

	T m_egg;
};

using LittleInt16 = EndianNumber<uint16_t, Endian::LITTLE>;
using LittleInt32 = EndianNumber<uint32_t, Endian::LITTLE>;
using LittleInt64 = EndianNumber<uint64_t, Endian::LITTLE>;

using BigInt16 = EndianNumber<uint16_t, Endian::BIG>;
using BigInt32 = EndianNumber<uint32_t, Endian::BIG>;
using BigInt64 = EndianNumber<uint64_t, Endian::BIG>;

using NetInt16 = BigInt16;
using NetInt32 = BigInt32;
using NetInt64 = BigInt64;

using HostInt16 = EndianNumber<uint16_t, our_endian>;
using HostInt32 = EndianNumber<uint32_t, our_endian>;
using HostInt64 = EndianNumber<uint64_t, our_endian>;

} // end ns
