// C++
#include <iostream>

// cosmos
#include <cosmos/compiler.hxx>

int main() {
	std::cout << "complication target architecture  / ABI\n";
	if constexpr (cosmos::arch::X86_64) {
		std::cout << "X86_64\n";
	}
	if constexpr (cosmos::arch::X32) {
		std::cout << "X32\n";
	}
	if constexpr (cosmos::arch::I386) {
		std::cout << "I386\n";
	}
	if constexpr (cosmos::arch::ARM) {
		std::cout << "ARM\n";
	}
	if constexpr (cosmos::arch::AARCH64) {
		std::cout << "AARCH64\n";
	}
}
