// C++
#include <iostream>
#include <cassert>

// Cosmos
#include "cosmos/BitMask.hxx"

// Test
#include "TestBase.hxx"

class BitMaskTest :
		public cosmos::TestBase {
protected: // types
	enum class MyEnum : int {
		VAL1 = 0x1,
		VAL2 = 0x2,
		VAL3 = 0x4,
		VAL4 = 0x8,
		VAL5 = 0x10,
		MASK45 = 0x18
	};

	typedef cosmos::BitMask<MyEnum> MyBitMask;
public:
	void run() {
		testCtors();
		testStringRep();
		testSetters();
		testFlip();
		testAllAndEmpty();
		testLimit();
	}

	void testCtors() {
		START_TEST("Constructor test");
		MyBitMask empty;

		START_STEP("Empty Bitmask Ctor");
		FINISH_STEP(empty.raw() == 0);

		MyBitMask valmask{MyEnum::VAL3};
		START_STEP("Single Enum Ctor");
		FINISH_STEP(valmask.raw() == static_cast<int>(MyEnum::VAL3));

		const auto val = cosmos::to_integral(MyEnum::VAL3) | cosmos::to_integral(MyEnum::VAL4);
		MyBitMask rawmask{val};
		START_STEP("Raw bitmask Ctor");
		FINISH_STEP(rawmask.raw() == val);

		MyBitMask initlist{{MyEnum::VAL2, MyEnum::VAL3}};

		for (const auto bit: {MyEnum::VAL1, MyEnum::VAL2, MyEnum::VAL3, MyEnum::VAL4, MyEnum::VAL5}) {
			bool expected = false;
			if (bit == MyEnum::VAL2 || bit == MyEnum::VAL3)
				expected = true;

			START_STEP(cosmos::sprintf("Initlist Ctor VAL%02x", cosmos::to_integral(bit)));
			FINISH_STEP(initlist[bit] == expected && initlist.test(bit) == expected);
		}

	}

	void testStringRep() {
		START_TEST("String Representation");

		MyBitMask initlist({MyEnum::VAL2, MyEnum::VAL3});
		std::string expected(27, '0');
		expected += "00110";

		START_STEP("toString() of {VAL2, VAL3}");
		FINISH_STEP(initlist.toString() == expected);
	}

	void testSetters() {
		START_TEST("Setter Test");
		MyBitMask bitmask;

		START_STEP("Setting various bitmask combinations");
		// should report true even if only one of the bits is set
		bitmask.set(MyEnum::VAL4);
		EVAL_STEP(bitmask[MyEnum::MASK45] == true);

		bitmask.set(MyEnum::VAL5);
		// when both are set it should still be true
		EVAL_STEP(bitmask[MyEnum::MASK45] == true);

		bitmask.set(MyEnum::VAL4, false);
		EVAL_STEP(bitmask[MyEnum::MASK45] == true);

		bitmask.set(MyEnum::VAL5, false);
		EVAL_STEP(bitmask[MyEnum::MASK45] == false);

		bitmask.set(MyEnum::VAL1);
		FINISH_STEP(bitmask[MyEnum::MASK45] == false);

		START_STEP("setting all bits");
		bitmask.set(MyBitMask::all);
		FINISH_STEP(bitmask.toString() == std::string(32, '1'));

		bitmask.reset();
		START_STEP("reset() after setting all");
		FINISH_STEP(bitmask.raw() == 0);

		START_STEP("setting VAL3, VAL5");
		bitmask.set(MyEnum::VAL3);
		bitmask.set(MyEnum::VAL5);
		FINISH_STEP(bitmask.raw() == (static_cast<int>(MyEnum::VAL3) | static_cast<int>(MyEnum::VAL5)));

		START_STEP("unsetting VAL3");
		bitmask.reset(MyEnum::VAL3);
		FINISH_STEP(bitmask.test(MyEnum::VAL3) == false);
	}

	void testFlip() {
		START_TEST("Flip Test");

		MyBitMask bitmask;
		START_STEP("flipping VAL3");
		bitmask.flip( MyEnum::VAL3 );
		FINISH_STEP (bitmask.test(MyEnum::VAL3) == true);

		bitmask.reset();
		START_STEP("flipping all-zero mask");
		bitmask.flip();
		FINISH_STEP (bitmask.toString() == std::string(32, '1'));
	}

	void testAllAndEmpty() {
		START_TEST("Properties of all/no bits set");
		MyBitMask bitmask{MyBitMask::all};

		START_STEP("Testing all bits set");
		EVAL_STEP(bitmask.count() == 32);
		EVAL_STEP(bitmask.size() == sizeof(int) * 8);
		for (const auto bit: {MyEnum::VAL1, MyEnum::VAL2, MyEnum::VAL3, MyEnum::VAL4, MyEnum::VAL5}) {
			EVAL_STEP(bitmask.test(bit) == true);
		}
		EVAL_STEP(bitmask.only(MyEnum::VAL1) == false);
		FINISH_STEP(bitmask.any() == true);

		bitmask.reset();
		START_STEP("Testing no bits set");
		EVAL_STEP(bitmask.any() == false);
		FINISH_STEP(bitmask.none() == true);
	}

	void testLimit() {
		START_TEST("Test limiting full bitmask");
		MyBitMask full{MyBitMask::all};
		START_STEP("limit to VAL1");
		full.limit({MyEnum::VAL1});

		for (const auto bit: {MyEnum::VAL1, MyEnum::VAL2, MyEnum::VAL3, MyEnum::VAL4, MyEnum::VAL5}) {
			bool expected = false;
			if (bit == MyEnum::VAL1) {
				expected = true;
			}

			EVAL_STEP(full[bit] == expected);
		}


		FINISH_STEP(full.only(MyEnum::VAL1) == true);
	}
};


int main() {
	BitMaskTest test;
	test.run();

	return test.finishTest();
}
