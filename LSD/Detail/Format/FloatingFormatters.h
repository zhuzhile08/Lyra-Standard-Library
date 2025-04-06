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

// Float formatting mode utility

enum class FloatFormatMode {
	regular,
	scientific,
	fixed
};

// Floating point formatter implementation

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
		auto result = generateResult(value, spec);

		switch (spec.align) {
			case '<':
				if (!writeToOutput(result, inserter)) return;

				for (auto count = spec.width; !inserter.done() && count > result.size(); count--) 
					inserter = spec.fillChr;

				break;

			case '>':
				for (auto count = spec.width; count > result.size(); count--) {
					if (inserter.done()) return;
					inserter = spec.fillChr;
				}

				writeToOutput(result, inserter);

				break;

			case '^':
				if (spec.width > result.size()) {
					auto fillc = spec.width - result.size();
					auto half = fillc / 2;
					
					for (auto count = fillc - half; count > 0; count--) {
						if (inserter.done()) return;
						inserter = spec.fillChr;
					}

					if (!writeToOutput(result, inserter)) return;

					for (; !inserter.done() && half > 0; half--) inserter = spec.fillChr;
				} else writeToOutput(result, inserter);
		}
	}

private:
	static string_type generateResult(value_type value, const format_spec& spec) {
		// Special inf or nan output
		
		if (auto inf = std::isinf(value), nan = std::isnan(value); inf || nan) {
			bool up = false;

			if (!spec.typeFormat.empty()) {
				auto c = spec.typeFormat[0];
				up = (c == 'A' || c == 'E' || c == 'F' || c == 'G');
			}
			
			if constexpr (std::is_same_v<char_type, char>)
				return (inf ? (up ? "INF" : "inf") : (up ? "NAN" : "nan"));
			else
				return (inf ? (up ? L"INF" : L"inf") : (up ? L"NAN" : L"nan"));
		}

		if (spec.typeFormat.empty()) return numberResult(value, 10, 10, false, spec, FloatFormatMode::regular);
		else {
			switch (spec.typeFormat[0]) {
				case 'a':
					return numberResult(value, 16, 2, true, spec, FloatFormatMode::scientific);

				case 'A':
					return numberResult(value, 16, 2, false, spec, FloatFormatMode::scientific);

				case 'e':
					return numberResult(value, 10, 10, true, spec, FloatFormatMode::scientific);

				case 'E':
					return numberResult(value, 10, 10, false, spec, FloatFormatMode::scientific);
				
				case 'f':
					return numberResult(value, 10, 10, true, spec, FloatFormatMode::fixed);

				case 'F':
					return numberResult(value, 10, 10, false, spec, FloatFormatMode::fixed);

				case 'g':
					return numberResult(value, 10, 10, true, spec, FloatFormatMode::regular);

				case 'G':
					return numberResult(value, 10, 10, false, spec, FloatFormatMode::regular);
			}
		}

		return string_type();
	}

	static constexpr std::size_t sciExp(value_type value, std::size_t base) {
		auto logAbs = logn(static_cast<value_type>(base), std::abs(value));

		if (value > 1 || value < -1) return implicitCast<std::size_t>(logAbs);
		else return implicitCast<std::size_t>(std::abs(logAbs)) + 1;
	}

	static string_type numberResult(
		value_type value,
		std::size_t numBase,
		std::size_t expBase,
		bool lowerCase,
		const format_spec& spec,
		FloatFormatMode formatMode
	) {
		// The number will generated written in reverse
		string_type result;

		// First, get the sign as a single character
		char_type sign = '\0';
		switch (spec.sign) {
			case '+':
				if (value < 0) sign = '-';
				else if (value >= 0) sign = '+';

				break;
			
			case '-':
				if (value < 0) sign = '-';
			
				break;
			
			case ' ':
				if (value < 0) sign = '-';
				else if (value >= 0) sign = ' ';
		}
		
		value = std::abs(value);

		std::conditional_t<std::is_same_v<char_type, char>, const char*, const wchar_t*> digits { };
		if constexpr (std::is_same_v<char_type, char>) digits = (lowerCase ? digitsLow : digitsUp);
		else digits = (lowerCase ? wDigitsLow : wDigitsUp);

		// Handle the number itself

		if (formatMode == FloatFormatMode::scientific) {
			// Write exponent part

			auto exp = 0;

			if (exp > 0) {
				while (exp > 0) {
					result.pushBack(digits[static_cast<std::size_t>(exp % numBase)]);
					exp /= numBase;
				}
			} else result.pushBack('0');

			if (value < 1) result.pushBack('-');
			else result.pushBack('+');

			if (numBase == 16) result.pushBack('p');
			else result.pushBack('e');


			// Handle the rest of the number

			auto numCount = spec.precision + 1;

			auto sci = static_cast<std::size_t>(std::fmod(value, 1) * std::pow(10, numCount));

			// Skip zeros
			for (; static_cast<std::size_t>(std::fmod(sci, numBase)) == 0; sci /= numBase, numCount--)
			;

			// Write fractional part
			for (; numCount > 1; numCount--) {
				result.pushBack(digits[static_cast<std::size_t>(std::fmod(sci, numBase))]);
				sci /= numBase;
			}

			// Write decimal point
			if (spec.alternateForm || !result.empty()) result.pushBack('.');

			// Write single integer
			if (value >= 1) result.pushBack(digits[static_cast<std::size_t>(sci)]);
			else result.pushBack('0');
		} else if (formatMode == FloatFormatMode::fixed) {
			FloatType integral = 0;
			FloatType fractional = std::modf(value, &integral);

			// Fractional part
			while (fractional > 0) {
				result.pushBack(digits[static_cast<std::size_t>(std::fmod(fractional, numBase))]);
				fractional /= numBase;
			}

			// Decimal point
			if (spec.alternateForm || !result.empty()) result.pushBack('.');

			// Whole part
			if (value >= 1) {
				while (value > 1) {
					result.pushBack(digits[static_cast<std::size_t>(std::fmod(integral, numBase))]);
					value /= numBase;
				}
			} else result.pushBack('0');
		} else {
			
		}

		if (sign != '\0') result.pushBack(sign);

		return result;
	}


	static bool writeToOutput(const string_type& result, back_inserter& inserter) {
		for (auto it = result.rbegin(); it != result.rend(); it++) {
			if (!inserter.done()) return false;
			inserter = *it;
		}

		return true;
	}
};

} // namespace detail


// Floating point formatters

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
