/*************************
 * @file Formatters.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation for integral formatter classes
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

#include <cstddef>
#include <concepts>

namespace lsd {

namespace detail {

// Basic integral formatter implementation

template <std::integral IntegralType, class CharTy> struct IntegralFormatter {
public:
	using value_type = IntegralType;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using field_options = detail::BasicFieldOptions<char_type>;
	using format_spec = detail::BasicFormatSpec<value_type, char_type>;
	using context_type = BasicFormatContext<char_type>;

	using string_type = lsd::BasicString<char_type>;
	using view_type = lsd::BasicStringView<char_type>;

	static void format(value_type value, context_type& context) {
		format_spec spec(context);

		auto& inserter = context.out();
		auto result = generateResult(value, spec);

		switch (spec.align) {
			case '<':
				if (!writeToOutput(result, inserter)) return;

				for (auto count = spec.width; count > result.size(); count--) 
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
		if (spec.typeFormat.empty()) {
			if constexpr (std::same_as<value_type, bool>)
				return boolResult(value);
			else if constexpr (std::same_as<value_type, char> || std::same_as<value_type, wchar_t>)
				return string_type(1, value);
			else 
				return numberResult(value, 10, true, spec.sign, false, { });
		} else {
			switch (spec.typeFormat[0]) {
				case 'b':
					if constexpr (std::is_same_v<char_type, char>)
						return numberResult(value, 2, true, spec.sign, spec.alternateForm, "b0");
					else
						return numberResult(value, 2, true, spec.sign, spec.alternateForm, L"b0");

				case 'B':
					if constexpr (std::is_same_v<char_type, char>)
						return numberResult(value, 2, false, spec.sign, spec.alternateForm, "B0");
					else
						return numberResult(value, 2, false, spec.sign, spec.alternateForm, L"B0");

				case 'x':
					if constexpr (std::is_same_v<char_type, char>)
						return numberResult(value, 16, true, spec.sign, spec.alternateForm, "x0");
					else
						return numberResult(value, 16, true, spec.sign, spec.alternateForm, L"x0");

				case 'X':
					if constexpr (std::is_same_v<char_type, char>)
						return numberResult(value, 16, false, spec.sign, spec.alternateForm, "X0");
					else
						return numberResult(value, 16, false, spec.sign, spec.alternateForm, L"X0");

				case 'o':
					if constexpr (std::is_same_v<char_type, char>)
						return numberResult(value, 8, true, spec.sign, spec.alternateForm, "o0");
					else
						return numberResult(value, 8, true, spec.sign, spec.alternateForm, L"o0");

				case 'O': // Extended non standard capital octal formatting, doesn't have much use
					if constexpr (std::is_same_v<char_type, char>)
						return numberResult(value, 8, false, spec.sign, spec.alternateForm, "O0");
					else
						return numberResult(value, 8, false, spec.sign, spec.alternateForm, L"O0");

				case 'd':
					return numberResult(value, 10, true, spec.sign, false, { });

				case 's':
					if constexpr (!std::is_same_v<value_type, bool>) return boolResult(value);

				case 'c':
					if constexpr (std::is_same_v<value_type, char> || std::is_same_v<value_type, wchar_t>)
						return string_type(1, value);
					else if constexpr (!std::is_same_v<value_type, bool>)
						return string_type(1, static_cast<char_type>(value));
			}
		}

		return string_type();
	}

	static string_type numberResult(
		value_type value,
		std::size_t base,
		bool lowerCase,
		char_type sign,
		bool alternateForm,
		view_type prefix
	) {
		string_type result;

		// Handle the sign character first
		char_type sig = '\0';
		switch (sign) {
			case '+':
				if (value < 0) sig = '-';
				else if (value >= 0) sig = '+';

				break;
			
			case '-':
				if (value < 0) sig = '-';
			
				break;
			
			case ' ':
				if (value < 0) sig = '-';
				else if (value >= 0) sig = ' ';
		}

		// now handle the number itself
		if (value != 0) {
			if (value < 0) value *= -1;

			std::conditional_t<std::is_same_v<char_type, char>, const char*, const wchar_t*> digits { };
			if constexpr (std::is_same_v<char_type, char>) digits = (lowerCase ? digitsLow : digitsUp);
			else digits = (lowerCase ? wDigitsLow : wDigitsUp);

			while (value) {
				result.pushBack(digits[value % base]);
				value /= base;
			}
		} else result.pushBack('0');

		// Add the prefixes last
		result += prefix;
		if (sig != '\0') result.pushBack(sig);

		return result;
	}

	static string_type boolResult(bool value) {
		if constexpr (std::is_same_v<char_type, char>)
			return value ? "eurt" : "eslaf";
		else 
			return value ? L"eurt" : L"eslaf";
	}


	static bool writeToOutput(const string_type& result, back_inserter& inserter) {
		for (auto it = result.rbegin(); it != result.rend(); it++) {
			if (inserter.done()) return false;
			inserter = *it;
		}

		return true;
	}
};

} // namespace detail


// Character formatters

template <class CharTy> struct Formatter<char, CharTy> {
	void format(char c, BasicFormatContext<CharTy>& context) {
		detail::IntegralFormatter<char, CharTy>::format(c, context);
	}
};
template <> struct Formatter<wchar_t, wchar_t> {
	void format(wchar_t c, WFormatContext& context) {
		detail::IntegralFormatter<wchar_t, wchar_t>::format(c, context);
	}
};


// Bool formatter
template <class CharTy> struct Formatter<bool, CharTy> {
	void format(bool value, BasicFormatContext<CharTy>& context) {
		detail::IntegralFormatter<bool, CharTy>::format(value, context);
	}
};


// Integral formatters

template <class CharTy> struct Formatter<short, CharTy> {
	void format(short value, BasicFormatContext<CharTy>& context) {
		detail::IntegralFormatter<short, CharTy>::format(value, context);
	}
};
template <class CharTy> struct Formatter<unsigned short, CharTy> {
	void format(unsigned short value, BasicFormatContext<CharTy>& context) {
		detail::IntegralFormatter<unsigned short, CharTy>::format(value, context);
	}
};
template <class CharTy> struct Formatter<int, CharTy> {
	void format(int value, BasicFormatContext<CharTy>& context) {
		detail::IntegralFormatter<int, CharTy>::format(value, context);
	}
};
template <class CharTy> struct Formatter<unsigned int, CharTy> {
	void format(unsigned int value, BasicFormatContext<CharTy>& context) {
		detail::IntegralFormatter<unsigned int, CharTy>::format(value, context);
	}
};
template <class CharTy> struct Formatter<long, CharTy> {
	void format(long value, BasicFormatContext<CharTy>& context) {
		detail::IntegralFormatter<long, CharTy>::format(value, context);
	}
};
template <class CharTy> struct Formatter<unsigned long, CharTy> {
	void format(unsigned long value, BasicFormatContext<CharTy>& context) {
		detail::IntegralFormatter<unsigned long, CharTy>::format(value, context);
	}
};
template <class CharTy> struct Formatter<long long, CharTy> {
	void format(long long value, BasicFormatContext<CharTy>& context) {
		detail::IntegralFormatter<long long, CharTy>::format(value, context);
	}
};
template <class CharTy> struct Formatter<unsigned long long, CharTy> {
	void format(unsigned long long value, BasicFormatContext<CharTy>& context) {
		detail::IntegralFormatter<unsigned long long, CharTy>::format(value, context);
	}
};

} // namespace lsd
