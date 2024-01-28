// C++
#include <cassert>
#include <iostream>
#include <ostream>
#include <vector>

// cosmos
#include "cosmos/error/UsageError.hxx"
#include "cosmos/io/colors.hxx"
#include "cosmos/io/Terminal.hxx"
#include "cosmos/private/Initable.hxx"
#include "cosmos/proc/process.hxx"

namespace cosmos::term {

namespace {

	bool stdout_is_tty = true;
	bool stderr_is_tty = true;

	bool isTTYStream(const std::ostream &o) {
		auto rdbuf = o.rdbuf();

		if (rdbuf == std::cout.rdbuf())
			return stdout_is_tty;
		else if (rdbuf == std::cerr.rdbuf())
			return stderr_is_tty;

		return false;
	}

	std::string build_ansi_command(const std::vector<ANSICode> &cmd_list) {
		// the ASCII escape character for ANSI control sequences
		constexpr char ANSI_ESC_START = '\033';
		// ANSI escape finish character
		constexpr char ANSI_ESC_FINISH = 'm';

		std::string ret(1, ANSI_ESC_START);
		ret += '[';
		for (const auto &cmd: cmd_list) {
			ret += std::to_string(to_integral(cmd));
			ret += ";";
		}
		// remove superfluous separator again
		ret.pop_back();
		ret += ANSI_ESC_FINISH;
		return ret;
	}

	std::string build_ansi_command(const ANSICode code) {
		return build_ansi_command(std::vector<ANSICode>({code}));
	}

	std::vector<const FeatureBase*> get_features(const FeatureBase &first) {
		std::vector<const FeatureBase*> ret;
		const FeatureBase *feature = &first;

		while (true) {
			ret.push_back(feature);

			if (!feature->hasNextFeature())
				break;

			feature = &feature->getNextFeature();
		}

		return ret;
	}

} // end anon ns

void refresh_tty_detection() {
	if (proc::get_env_var("COSMOS_FORCE_COLOR_ON")) {
		stdout_is_tty = stderr_is_tty = true;
		return;
	} else if (proc::get_env_var("COSMOS_FORCE_COLOR_OFF")) {
		stdout_is_tty = stderr_is_tty = false;
		return;
	}

	Terminal output{FileDescriptor{FileNum::STDOUT}};
	Terminal error{FileDescriptor{FileNum::STDERR}};

	stdout_is_tty = output.isTTY();
	stderr_is_tty = error.isTTY();
}

ANSICode get_ansi_color_code(const ColorSpec &color) {
	size_t code = color.isFrontColor() ? 30 : 40;
	if (color.isBright())
		code += 60;

	return ANSICode{code + to_integral(color.getColor())};
}

TermControl get_off_control(const TermControl ctrl) {
	switch(ctrl) {
	case TermControl::UNDERLINE_ON: return TermControl::UNDERLINE_OFF;
	case TermControl::BLINK_ON:     return TermControl::BLINK_OFF;
	case TermControl::INVERSE_ON:   return TermControl::INVERSE_OFF;
	default: cosmos_throw (UsageError("bad value"));
		 return TermControl::RESET; /* to silence compiler warning */
	}
}

namespace {

/// Init helper that adjusts the running_on_valgrind setting, if necessary.
class CheckStdioTTYsInit :
		public Initable {
public: // functions

	CheckStdioTTYsInit() : Initable(InitPrio::CHECK_STDIO_TTYS) {
	}

protected: // functions

	void libInit() override {
		refresh_tty_detection();
	}

	void libExit() override {
		// no-op
	}
};

CheckStdioTTYsInit g_check_stdio_ttys;

} // end anon ns

} // end ns


std::ostream& operator<<(std::ostream &o, const cosmos::term::ColorSpec &fc) {
	if (!cosmos::term::isTTYStream(o))
		return o;
	const auto code = get_ansi_color_code(fc);

	return o << cosmos::term::build_ansi_command(code);
}

std::ostream& operator<<(std::ostream &o, const cosmos::term::TermControl p) {
	if (!cosmos::term::isTTYStream(o))
		return o;
	return o << cosmos::term::build_ansi_command(cosmos::term::ANSICode{cosmos::to_integral(p)});
}

std::ostream& operator<<(std::ostream &o, const cosmos::term::FeatureBase &fb) {
	using namespace cosmos::term;

	if (!cosmos::term::isTTYStream(o)) {
		if (!fb.hasNextFeature())
			return o << fb.getText();
		else {
			const auto features = get_features(fb);
			return o << features.back()->getText();
		}
	}

	// simple case without dynamic allocation
	if (!fb.hasNextFeature()) {
		return o << build_ansi_command(fb.getOnCode()) << fb.getText() << build_ansi_command(fb.getOffCode());
	}

	// complex case with nested features
	std::vector<cosmos::term::ANSICode> on_codes, off_codes;
	const auto features = get_features(fb);
	for (const auto feature: features) {
		on_codes.push_back(feature->getOnCode());
		off_codes.push_back(feature->getOffCode());
	}

	assert (features.back()->hasText());
	return o << build_ansi_command(on_codes) << features.back()->getText() << build_ansi_command(off_codes);
}
