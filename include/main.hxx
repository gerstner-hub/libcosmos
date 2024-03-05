#pragma once

// C++
#include <exception>
#include <iostream>
#include <type_traits>

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/cosmos.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/string.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

/// Class based main entry point that passes no command line arguments.
class MainNoArgs {
protected: // functions

	virtual ExitStatus main() = 0;

	template <typename MAIN>
	friend int main(const int argc, const char **argv);
};

/// Class based entry point that passes C-style command line arguments.
class MainPlainArgs {
protected: // functions

	virtual ExitStatus main(const int argc, const char **argv) = 0;

	template <typename MAIN>
	friend int main(const int argc, const char **argv);
};

/// Clsas based main() entry point that passes STL style command line arguments.
class MainContainerArgs {
protected: // functions

	virtual ExitStatus main(const std::string_view argv0, const StringViewVector &args) = 0;

	template <typename MAIN>
	friend int main(const int argc, const char **argv);
};

/// C++ wrapper for the main() application entry pont.
/**
 * This wrapper can be used to invoke a class member function to gain a
 * libcosmos C++ style entry point into the program along with automatic
 * library initialization and handling of uncaught exceptions as well as
 * ExitStatus propagation.
 *
 * Use this wrapper in the actual `main()` entry point of your program to
 * instantiate the MAIN class type. This type needs to implement one of the
 * main class interfaces MainNoArgs, MainPlainArgs or MainContainerArgs.
 *
 * These classes use the cosmos::ExitStatus strong type instead of the plain
 * `int` exit code. This offers improved readability and less room for
 * mistakes. To allow quick exit paths in the application you can throw a
 * plain cosmos::ExitStatus which will be catched in the wrapper and results
 * in a regular return of the integer value from the actual `main()` entry
 * point.
 *
 * Any other exceptions derived from std::exception will also be catched and
 * lead to an output on stderr and an exit status of ExitStatus::FAILURE. This
 * should be a last resort only, though, and you should handle non-fatal
 * exceptions in the implementation of your program to provide context aware
 * error messages.
 **/
template <typename MAIN>
int main(const int argc, const char **argv) {
	try {
		cosmos::Init m_init;
		MAIN instance;

		try {
			ExitStatus status;

			if constexpr (std::is_base_of_v<MainNoArgs, MAIN>) {
				status = static_cast<MainNoArgs&>(instance).main();
			} else if constexpr (std::is_base_of_v<MainPlainArgs, MAIN>) {
				status = static_cast<MainPlainArgs&>(instance).main(argc, argv);
			} else if constexpr (std::is_base_of_v<MainContainerArgs, MAIN>) {
				status = static_cast<MainContainerArgs&>(instance).main(
						std::string_view{argv[0]},
						StringViewVector{argv+1, argv+argc});
			} else {
				// we cannot use `false` here, because if the
				// condition does not depend on the template
				// parameter then it will still be evaluated,
				// even if the else branch doesn't match
				// during compile time!
				static_assert(sizeof(MAIN) == 0, "MAIN type does not implement a main function interface");
			}

			return to_integral(status);
		} catch (const cosmos::ExitStatus status) {
			return to_integral(status);
		} catch (const std::exception &ex) {
			std::cerr << "Unhandled exception: " << ex.what() << std::endl;
			return to_integral(ExitStatus::FAILURE);
		}
	} catch (const std::exception &ex) {
		// this outer exception handler handles exception thrown from
		// Init or MAIN constructors.
		std::cerr << "Error starting program: " << ex.what() << std::endl;
		return to_integral(ExitStatus::FAILURE);
	}
}

}; // end ns
