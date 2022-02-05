// C++
#include <iostream>
#include <cassert>

#include "cosmos/BitMask.hxx"

enum class MyEnum : int
{
	VAL1 = 0x1,
	VAL2 = 0x2,
	VAL3 = 0x4,
	VAL4 = 0x8,
	VAL5 = 0x10
};

typedef cosmos::BitMask<MyEnum> MyBitMask;

int main()
{
	{
		MyBitMask bitmask;
		assert( bitmask.get() == 0 );
	}

	{
		MyBitMask bitmask(MyEnum::VAL3);
		assert( bitmask.get() == static_cast<int>(MyEnum::VAL3) );
	}

	{
		const auto val = static_cast<int>(MyEnum::VAL3) | static_cast<int>(MyEnum::VAL4);
		MyBitMask bitmask(val);
		assert( bitmask.get() == val );
	}

	{
		MyBitMask bitmask({MyEnum::VAL2, MyEnum::VAL3});
		const auto val = static_cast<int>(MyEnum::VAL2) | static_cast<int>(MyEnum::VAL3);
		assert( static_cast<int>(bitmask) == val );

		for( const auto bit: { MyEnum::VAL1, MyEnum::VAL2, MyEnum::VAL3, MyEnum::VAL4, MyEnum::VAL5 } )
		{
			bool expected = false;
			if( bit == MyEnum::VAL2 || bit == MyEnum::VAL3 )
				expected = true;
			assert( bitmask[bit] == expected );
			assert( bitmask.test(bit) == expected );
		}

		std::string expected(27, '0');
		expected += "00110";
		assert( bitmask.to_string() == expected );
	}

	{
		MyBitMask bitmask;
		bitmask.set();
		assert( bitmask.to_string() == std::string(32, '1') );

		bitmask.reset();
		assert( bitmask.get() == 0 );

		bitmask.set( MyEnum::VAL3 );
		bitmask.set( MyEnum::VAL5 );
		assert( bitmask.get() == (static_cast<int>(MyEnum::VAL3) | static_cast<int>(MyEnum::VAL5)) );

		bitmask.reset( MyEnum::VAL3 );
		assert( bitmask.test(MyEnum::VAL3) == false );

		bitmask.flip( MyEnum::VAL3 );
		assert( bitmask.test(MyEnum::VAL3) == true );

		bitmask.reset();
		bitmask.flip();
		assert( bitmask.to_string() == std::string(32, '1') );

		assert( bitmask.count() == 32 );

		assert( bitmask.size() == sizeof(int) * 8 );

		assert( bitmask.any() == true );
		bitmask.reset();
		assert( bitmask.any() == false );
		assert( bitmask.none() == true );
	}

	return 0;
}
