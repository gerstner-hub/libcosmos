#ifndef COSMOS_ENUMBITSET_HXX
#define COSMOS_ENUMBITSET_HXX

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

	/// Sets all bits zo one.
	explicit BitMask(const All a) {
		set(a);
	}

	/// Sets only the flags found in the given initializer list.
	explicit constexpr BitMask(const std::initializer_list<ENUM> &init_list) {
		for (auto val: init_list) {
			m_flags |= static_cast<EnumBaseType>(val);
		}
	}

	/// Sets exactly the given value to one.
	explicit constexpr BitMask(const ENUM val) :
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

	/// Returns a boolean value for the given value, \see test().
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

	/// Sets all bits it the set.
	BitMask& set(const All) {
		m_flags = ~EnumBaseType(0);
		return *this;
	}

	/// Set or unset the given value.
	BitMask& set(const ENUM val, bool on_off = true) {
		const auto bitval = static_cast<EnumBaseType>(val);
		m_flags = (on_off ? (m_flags|bitval) : (m_flags & ~bitval));
		return *this;
	}

	BitMask& set(const std::initializer_list<ENUM> &flags) {
		for (auto flag: flags) {
			set(flag);
		}
		return *this;
	}

	/// Sets all the bits that are also set in \c other.
	BitMask& set(const BitMask other) {
		m_flags |= other.m_flags;
		return *this;
	}

	/// Sets all bits to zero.
	BitMask& reset() {
		m_flags = EnumBaseType{0};
		return *this;
	}

	/// Zeroes the given value.
	BitMask& reset(const ENUM val) {
		reset({val});
		return *this;
	}

	/// Zeroes all of the given values.
	BitMask& reset(const std::initializer_list<ENUM> &flags) {
		for (auto val: flags) {
			m_flags &= ~static_cast<EnumBaseType>(val);
		}
		return *this;
	}

	BitMask& reset(const BitMask other) {
		m_flags &= ~(other.raw());
		return *this;
	}

	BitMask reset(const BitMask other) const {
		auto ret = *this;
		return ret.reset(other);
	}

	/// Sets all bits to zero except the given flags.
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

	BitMask& limit(const BitMask other) {
		m_flags &= other.raw();
		return *this;
	}

	BitMask limit(const BitMask other) const {
		auto ret = *this;
		return ret.limit(other);
	}

	/// Flip every bit in the bit mask.
	BitMask& flip() {
		m_flags = ~m_flags;
		return *this;
	}

	/// Flips the given value.
	BitMask& flip(const ENUM val) {
		m_flags ^= static_cast<EnumBaseType>(val);
		return *this;
	}

	/// Returns the number of set bits.
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

	/// Returns whether the given value is set.
	/**
	 * \note If \c val consists of multiple bits then this only returns \c
	 * true if all of the bits it represents are set.
	 **/
	bool test(const ENUM val) const {
		const auto raw_val = static_cast<EnumBaseType>(val);
		return (m_flags & raw_val) == raw_val;
	}

	/// Returns whether any of the bits of \c val are set.
	/**
	 * This is only different to test() if the given value
	 * consists of multiple bit positions. In this case testAny() will
	 * return \c true even if only some of the bit positions are set in
	 * the mask, while test() will only return \c true if *all* of the bit
	 * positions are set.
	 **/
	bool testAny(const ENUM val) const {
		return ((m_flags & static_cast<EnumBaseType>(val)) != 0);
	}


	/// Returns whether this is the only value set.
	bool only(const ENUM val) const {
		return m_flags == static_cast<EnumBaseType>(val);
	}

	/// Returns whether any bit in the bitset is set.
	bool any() const {
		return m_flags != 0;
	}

	/// Tests whether all of the given values are set.
	bool allOf(const std::initializer_list<ENUM> &flags) const {
		for (auto val: flags) {
			if (!test(val))
				return false;
		}

		return true;
	}

	/// Returns whether any of the given values is set.
	bool anyOf(const std::initializer_list<ENUM> &flags) const {
		for (auto val: flags) {
			if (test(val))
				return true;
		}

		return false;
	}

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

	/// Checks whether any bit of the given value is set, \see testAny().
	bool operator&(const ENUM val) const {
		return testAny(val);
	}

	/// returns an ENUM value containing only the values found in both masks.
	ENUM operator&(const BitMask &other) const {
		return ENUM{other.raw() & this->raw()};
	}

	BitMask operator~() const {
		return BitMask{~m_flags};
	}

	/// Returns an object containing all the bits found in \c first without the bits found inc \c second.
	friend BitMask operator-(const BitMask &first, const BitMask &second) {
		BitMask ret{first};
		return ret.reset(second);
	}

	/// Returns an object containing all the bits found in \c first without \c val.
	friend BitMask operator-(const BitMask &first, const ENUM val) {
		BitMask ret{first};
		return ret.reset(val);
	}

	/// Returns an object containing all the bits found in \c first and \c secopnd.
	friend BitMask operator+(const BitMask &first, const BitMask &second) {
		BitMask ret{first};
		return ret.set(second);
	}

	/// Returns an object containing all the bits found in \c first and /also \c val.
	friend BitMask operator+(const BitMask &first, const ENUM val) {
		BitMask ret{first};
		return ret.set(val);
	}

protected: // data

	EnumBaseType m_flags = 0;
};

} // end ns

#endif // inc. guard
