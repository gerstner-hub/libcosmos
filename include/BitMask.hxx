#pragma once

// C++
#include <initializer_list>
#include <string>
#include <type_traits>

namespace cosmos {

/// A typesafe bit mask representation using class enums.
/**
 * Instead of using a plain integer type and preprocessor constants to denote
 * certain bit positions, this type provides a type safe implementation of a
 * bitset with named bits based on a strongly typed enum class.
 *
 * The interface is kept similar to that of std::bitset.
 **/
template <typename ENUM>
class BitMask {
public: // types

	/// Helper type for setting all bits during construction time of BitMask.
	struct All {};
	static constexpr All all{};

	using EnumBaseType = typename std::underlying_type<ENUM>::type;

public: // functions

	/// Sets all bits to zero.
	constexpr BitMask() {}

	/// Sets all bits to one.
	explicit BitMask(const All a) {
		set(a);
	}

	/// Sets only the flags found in the given initializer list.
	BitMask(const std::initializer_list<ENUM> &flags) {
		for (auto val: flags) {
			m_flags |= static_cast<EnumBaseType>(val);
		}
	}

	/// Sets exactly the given bit position.
	constexpr BitMask(const ENUM val) :
		m_flags{static_cast<EnumBaseType>(val)}
	{}

	/// Sets exactly the given primitive type bitmask.
	explicit constexpr BitMask(const EnumBaseType value) :
		m_flags{value}
	{}

	/// Returns the raw bitfield integer.
	EnumBaseType raw() const { return m_flags; }

	/// Return a string representation of the bit mask.
	explicit operator std::string() const { return toString(); }

	/// Returns a boolean value for the given value, \see `test()`.
	bool operator[] (const ENUM flag) const { return test(flag); }

	std::string toString() const {
		std::string ret;

		// append each bit starting with the highest one
		for (int bit = size() - 1; bit >= 0; bit--) {
			const auto val = 1 << bit;
			ret.push_back(this->test(static_cast<ENUM>(val)) ? '1' : '0');
		}

		return ret;
	}

	/// Sets all bits it the mask.
	BitMask& set(const All) {
		m_flags = ~EnumBaseType(0);
		return *this;
	}

	/// Set or reset the given bit position.
	BitMask& set(const ENUM val, bool on_off = true) {
		const auto bitval = static_cast<EnumBaseType>(val);
		m_flags = (on_off ? (m_flags|bitval) : (m_flags & ~bitval));
		return *this;
	}

	/// Set all of the given bit positions.
	BitMask& set(const std::initializer_list<ENUM> &flags) {
		for (auto flag: flags) {
			set(flag);
		}
		return *this;
	}

	/// Set all the bits that are also set in `other`.
	BitMask& set(const BitMask other) {
		m_flags |= other.m_flags;
		return *this;
	}

	/// Sets all bits to zero.
	BitMask& reset() {
		m_flags = EnumBaseType{0};
		return *this;
	}

	/// Zeroes the given bit position.
	BitMask& reset(const ENUM val) {
		reset({val});
		return *this;
	}

	/// Zeroes all of the given flags.
	BitMask& reset(const std::initializer_list<ENUM> &flags) {
		for (auto val: flags) {
			m_flags &= ~static_cast<EnumBaseType>(val);
		}
		return *this;
	}

	/// Zeroes all bit positions that are set in `other`.
	BitMask& reset(const BitMask other) {
		m_flags &= ~(other.raw());
		return *this;
	}

	/// Returns a copy of the bit mask with all positions zeroed that are set in `other`.
	BitMask reset(const BitMask other) const {
		auto ret = *this;
		return ret.reset(other);
	}

	/// Sets all bits to zero except those already set and also found in `flags`.
	BitMask& limit(const std::initializer_list<ENUM> &flags) {
		EnumBaseType mask = 0;
		for (auto val: flags) {
			mask |= static_cast<EnumBaseType>(val);
		}

		m_flags &= mask;
		return *this;
	}

	/// Sets all bits to zero except the given flag.
	BitMask& limit(const ENUM flag) {
		return limit({flag});
	}

	/// Sets all bits to zero except the bits in the given mask `other`.
	BitMask& limit(const BitMask other) {
		m_flags &= other.raw();
		return *this;
	}

	/// Returns a copy of the bit mask with all bits set to zero except those also set in `other`.
	BitMask limit(const BitMask other) const {
		auto ret = *this;
		return ret.limit(other);
	}

	/// Flip every bit in the bit mask.
	BitMask& flip() {
		m_flags = ~m_flags;
		return *this;
	}

	/// Flips the given bit position.
	BitMask& flip(const ENUM val) {
		m_flags ^= static_cast<EnumBaseType>(val);
		return *this;
	}

	/// Returns the number of set bits in the mask.
	size_t count() const {
		size_t ret = 0;

		for (size_t bit = 0; bit < size(); bit++) {
			auto val = 1 << bit;
			if (this->test(static_cast<ENUM>(val)))
				ret++;
		}

		return ret;
	}

	/// Returns the maximum number of bits that can be stored in the bit mask.
	constexpr size_t size() const {
		return sizeof(EnumBaseType) * 8;
	}

	/// like `test()`, but automatically reset the bit position if it's set.
	bool steal(const ENUM val) {
		if (!test(val)) {
			return false;
		} else {
			reset(val);
			return true;
		}
	}

	/// Returns whether the given bit position is set.
	/**
	 * \note If `val` consists of multiple bits then this only returns
	 * `true` if all of the bits it represents are set.
	 **/
	bool test(const ENUM val) const {
		const auto raw_val = static_cast<EnumBaseType>(val);
		return (m_flags & raw_val) == raw_val;
	}

	/// Returns whether any of the bits of `val` are set in the mask.
	/**
	 * This is only different to test() if the given value
	 * consists of multiple bit positions. In this case testAny() will
	 * return `true` even if only some of the bit positions are set in
	 * the mask, while test() will only return `true` if *all* of the bit
	 * positions are set.
	 **/
	bool testAny(const ENUM val) const {
		return ((m_flags & static_cast<EnumBaseType>(val)) != 0);
	}


	/// Returns whether this is the only bit position set in the mask.
	bool only(const ENUM val) const {
		return m_flags == static_cast<EnumBaseType>(val);
	}

	/// Returns whether any bit in the mask is set.
	bool any() const {
		return m_flags != 0;
	}

	/// Tests whether all of the given bit positions are set in the mask.
	bool allOf(const std::initializer_list<ENUM> &flags) const {
		for (auto val: flags) {
			if (!test(val))
				return false;
		}

		return true;
	}

	/// Tests whether all of the bits set in `other` are also set in this mask.
	bool allOf(const BitMask other) const {
		return (m_flags & other.m_flags) == other.m_flags;
	}

	/// Returns whether any of the given bit positions is set in the mask.
	bool anyOf(const std::initializer_list<ENUM> &flags) const {
		for (auto val: flags) {
			if (test(val))
				return true;
		}

		return false;
	}

	/// Returns whether any of the flags set in `other` is also set in this mask.
	bool anyOf(const BitMask other) const {
		return (m_flags & other.m_flags) != 0;
	}

	/// Returns whether no bit in the bitset is set.
	bool none() const {
		return !this->any();
	}

	bool operator==(const BitMask &other) const {
		return m_flags == other.m_flags;
	}

	bool operator!=(const BitMask &other) const {
		return !(*this == other);
	}

	/// Checks whether any bit of the given `val` is set, \see testAny().
	bool operator&(const ENUM val) const {
		return testAny(val);
	}

	/// returns an ENUM value containing only the values found in both masks.
	ENUM operator&(const BitMask &other) const {
		return ENUM{other.raw() & this->raw()};
	}

	/// Returns the flip()'ed mask.
	BitMask operator~() const {
		return BitMask{~m_flags};
	}

	/// Returns an object containing all the bits found in `first` without the bits found inc `second`.
	friend BitMask operator-(const BitMask &first, const BitMask &second) {
		BitMask ret{first};
		return ret.reset(second);
	}

	/// Returns an object containing all the bits found in `first` without `val`.
	friend BitMask operator-(const BitMask &first, const ENUM val) {
		BitMask ret{first};
		return ret.reset(val);
	}

	/// Returns an object containing all the bits found in `first` and `second`.
	friend BitMask operator+(const BitMask &first, const BitMask &second) {
		BitMask ret{first};
		return ret.set(second);
	}

	/// Returns an object containing all the bits found in `first` and /also `val`.
	friend BitMask operator+(const BitMask &first, const ENUM val) {
		BitMask ret{first};
		return ret.set(val);
	}

	/// Returns the union of `first` and `second`.
	friend BitMask operator|(const BitMask &first, const BitMask &second) {
		BitMask ret{first.m_flags | second.m_flags};
	}

protected: // data

	EnumBaseType m_flags = 0;
};

} // end ns
