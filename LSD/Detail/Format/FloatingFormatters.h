/*************************
 * @file FloatingFormatters.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation for floating point formatter classes
 * 
 * @date 2024-10-31
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "FormatCore.h"
#include "FormatSpecs.h"
#include "FormatContext.h"

#include "../CoreUtility.h"
#include "../../String.h"
#include "../../StringView.h"

#include "FormatCore.h"

#include <cstddef>
#include <concepts>

namespace lsd {

namespace detail {

// float formatting mode utility

enum class FloatFormatMode {
	regular,
	scientific,
	fixed
};

// floating point formatter implementation

template <std::floating_point FloatType, class CharTy> struct FloatFormatter {
public:
	using value_type = FloatType;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using field_options = detail::BasicFieldOptions<char_type>;
	using format_spec = detail::BasicFormatSpec<value_type, char_type>;
	using context_type = BasicFormatContext<char_type>;

	using string_type = lsd::BasicString<char_type>;

	static void format(value_type value, context_type& context) {
		format_spec spec(context);

		auto& inserter = context.out();
		auto length = outputLength(value, spec);

		switch (spec.align) {
			case '<':
				if (inserter.done()) return;
				writeToOutput(value, inserter, spec);

				for (auto count = spec.width; !inserter.done() && count > length; count--) 
					inserter = spec.fillChr;

				break;

			case '>':
				for (auto count = spec.width; !inserter.done() && count > length; count--) 
						inserter = spec.fillChr;

				if (!inserter.done()) writeToOutput(value, inserter, spec);

				break;

			case '^':
				if (spec.width > length) {
					auto fillc = spec.width - length;
					auto half = fillc / 2;
					
					for (auto count = fillc - half; !inserter.done() && count > 0; count--) inserter = spec.fillChr;

					if (inserter.done()) return;
					writeToOutput(value, inserter, spec);

					for (; !inserter.done() && half > 0; half--) inserter = spec.fillChr;
				} else {
					if (inserter.done()) return;
					writeToOutput(value, inserter, spec);
				}

				break;
		}
	}

private:
	// general calculations
	static constexpr std::size_t sciExp(value_type value, std::size_t base) {
		auto logAbs = logn(static_cast<value_type>(base), std::abs(value));

		if (value > 1 || value < -1) return implicitCast<std::size_t>(logAbs);
		else return implicitCast<std::size_t>(std::abs(logAbs)) + 1;
	}

	// output length calculation

	static constexpr std::size_t outputLength(value_type value, const format_spec& spec) {
		if (std::isinf(value) || std::isnan(value)) return 3;

		if (spec.typeFormat.empty()) return signLen(value, spec.sign) + numLen(value, 10) + (spec.alternateForm ? (spec.precision + 1) : fracLen(value, 10, spec.precision, false));
		else {
			switch (spec.typeFormat[0]) {
				case 'a':
				case 'A':
					return signLen(value, spec.sign) + 3 + fracLen(value, 16, spec.precision + 1, spec.alternateForm) + sciExpLen(value, 2);

					break;

				case 'e':
				case 'E':
					return signLen(value, spec.sign) + 3 + fracLen(value, 10, spec.precision + 1, spec.alternateForm) + sciExpLen(value, 10);

					break;
				
				case 'f':
				case 'F':
					return signLen(value, spec.sign) + numLen(value, 10) + 1 + spec.precision;

					break;

				case 'g':
				case 'G':
					return signLen(value, spec.sign) + numLen(value, 10) + (spec.alternateForm ? (spec.precision + 1) : fracLen(value, 10, spec.precision, false));

					break;
			}
		}

		return 0;
	}
	static constexpr std::size_t signLen(value_type value, char_type sign) {
		switch (sign) {
			case '-':
				if (value < 0) return 1;
				else return 0;

				break;

			case '+':
			case ' ':
				return 1;

				break;
		}

		return 0;
	}
	static constexpr std::size_t sciExpLen(value_type value, std::size_t base) {
		return logn(base, sciExp(value, base));
	}
	static constexpr std::size_t fracLen(value_type value, std::size_t base, std::size_t precision, bool alternate) {
		value = std::abs(value);
		std::size_t l { };

		for (
			value_type diff { }; 
			diff > std::numeric_limits<value_type>::epsilon() && precision > 0;
			l++, value *= base, precision--) {
			diff = value - std::floor(value);
		}

		if (l) return ++l;
		else if (alternate) return 1;
		else return 0;
	}

	// output to the iterator

	static void writeToOutput(value_type value, back_inserter& inserter, const format_spec& spec) {
		// special inf or nan output

		if (auto inf = std::isinf(value), nan = std::isnan(value); inf || nan) {
			bool up = false;

			if (!spec.typeFormat.empty()) {
				auto c = spec.typeFormat[0];
				up = (c == 'A' || c == 'E' || c == 'F' || c == 'G');
			}

			auto str = (inf ? (up ? infUp : infLow) : (up ? nanUp : nanLow));

			inserter = str[0];
			if (inserter.done()) return;
			inserter = str[1];
			if (inserter.done()) return;
			inserter = str[2];

			return;
		}

		// format the number

		if (spec.typeFormat.empty()) outputNumberValue(value, inserter, 10, 10, false, spec, FloatFormatMode::regular);
		else {
			switch (spec.typeFormat[0]) {
				case 'a':
					outputNumberValue(value, inserter, 16, 2, true, spec, FloatFormatMode::scientific);

					break;

				case 'A':
					outputNumberValue(value, inserter, 16, 2, false, spec, FloatFormatMode::scientific);

					break;

				case 'e':
					outputNumberValue(value, inserter, 10, 10, true, spec, FloatFormatMode::scientific);

					break;

				case 'E':
					outputNumberValue(value, inserter, 10, 10, false, spec, FloatFormatMode::scientific);

					break;
				
				case 'f':
					outputNumberValue(value, inserter, 10, 10, true, spec, FloatFormatMode::fixed);

					break;

				case 'F':
					outputNumberValue(value, inserter, 10, 10, false, spec, FloatFormatMode::fixed);

					break;

				case 'g':
					outputNumberValue(value, inserter, 10, 10, true, spec, FloatFormatMode::regular);

					break;

				case 'G':
					outputNumberValue(value, inserter, 10, 10, false, spec, FloatFormatMode::regular);

					break;
			}
		}
	}
	static void outputNumberValue(
		value_type value,
		back_inserter& inserter,
		std::size_t numBase,
		std::size_t expBase,
		bool lowerCase,
		const format_spec& spec,
		FloatFormatMode formatMode
	) {
		// first handle prefixes

		switch (spec.sign) {
			case '+':
				if (value < 0) inserter = '-';
				if (value >= 0) inserter = '+';

				break;
			
			case '-':
				if (value < 0) inserter = '-';
			
				break;
			
			case ' ':
				if (value < 0) inserter = '-';
				if (value >= 0) inserter = ' ';
			
				break;
		}

		// now handle the number itself

		if (value < 0) value *= -1;

		string_type num;

		std::conditional_t<std::is_same_v<char_type, char>, const char*, const wchar_t*> digits { };
		if constexpr (std::is_same_v<char_type, char>) digits = (lowerCase ? digitsLow : digitsUp);
		else digits = (lowerCase ? wDigitsLow : wDigitsUp);

		if (formatMode == FloatFormatMode::scientific) {
			// write exponent part

			auto exp = sciExp(value, expBase);

			if (exp > 0) {
				while (exp > 0) {
					num.pushBack(digits[static_cast<std::size_t>(exp % numBase)]);
					exp /= numBase;
				}
			} else num.pushBack('0');

			if (value < 1) num.pushBack('-');
			else num.pushBack('+');

			if (numBase == 16) num.pushBack('p');
			else num.pushBack('e');


			// handle the rest of the number

			auto numCount = spec.precision + 1;

			auto sci = static_cast<std::size_t>(std::fmod(value, 1) * std::pow(10, numCount));

			// skip zeros
			for (; static_cast<std::size_t>(std::fmod(sci, numBase)) == 0; sci /= numBase, numCount--)
			;

			// write fractional part
			for (; numCount > 1; numCount--) {
				num.pushBack(digits[static_cast<std::size_t>(std::fmod(sci, numBase))]);
				sci /= numBase;
			}

			// write decimal point
			if (spec.alternateForm || !num.empty()) num.pushBack('.');

			// write single integer
			if (value >= 1) num.pushBack(digits[static_cast<std::size_t>(sci)]);
			else num.pushBack('0');
		} else {
			auto fractional = static_cast<std::size_t>(std::fmod(value, 1) * std::pow(10, spec.precision));

			if (!spec.alternateForm && formatMode == FloatFormatMode::regular) for (; static_cast<std::size_t>(std::fmod(fractional, numBase)) == 0; fractional /= numBase)
			;

			// write fractional part
			while (fractional > 0) {
				num.pushBack(digits[static_cast<std::size_t>(std::fmod(fractional, numBase))]);
				fractional /= numBase;
			}

			// write decimal point
			if (spec.alternateForm || !num.empty()) num.pushBack('.');

			// write integer part
			if (value >= 1) {
				while (value > 1) {
					num.pushBack(digits[static_cast<std::size_t>(std::fmod(value, numBase))]);
					value /= numBase;
				}
			} else num.pushBack('0');
		}

		// write the number to the output
		for (auto it = num.rbegin(); it != num.rend() && !inserter.done(); it++) inserter = *it;
	}
};

} // namespace detail


// floating point formatters

template <class CharTy> struct Formatter<float, CharTy> {
	void format(float value, BasicFormatContext<CharTy>& context) {
		detail::FloatFormatter<float, CharTy>::format(value, context);
	}
};
template <class CharTy> struct Formatter<double, CharTy> {	
	void format(double value, BasicFormatContext<CharTy>& context) {
		detail::FloatFormatter<double, CharTy>::format(value, context);
	}
};
template <class CharTy> struct Formatter<long double, CharTy> {
	void format(long double value, BasicFormatContext<CharTy>& context) {
		detail::FloatFormatter<long double, CharTy>::format(value, context);
	}
};

} // namespace lsd
