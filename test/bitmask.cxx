// C++
#include <iostream>
#include <cassert>

// Cosmos
#include "cosmos/BitMask.hxx"

enum class MyEnum : int {
	VAL1 = 0x1,
	VAL2 = 0x2,
	VAL3 = 0x4,
	VAL4 = 0x8,
	VAL5 = 0x10,
	MASK45 = 0x18
};

typedef cosmos::BitMask<MyEnum> MyBitMask;

int main() {
	{
		MyBitMask bitmask;
		assert( bitmask.raw() == 0 );
	}

	{
		MyBitMask bitmask(MyEnum::VAL3);
		assert( bitmask.raw() == static_cast<int>(MyEnum::VAL3) );
	}

	{
		const auto val = static_cast<int>(MyEnum::VAL3) | static_cast<int>(MyEnum::VAL4);
		MyBitMask bitmask(val);
		assert (bitmask.raw() == val);
	}

	{
		MyBitMask bitmask({MyEnum::VAL2, MyEnum::VAL3});

		for (const auto bit: { MyEnum::VAL1, MyEnum::VAL2, MyEnum::VAL3, MyEnum::VAL4, MyEnum::VAL5 }) {
			bool expected = false;
			if (bit == MyEnum::VAL2 || bit == MyEnum::VAL3)
				expected = true;
			assert (bitmask[bit] == expected);
			assert (bitmask.test(bit) == expected);
		}

		std::string expected(27, '0');
		expected += "00110";
		assert (bitmask.to_string() == expected);
	}

	{
		MyBitMask bitmask;
		bitmask.set(MyEnum::VAL4);
		// should report true event if only one of the bits is set
		assert (bitmask[MyEnum::MASK45] == true);
		bitmask.set(MyEnum::VAL5);
		// when both are set it should still be true
		assert (bitmask[MyEnum::MASK45] == true);
		bitmask.set(MyEnum::VAL4, false);
		assert (bitmask[MyEnum::MASK45] == true);
		bitmask.set(MyEnum::VAL5, false);
		assert (bitmask[MyEnum::MASK45] == false);
		bitmask.set(MyEnum::VAL1);
		assert (bitmask[MyEnum::MASK45] == false);
	}

	{
		MyBitMask bitmask;
		bitmask.set(MyBitMask::all);
		assert (bitmask.to_string() == std::string(32, '1'));

		bitmask.reset();
		assert (bitmask.raw() == 0);

		bitmask.set( MyEnum::VAL3 );
		bitmask.set( MyEnum::VAL5 );
		assert (bitmask.raw() == (static_cast<int>(MyEnum::VAL3) | static_cast<int>(MyEnum::VAL5)));

		bitmask.reset( MyEnum::VAL3 );
		assert (bitmask.test(MyEnum::VAL3) == false);

		bitmask.flip( MyEnum::VAL3 );
		assert (bitmask.test(MyEnum::VAL3) == true);

		bitmask.reset();
		bitmask.flip();
		assert (bitmask.to_string() == std::string(32, '1'));

		assert (bitmask.count() == 32);

		assert (bitmask.size() == sizeof(int) * 8);

		assert (bitmask.any() == true);
		bitmask.reset();
		assert (bitmask.any() == false);
		assert (bitmask.none() == true);
	}

	{
		MyBitMask full(MyBitMask::all);
		for (const auto bit: { MyEnum::VAL1, MyEnum::VAL2, MyEnum::VAL3, MyEnum::VAL4, MyEnum::VAL5 }) {
			assert (full.test(bit) == true);
		}

		assert (full.only(MyEnum::VAL1) == false);

		full.limit({MyEnum::VAL1});

		for (const auto bit: { MyEnum::VAL1, MyEnum::VAL2, MyEnum::VAL3, MyEnum::VAL4, MyEnum::VAL5 }) {
			bool expected = false;
			if (bit == MyEnum::VAL1) {
				expected = true;
			}

			assert (full[bit] == expected);
		}


		assert (full.only(MyEnum::VAL1) == true);
	}

	return 0;
}
