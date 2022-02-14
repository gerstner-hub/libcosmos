#include <iostream>
#include <utility>

#include "cosmos/io/colors.hxx"

int main()
{
	using namespace cosmos::term;

	std::cout << Inversed("\ninversed text");

	std::cout << TermControl::RESET << "\nreset text\n",

	std::cout << Underlined("underlined text") << "\n";
	std::cout << Blinking("blinking text") << "\n";

	std::cout << Black("Black text") << "\n";
	std::cout << Blue("Blue text") << "\n";
	std::cout << Green("Green text") << "\n";
	std::cout << Yellow("Yellow text") << "\n";
	std::cout << Magenta("Magenta text") << "\n";
	std::cout << Cyan("Cyan text") << "\n";
	std::cout << White("White text") << "\n";
	std::cout << BrightBlack("Bright Black text") << "\n";
	std::cout << BrightBlue("Bright Blue text") << "\n";
	std::cout << BrightGreen("Bright Green text") << "\n";
	std::cout << BrightYellow("Bright Yellow text") << "\n";
	std::cout << BrightMagenta("Bright Magenta text") << "\n";
	std::cout << BrightCyan("Bright Cyan text") << "\n";
	std::cout << BrightWhite("Bright White text") << "\n";

	std::cout << Black(Blinking("black blinking text")) << "\n";
	std::cout << Red(Blinking("red blinking text")) << "\n";
	std::cout << Green(Underlined("green underlined text")) << "\n";
	std::cout << Yellow(Inversed("yellow inversed text")) << "\n";
	std::cout << Magenta(Blinking(Underlined("magenta blinking underlined text"))) << "\n";
	std::cout << Magenta(OnBrightBlack("magenta on bright black")) << "\n";
	std::cout << Underlined(Black(OnBrightYellow("underlined black on bright yellow"))) << "\n";
}
