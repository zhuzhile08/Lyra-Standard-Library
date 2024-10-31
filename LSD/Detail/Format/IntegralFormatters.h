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

#include "../CoreUtility.h"
#include "../../String.h"
#include "../../StringView.h"

#include "FormatCore.h"

#include <cstddef>
#include <concepts>

namespace lsd {

namespace detail {

// basic integral formatter implementation

template <std::integral IntegralType, class CharTy> struct IntegralFormatter {
public:
	using value_type = IntegralType;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using field_options = detail::BasicFieldOptions<char_type>;
	using format_spec = detail::BasicFormatSpec<char_type>;

	using string_type = lsd::BasicString<char_type>;
	using view_type = lsd::BasicStringView<char_type>;

	static void format(value_type value, back_inserter& inserter, const field_options& options) {
		format_spec spec(options);
		auto length = outputLength(value, spec);

		switch (spec.align) {
			case '<':
				if (inserter.done()) return;
				writeToOutput(value, inserter, spec);

				for (auto count = spec.fillCount; !inserter.done() && count > length; count--) 
					inserter = spec.fillChr;

				break;

			case '>':
				for (auto count = spec.fillCount; !inserter.done() && count > length; count--) 
						inserter = spec.fillChr;

				if (!inserter.done()) writeToOutput(value, inserter, spec);

				break;

			case '^':
				if (spec.fillCount > length) {
					auto fillc = spec.fillCount - length;
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
	// output length calculation

	static constexpr std::size_t outputLength(value_type value, const format_spec& spec) {
		if (spec.typeFormat.empty()) {
			if constexpr (std::same_as<value_type, bool>) return value ? 4 : 5;
			else if constexpr (std::same_as<value_type, char> || std::same_as<value_type, wchar_t>) return 1;
			else return decNumLen(value) + signLen(value, spec.sign);
		} else {
			switch (spec.typeFormat[0]) {
				case 'b':
				case 'B':
					return numLen(value, 2) + signLen(value, spec.sign) + 2;

					break;

				case 'x':
				case 'X':
					return numLen(value, 16) + signLen(value, spec.sign) + 2;

					break;
				
				case 'o':
					return numLen(value, 8) + signLen(value, spec.sign) + 2;

					break;

				case 'd':
					return decNumLen(value) + signLen(value, spec.sign);

					break;
				
				case 's':
					if constexpr (std::same_as<value_type, bool>) return value ? 4 : 5;

					break;

				case 'c':
					if constexpr (!std::same_as<value_type, bool>) return 1;

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

	// output to the iterator

	static void writeToOutput(value_type value, back_inserter& inserter, const format_spec& spec) {
		if (spec.typeFormat.empty()) {
			if constexpr (std::same_as<value_type, bool>) outputBoolValue(value, inserter);
			else if constexpr (std::same_as<value_type, char> || std::same_as<value_type, wchar_t>) inserter = value;
			else outputNumberValue(value, inserter, 10, true, spec.sign, false, { });
		} else {
			switch (spec.typeFormat[0]) {
				case 'b':
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 2, true, spec.sign, spec.alternateForm, "0b");
					else
						outputNumberValue(value, inserter, 2, true, spec.sign, spec.alternateForm, L"0b");

					break;

				case 'B':
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 2, false, spec.sign, spec.alternateForm, "0B");
					else
						outputNumberValue(value, inserter, 2, false, spec.sign, spec.alternateForm, L"0B");

					break;

				case 'x':
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 16, true, spec.sign, spec.alternateForm, "0x");
					else
						outputNumberValue(value, inserter, 16, true, spec.sign, spec.alternateForm, L"0x");

					break;

				case 'X':
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 16, false, spec.sign, spec.alternateForm, "0X");
					else
						outputNumberValue(value, inserter, 16, false, spec.sign, spec.alternateForm, L"0X");

					break;

				case 'o':
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 8, true, spec.sign, spec.alternateForm, "0o");
					else
						outputNumberValue(value, inserter, 8, true, spec.sign, spec.alternateForm, L"0o");

					break;

				case 'O': // extended non standard capital octal formatting, doesn't have much use
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 8, false, spec.sign, spec.alternateForm, "0O");
					else
						outputNumberValue(value, inserter, 8, false, spec.sign, spec.alternateForm, L"0O");

					break;

				case 'd':
					outputNumberValue(value, inserter, 10, true, spec.sign, false, { });

					break;

				case 's':
					if constexpr (!std::is_same_v<value_type, bool>) outputBoolValue(value, inserter);

					break;

				case 'c':
					if constexpr (std::is_same_v<value_type, char> || std::is_same_v<value_type, wchar_t>) inserter = value;
					else if constexpr (!std::is_same_v<value_type, bool>) inserter = static_cast<char_type>(value);

					break;
			}
		}
	}
	static void outputNumberValue(
		value_type value,
		back_inserter& inserter,
		std::size_t base,
		bool lowerCase,
		char_type sign,
		bool alternateForm,
		view_type prefix
	) {
		// first handle prefixes

		switch (sign) {
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

		if (!inserter.done()) {
			if (alternateForm) {
				inserter = prefix[0]; // techinically unsafe, but out of bounds won't happen no matter what

				if (!inserter.done()) inserter = prefix[1];
				else return;

				if (inserter.done()) return;
			}
		} else return;


		// now handle the number itself

		string_type num;

		if (value) {
			if (value < 0) value *= -1;

			std::conditional_t<std::is_same_v<char_type, char>, const char*, const wchar_t*> digits { };
			if constexpr (std::is_same_v<char_type, char>) digits = (lowerCase ? digitsLow : digitsUp);
			else digits = (lowerCase ? wDigitsLow : wDigitsUp);

			while (value) {
				num.pushBack(digits[value % base]);
				value /= base;
			}
		} else num.pushBack('0');

		// write the number to the output
		for (auto it = num.rbegin(); it != num.rend() && !inserter.done(); it++) inserter = *it;
	}
	static void outputBoolValue(bool value, back_inserter& inserter) {
		// no first done check, since it was already checked in the format function

		if (value) {
			inserter = 't';
			if (inserter.done()) return;
			inserter = 'r';
			if (inserter.done()) return;
			inserter = 'u';
			if (inserter.done()) return;
			inserter = 'e';
		} else {
			inserter = 'f';
			if (!inserter.done()) return;
			inserter = 'a';
			if (!inserter.done()) return;
			inserter = 'l';
			if (!inserter.done()) return;
			inserter = 's';
			if (!inserter.done()) return;
			inserter = 'e';
		}
	}
};

} // namespace detail


// character formatters

template <class CharTy> struct Formatter<char, CharTy> {
	void format(
		char c, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::IntegralFormatter<char, CharTy>::format(c, inserter, spec);
	}
};
template <> struct Formatter<wchar_t, wchar_t> {
	void format(wchar_t c, detail::WFormatBackInserter& inserter, const detail::WFieldOptions& spec) {
		detail::IntegralFormatter<wchar_t, wchar_t>::format(c, inserter, spec);
	}
};


// bool formatter
template <class CharTy> struct Formatter<bool, CharTy> {
	void format(
		bool value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::IntegralFormatter<bool, CharTy>::format(value, inserter, spec);
	}
};


// integral formatters

template <class CharTy> struct Formatter<short, CharTy> {
	void format(
		short value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::IntegralFormatter<short, CharTy>::format(value, inserter, spec);
	}
};
template <class CharTy> struct Formatter<unsigned short, CharTy> {
	void format(
		unsigned short value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::IntegralFormatter<unsigned short, CharTy>::format(value, inserter, spec);
	}
};
template <class CharTy> struct Formatter<int, CharTy> {
	void format(
		int value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::IntegralFormatter<int, CharTy>::format(value, inserter, spec);
	}
};
template <class CharTy> struct Formatter<unsigned int, CharTy> {
	void format(
		unsigned int value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::IntegralFormatter<unsigned int, CharTy>::format(value, inserter, spec);
	}
};
template <class CharTy> struct Formatter<long, CharTy> {
	void format(
		long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::IntegralFormatter<long, CharTy>::format(value, inserter, spec);
	}
};
template <class CharTy> struct Formatter<unsigned long, CharTy> {
	void format(
		unsigned long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::IntegralFormatter<unsigned long, CharTy>::format(value, inserter, spec);
	}
};
template <class CharTy> struct Formatter<long long, CharTy> {
	void format(
		long long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::IntegralFormatter<long long, CharTy>::format(value, inserter, spec);
	}
};
template <class CharTy> struct Formatter<unsigned long long, CharTy> {
	void format(
		unsigned long long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::IntegralFormatter<unsigned long long, CharTy>::format(value, inserter, spec);
	}
};

} // namespace lsd
