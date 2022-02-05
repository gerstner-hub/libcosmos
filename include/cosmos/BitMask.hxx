#ifndef COSMOS_ENUMBITSET_HXX
#define COSMOS_ENUMBITSET_HXX

// C++
#include <initializer_list>
#include <string>
#include <type_traits>

namespace cosmos
{

/**
 * \brief
 *	A typesafe bit mask representation using enums
 * \details
 * 	Instead of using a plain integer type and preprocessor constants to
 * 	denote certain bit positions, this type provides a type safe
 * 	implementation of a bitset with named bits based on a strongly typed
 * 	enum class.
 *
 * 	The interface is kept similar to that of std::bitset.
 **/
template <typename ENUM>
class BitMask
{
public:

	typedef typename std::underlying_type<ENUM>::type EnumBaseType;

	//! sets all bits to zero
	BitMask() { }

	//! sets only the flags found in the given initializer list
	explicit BitMask(const std::initializer_list<ENUM> &init_list)
	{
		for( auto bit: init_list )
		{
			m_flags |= static_cast<EnumBaseType>(bit);
		}
	}

	//! sets exactly the given bit to one
	explicit BitMask(const ENUM &bit) :
		m_flags( static_cast<EnumBaseType>(bit) )
	{}

	//! sets exactly the given primitive type bitmask
	explicit BitMask(const EnumBaseType &value) :
		m_flags(value)
	{}

	EnumBaseType get() const { return m_flags; }

	// conversion operator to the underlying enum type
	operator EnumBaseType() const { return m_flags; }

	// return a string representation of the bit mask
	operator std::string() const { return to_string(); }

	//! returns a boolean value for the given bit position
	bool operator[] (const ENUM &flag) const { return test(flag); }

	std::string to_string() const
	{
		std::string ret;

		// append each bit starting with the highest one
		for(int bit = size() - 1; bit >= 0; bit--)
		{
			const auto val = 1 << bit;
			ret.push_back( this->test(static_cast<ENUM>(val)) ? '1' : '0' );
		}

		return ret;
	}

	//! sets all bits it the set
	BitMask& set()
	{
		m_flags = ~EnumBaseType(0);
		return *this;
	}

	//! assigns the given value to the given bit position
	BitMask& set(const ENUM &bit, bool val = true)
	{
		const auto bitval = static_cast<EnumBaseType>(bit);
		m_flags = (val ? (m_flags|bitval) : (m_flags&~bitval));
		return *this;
	}

	//! sets all bits to zero
	BitMask& reset()
	{
		m_flags = EnumBaseType(0);
		return *this;
	}

	//! zeroes the given bit position
	BitMask& reset(const ENUM &bit)
	{
		m_flags = m_flags & ~static_cast<EnumBaseType>(bit);
		return *this;
	}

	//! flip each bit in the bit mask
	BitMask& flip()
	{
		m_flags = ~m_flags;
		return *this;
	}

	//! flips the given bit position
	BitMask& flip(const ENUM &bit)
	{
		m_flags ^= static_cast<EnumBaseType>(bit);
		return *this;
	}

	//! returns the number of set bits
	size_t count() const
	{
		size_t ret = 0;

		for( size_t bit = 0; bit < size(); bit++ )
		{
			auto val = 1 << bit;
			if( this->test(static_cast<ENUM>(val)) )
				ret++;
		}

		return ret;
	}

	//! returns the maximum number of bits that can be stored in the bit mask
	constexpr size_t size() const
	{
		return sizeof(EnumBaseType) * 8;
	}

	//! returns whether the given bit position is set
	bool test(const ENUM &bit) const
	{
	    return (m_flags & static_cast<EnumBaseType>(bit)) != 0;
	}

	//! returns whether any bit in the bitset is set
	bool any() const
	{
		return m_flags != 0;
	}

	//! returns whether no bit in the bitset is set
	bool none() const
	{
	    return !this->any();
	}

protected:

	EnumBaseType m_flags = 0;
};

} // end ns

#endif // inc. guard
