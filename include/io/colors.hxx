#ifndef COSMOS_COLORS_HXX
#define COSMOS_COLORS_HXX

// C++
#include <iosfwd>
#include <string_view>
#include <variant>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/dso_export.h"

/**
 * @file
 *
 * This header provides ANSI terminal features for printing colored or
 * otherwise marked up text. The ostream operators work only for terminal
 * devices, see Terminal::isTTY().
 *
 * This is in a separate namespace due to a lot of symbol pollution. Since a
 * lot of types are in here a `using namespace cosmos::term` can help making
 * code more compact by selectively importing this namespace.
 **/

namespace cosmos::term {

/// Primitive Colors for ANSI Terminals.
/**
 *  There are 16 different colors when taking bright variants into account.
 *  The colors can be used both for text color and background color.
 **/
enum class TermColor : size_t {
	// the integer values denote the offset from the base ANSI escape code
	// for front/back bright/normal colors.
	BLACK = 0,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE
};

/// Differentiation between text (front) and background color.
enum class ColorKind {
	FRONT,
	BACK
};

/// Differentiation of normal and bright color intensity.
enum class ColorIntensity {
	NORMAL,
	BRIGHT
};

/// Complete color specification for ANSI terminals.
/**
 *  This type carries a complete color specification:
 *
 *  - basic color value
 *  - FRONT or BACK color
 *  - NORMAL or BRIGHT intensity
 *
 *  It can be used to permanently change the color setting of the terminal
 *  and is also used for FeatureBase to build more complex color
 *  mechanisms.
 **/
class ColorSpec {
public: // functions

	ColorSpec(const TermColor color, const ColorKind kind, const ColorIntensity intensity) :
		m_color(color), m_kind(kind), m_intensity(intensity) {}

	TermColor getColor() const { return m_color; }
	bool isBright() const { return m_intensity == ColorIntensity::BRIGHT; }
	bool isNormal() const { return !isBright(); }
	bool isFrontColor() const { return m_kind == ColorKind::FRONT; }
	bool isBackColor() const { return !isFrontColor(); }

protected: // data

	TermColor m_color;
	ColorKind m_kind = ColorKind::FRONT;
	ColorIntensity m_intensity = ColorIntensity::NORMAL;
};

/// Simple type to represent an ANSI foreground color in bright or normal intensity.
struct FrontColor : public ColorSpec {
	explicit FrontColor(const TermColor c) : ColorSpec(c, ColorKind::FRONT, ColorIntensity::NORMAL) {}
	FrontColor& setBright() { m_intensity = ColorIntensity::BRIGHT; return *this; }
};

/// Simple type to represent an ANSI background color in bright or normal intensity.
struct BackColor : public ColorSpec {
	explicit BackColor(const TermColor c) : ColorSpec(c, ColorKind::BACK, ColorIntensity::NORMAL) {}
	BackColor& setBright() { m_intensity = ColorIntensity::BRIGHT; return *this; }
};

/// Various feature controls for ANSI terminals.
enum class TermControl : size_t {
	RESET            = 0,  ///< Remove all attributes currently set (including colors)
	UNDERLINE_ON     = 4,  ///< Underlined text
	UNDERLINE_OFF    = 24,
	BLINK_ON         = 5,  ///< Blinking text
	BLINK_OFF        = 25,
	INVERSE_ON       = 7,  ///< Inverse fg/bg colors
	INVERSE_OFF      = 27,
	DEFAULT_FG_COLOR = 39, ///< Set default fg color
	DEFAULT_BG_COLOR = 49  ///< Set default bg color
};

/// A generic ANSI code e.g. for color indices
enum class ANSICode : size_t {
};

/// Returns the matching _OFF value for an _ON value of the TermControl enum.
/**
 * This can throw an exception on unexpected input.
 **/
TermControl COSMOS_API get_off_control(const TermControl ctrl);
/// Returns the actual ANSI escape code number for the given color specification.
ANSICode COSMOS_API get_ansi_color_code(const ColorSpec &color);

/// Base class to build nested ANSI feature objects.
/**
 *  Each instance of this object either holds a piece of text or a pointer to
 *  the next feature to be applied. Each object at this level only knows the
 *  ANSI codes to enable and disable the feature it represents.
 *
 *  The output operator thus can apply all desired features for the piece of
 *  text and restore the original terminal features after the output is
 *  finished.
 *
 *  This allows nested features like
 *      std::cout << Green(OnBlue("my green on //blue text")) << "\n";
 **/
class FeatureBase {
public:
	ANSICode getOnCode() const { return m_on_code; }
	ANSICode getOffCode() const { return m_off_code; }
	bool hasText() const { return std::holds_alternative<const std::string_view*>(m_info); }
	const std::string_view& getText() const { return *std::get<const std::string_view*>(m_info); }
	bool hasNextFeature() const { return std::holds_alternative<const FeatureBase*>(m_info); }
	const FeatureBase& getNextFeature() const { return *std::get<const FeatureBase*>(m_info); }
protected:
	FeatureBase(const std::string_view &text, const ANSICode on_code, const ANSICode off_code) :
		m_info(&text), m_on_code(on_code), m_off_code(off_code)
	{}

	FeatureBase(const FeatureBase &next, const ANSICode on_code, const ANSICode off_code) :
		m_info(&next), m_on_code(on_code), m_off_code(off_code)
	{}
protected:
	// TODO: this way of stacking features is prone to memory corruption
	// if the used pointers lose validity ... it works for a stack of
	// temporary objects but in some non-standard use cases errors can
	// occur.
	/// either a terminal string or a pointer to the next feature to apply.
	std::variant<const std::string_view*, const FeatureBase*> m_info;
	const ANSICode m_on_code;
	const ANSICode m_off_code;
};

/// Base class for easy feature TermControl application on ostreams
class TextEffect :
		public FeatureBase {
protected:
	TextEffect(const TermControl feature, const std::string_view &text) :
			FeatureBase{
				text,
				ANSICode{to_integral(feature)},
				ANSICode{to_integral(get_off_control(feature))}
			}
	{}

	TextEffect(const TermControl feature, const FeatureBase &next) :
			FeatureBase{
				next,
				ANSICode{to_integral(feature)},
				ANSICode{to_integral(get_off_control(feature))}
			}
	{}
};

/// Template for definition of concrete text effect helpers.
template <TermControl effect>
struct TextEffectT : public TextEffect {
	explicit TextEffectT(const std::string_view &text) :
		TextEffect{effect, text} {}
	explicit TextEffectT(const FeatureBase &next) :
		TextEffect{effect, next} {}
};

/// Helper to print underlined text easily onto an ostream.
typedef TextEffectT<TermControl::UNDERLINE_ON> Underlined;
/// Helper to print blinking text easily onto an ostream.
typedef TextEffectT<TermControl::BLINK_ON> Blinking;
/// Helper to print blinking text easily onto an ostream.
typedef TextEffectT<TermControl::INVERSE_ON> Inversed;

/// Base class for easy colored text application on ostreams.
struct ColoredText : public FeatureBase {
	explicit ColoredText(const std::string_view &text, const TermColor c, const ColorKind kind, const ColorIntensity intensity) :
		FeatureBase{
			text,
			get_ansi_color_code(ColorSpec{c, kind, intensity}),
			getOffCode(kind)
		}
	{}

	explicit ColoredText(const FeatureBase &next, const TermColor c, const ColorKind kind, const ColorIntensity intensity) :
		FeatureBase{
			next,
			get_ansi_color_code(ColorSpec{c, kind, intensity}),
			getOffCode(kind)
		}
	{}

protected: // functions

	ANSICode getOffCode(const ColorKind kind) const {
		auto ret = kind == ColorKind::FRONT ? TermControl::DEFAULT_FG_COLOR : TermControl::DEFAULT_BG_COLOR;
		return ANSICode{to_integral(ret)};
	}
};

/// Template for definition of concrete color text helpers.
template <TermColor color, ColorIntensity intensity = ColorIntensity::NORMAL>
struct ColoredTextT : public ColoredText {
	explicit ColoredTextT(const std::string_view &text) : ColoredText{text, color, ColorKind::FRONT, intensity} {}
	explicit ColoredTextT(const FeatureBase &next) : ColoredText{next, color, ColorKind::FRONT, intensity} {}
};

/// Template for definition of concrete background color helpers.
template <TermColor color, ColorIntensity intensity = ColorIntensity::NORMAL>
struct TextOnColorT : public ColoredText {
	explicit TextOnColorT(const std::string_view &text) : ColoredText{text, color, ColorKind::BACK, intensity} {}
	explicit TextOnColorT(const FeatureBase &next) : ColoredText{next, color, ColorKind::BACK, intensity} {}
};

/*
 * Helpers to apply colored text and background easily on an ostream.
 *
 * These only enable the color for the given string argument and disable
 * coloring after the output operation again.
 *
 * These helpers can be nested with text effects or background helpers to
 * create readable chained effects for a string like:
 *
 * std::cout << Green(OnWhite(Underlined("text"))) << std::endl;
 */

typedef ColoredTextT<TermColor::BLACK> Black;
typedef ColoredTextT<TermColor::BLACK, ColorIntensity::BRIGHT> BrightBlack;
typedef TextOnColorT<TermColor::BLACK> OnBlack;
typedef TextOnColorT<TermColor::BLACK, ColorIntensity::BRIGHT> OnBrightBlack;

typedef ColoredTextT<TermColor::RED> Red;
typedef ColoredTextT<TermColor::RED, ColorIntensity::BRIGHT> BrightRed;
typedef TextOnColorT<TermColor::RED> OnRed;
typedef TextOnColorT<TermColor::RED, ColorIntensity::BRIGHT> OnBrightRed;

typedef ColoredTextT<TermColor::GREEN> Green;
typedef ColoredTextT<TermColor::GREEN, ColorIntensity::BRIGHT> BrightGreen;
typedef TextOnColorT<TermColor::GREEN> OnGreen;
typedef TextOnColorT<TermColor::GREEN, ColorIntensity::BRIGHT> OnBrightGreen;

typedef ColoredTextT<TermColor::YELLOW> Yellow;
typedef ColoredTextT<TermColor::YELLOW, ColorIntensity::BRIGHT> BrightYellow;
typedef TextOnColorT<TermColor::YELLOW> OnYellow;
typedef TextOnColorT<TermColor::YELLOW, ColorIntensity::BRIGHT> OnBrightYellow;

typedef ColoredTextT<TermColor::BLUE> Blue;
typedef ColoredTextT<TermColor::BLUE, ColorIntensity::BRIGHT> BrightBlue;
typedef TextOnColorT<TermColor::BLUE> OnBlue;
typedef TextOnColorT<TermColor::BLUE, ColorIntensity::BRIGHT> OnBrightBlue;

typedef ColoredTextT<TermColor::MAGENTA> Magenta;
typedef ColoredTextT<TermColor::MAGENTA, ColorIntensity::BRIGHT> BrightMagenta;
typedef TextOnColorT<TermColor::MAGENTA> OnMagenta;
typedef TextOnColorT<TermColor::MAGENTA, ColorIntensity::BRIGHT> OnBrightMagenta;

typedef ColoredTextT<TermColor::CYAN> Cyan;
typedef ColoredTextT<TermColor::CYAN, ColorIntensity::BRIGHT> BrightCyan;
typedef TextOnColorT<TermColor::CYAN> OnCyan;
typedef TextOnColorT<TermColor::CYAN, ColorIntensity::BRIGHT> OnBrightCyan;

typedef ColoredTextT<TermColor::WHITE> White;
typedef ColoredTextT<TermColor::WHITE, ColorIntensity::BRIGHT> BrightWhite;
typedef TextOnColorT<TermColor::WHITE> OnWhite;
typedef TextOnColorT<TermColor::WHITE, ColorIntensity::BRIGHT> OnBrightWhite;

} // end ns

COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::term::ColorSpec &fc);
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::term::TermControl p);
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::term::FeatureBase &fb);

#endif // inc. guard
