// C++
#include <cassert>
#include <ostream>
#include <vector>

// cosmos
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/io/colors.hxx"

namespace cosmos::term {

static std::string buildANSICommand(const std::vector<ANSICode> &cmd_list) {
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

static std::string buildANSICommand(const ANSICode code) {
	return buildANSICommand(std::vector<ANSICode>({code}));
}

ANSICode getANSIColorCode(const ColorSpec &color) {
	size_t code = color.isFrontColor() ? 30 : 40;
	if (color.isBright())
		code += 60;

	return ANSICode{code + to_integral(color.getColor())};
}

TermControl getOffControl(const TermControl ctrl) {
	switch(ctrl) {
	case TermControl::UNDERLINE_ON: return TermControl::UNDERLINE_OFF;
	case TermControl::BLINK_ON:     return TermControl::BLINK_OFF;
	case TermControl::INVERSE_ON:   return TermControl::INVERSE_OFF;
	default: cosmos_throw (UsageError("bad value"));
		 return TermControl::RESET; /* to silence compiler warning */
	}
}

static std::vector<const FeatureBase*> getFeatures(const FeatureBase &first) {
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

} // end ns


std::ostream& operator<<(std::ostream &o, const cosmos::term::ColorSpec &fc) {
	const auto code = getANSIColorCode(fc);

	return o << cosmos::term::buildANSICommand(code);
}

std::ostream& operator<<(std::ostream &o, const cosmos::term::TermControl p) {
	return o << cosmos::term::buildANSICommand(cosmos::term::ANSICode{cosmos::to_integral(p)});
}

std::ostream& operator<<(std::ostream &o, const cosmos::term::FeatureBase &fb) {
	using namespace cosmos::term;

	// simple case without dynamic allocation
	if (!fb.hasNextFeature()) {
		return o << buildANSICommand(fb.getOnCode()) << fb.getText() << buildANSICommand(fb.getOffCode());
	}

	// complex case with nested features
	std::vector<cosmos::term::ANSICode> on_codes, off_codes;
	const auto features = getFeatures(fb);
	for (const auto feature: features) {
		on_codes.push_back(feature->getOnCode());
		off_codes.push_back(feature->getOffCode());
	}

	assert (features.back()->hasText());
	return o << buildANSICommand(on_codes) << features.back()->getText() << buildANSICommand(off_codes);
}
