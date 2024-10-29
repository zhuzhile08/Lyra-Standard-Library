/*************************
 * @file Formatters.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation for all formatter classes
 * 
 * @date 2024-08-20
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "../CoreUtility.h"
#include "../../String.h"
#include "../../StringView.h"

#include "FormatCore.h"
#include "FormatArgs.h"

#include <cstddef>

#include <format>

namespace lsd {

namespace detail {

// formatter implementations

namespace {

static constexpr auto digitsLow = "0123456789abcdefghijklmnopqrstuvpxyz";
static constexpr auto digitsUp = "0123456789ABCDEFGHIJKLMNOPQRSTUVPXYZ";

static constexpr auto wDigitsLow = L"0123456789abcdefghijklmnopqrstuvpxyz";
static constexpr auto wDigitsUp = L"0123456789ABCDEFGHIJKLMNOPQRSTUVPXYZ";

static constexpr auto infLow = "inf";
static constexpr auto infUp = "INF";

static constexpr auto nanLow = "nan";
static constexpr auto nanUp = "NAN";

enum class FloatFormatMode {
	regular,
	scientific,
	fixed
};

}

template <std::integral IntegralType, class CharTy> struct IntegralFormatter {
public:
	using value_type = IntegralType;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using field_options = detail::BasicFieldOptions<char_type>;

	using string_type = lsd::BasicString<char_type>;
	using view_type = lsd::BasicStringView<char_type>;

	static void format(value_type value, back_inserter& inserter, const field_options& options) {
		auto length = outputLength(value, options);

		switch (options.align) {
			case '<':
				if (inserter.done()) return;
				writeToOutput(value, inserter, options);

				for (auto count = options.fillCount; !inserter.done() && count > length; count--) 
					inserter = options.fillChr;

				break;

			case '>':
				for (auto count = options.fillCount; !inserter.done() && count > length; count--) 
						inserter = options.fillChr;

				if (!inserter.done()) writeToOutput(value, inserter, options);

				break;

			case '^':
				if (options.fillCount > length) {
					auto fillc = options.fillCount - length;
					auto half = fillc / 2;
					
					for (auto count = fillc - half; !inserter.done() && count > 0; count--) inserter = options.fillChr;

					if (inserter.done()) return;
					writeToOutput(value, inserter, options);

					for (; !inserter.done() && half > 0; half--) inserter = options.fillChr;
				} else {
					if (inserter.done()) return;
					writeToOutput(value, inserter, options);
				}

				break;
		}
	}

private:
	// output length calculation

	static constexpr std::size_t outputLength(value_type value, const field_options& options) {
		if (options.typeFormat.empty()) {
			if constexpr (std::same_as<value_type, bool>) return value ? 4 : 5;
			else if constexpr (std::same_as<value_type, char> || std::same_as<value_type, wchar_t>) return 1;
			else return decNumLen(value) + signLen(value, options.sign);
		} else {
			switch (options.typeFormat[0]) {
				case 'b':
				case 'B':
					return numLen(value, 2) + signLen(value, options.sign) + 2;

					break;

				case 'x':
				case 'X':
					return numLen(value, 16) + signLen(value, options.sign) + 2;

					break;
				
				case 'o':
					return numLen(value, 8) + signLen(value, options.sign) + 2;

					break;

				case 'd':
					return decNumLen(value) + signLen(value, options.sign);

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

	static void writeToOutput(value_type value, back_inserter& inserter, const field_options& options) {
		if (options.typeFormat.empty()) {
			if constexpr (std::same_as<value_type, bool>) outputBoolValue(value, inserter);
			else if constexpr (std::same_as<value_type, char> || std::same_as<value_type, wchar_t>) inserter = value;
			else outputNumberValue(value, inserter, 10, true, options.sign, false, { });
		} else {
			switch (options.typeFormat[0]) {
				case 'b':
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 2, true, options.sign, options.alternateForm, "0b");
					else
						outputNumberValue(value, inserter, 2, true, options.sign, options.alternateForm, L"0b");

					break;

				case 'B':
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 2, false, options.sign, options.alternateForm, "0B");
					else
						outputNumberValue(value, inserter, 2, false, options.sign, options.alternateForm, L"0B");

					break;

				case 'x':
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 16, true, options.sign, options.alternateForm, "0x");
					else
						outputNumberValue(value, inserter, 16, true, options.sign, options.alternateForm, L"0x");

					break;

				case 'X':
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 16, false, options.sign, options.alternateForm, "0X");
					else
						outputNumberValue(value, inserter, 16, false, options.sign, options.alternateForm, L"0X");

					break;

				case 'o':
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 8, true, options.sign, options.alternateForm, "0o");
					else
						outputNumberValue(value, inserter, 8, true, options.sign, options.alternateForm, L"0o");

					break;

				case 'O': // extended non standard capital octal formatting, doesn't have much use
					if constexpr (std::is_same_v<char_type, char>)
						outputNumberValue(value, inserter, 8, false, options.sign, options.alternateForm, "0O");
					else
						outputNumberValue(value, inserter, 8, false, options.sign, options.alternateForm, L"0O");

					break;

				case 'd':
					outputNumberValue(value, inserter, 10, true, options.sign, false, { });

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

template <std::floating_point FloatType, class CharTy> struct FloatFormatter {
public:
	using value_type = FloatType;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using field_options = detail::BasicFieldOptions<char_type>;

	using string_type = lsd::BasicString<char_type>;
	using view_type = lsd::BasicStringView<char_type>;

	static void format(value_type value, back_inserter& inserter, const field_options& options) {
		auto length = outputLength(value, options);

		switch (options.align) {
			case '<':
				if (inserter.done()) return;
				writeToOutput(value, inserter, options);

				for (auto count = options.fillCount; !inserter.done() && count > length; count--) 
					inserter = options.fillChr;

				break;

			case '>':
				for (auto count = options.fillCount; !inserter.done() && count > length; count--) 
						inserter = options.fillChr;

				if (!inserter.done()) writeToOutput(value, inserter, options);

				break;

			case '^':
				if (options.fillCount > length) {
					auto fillc = options.fillCount - length;
					auto half = fillc / 2;
					
					for (auto count = fillc - half; !inserter.done() && count > 0; count--) inserter = options.fillChr;

					if (inserter.done()) return;
					writeToOutput(value, inserter, options);

					for (; !inserter.done() && half > 0; half--) inserter = options.fillChr;
				} else {
					if (inserter.done()) return;
					writeToOutput(value, inserter, options);
				}

				break;
		}
	}

private:
	// output length calculation

	static constexpr std::size_t outputLength(value_type value, const field_options& options) {
		if (std::isinf(value) || std::isnan(value)) return 3;

		if (options.typeFormat.empty()) return signLen(value, options.sign) + numLen(value, 10) + (options.alternateForm ? (options.precision + 1) : fracLength<10>(value, options.precision, false));
		else {
			switch (options.typeFormat[0]) {
				case 'a':
				case 'A':
					return signLen(value, options.sign) + 3 + fracLength<16>(value, options.precision + 1, options.alternateForm) + sciExpLength<2>(value);

					break;

				case 'e':
				case 'E':
					return signLen(value, options.sign) + 3 + fracLength<10>(value, options.precision + 1, options.alternateForm) + sciExpLength<10>(value);

					break;
				
				case 'f':
				case 'F':
					return signLen(value, options.sign) + numLen(value, 10) + 1 + options.precision;

					break;

				case 'g':
				case 'G':
					return signLen(value, options.sign) + numLen(value, 10) + (options.alternateForm ? (options.precision + 1) : fracLength<10>(value, options.precision, false));

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
	template <std::size_t Base> static constexpr std::size_t sciExpLength(value_type value) {
		auto logAbs = logn<Base>(std::abs(value));

		if (value > 1) return implicitCast<std::size_t>(logAbs);
		else return std::floor(std::abs(logAbs)) + 1;
	}
	template <std::size_t Base> static constexpr std::size_t fracLength(value_type value, std::size_t precision, bool alternate) {
		value = std::abs(value);
		std::size_t l { };

		for (
			value_type diff { }; 
			diff > std::numeric_limits<value_type>::epsilon() && precision > 0;
			l++, value *= Base, precision--) {
			diff = value - std::floor(value);
		}

		if (l) return ++l;
		else if (alternate) return 1;
		else return 0;
	}

	// output to the iterator

	static void writeToOutput(value_type value, back_inserter& inserter, const field_options& options) {
		// special inf or nan output

		if (auto inf = std::isinf(value), nan = std::isnan(value); inf || nan) {
			bool up = false;

			if (!options.typeFormat.empty()) {
				auto c = options.typeFormat[0];
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

		if (options.typeFormat.empty()) outputNumberValue(value, inserter, 10, 10, false, options, FloatFormatMode::regular);
		else {
			switch (options.typeFormat[0]) {
				case 'a':
					outputNumberValue(value, inserter, 16, 2, true, options, FloatFormatMode::scientific);

					break;

				case 'A':
					outputNumberValue(value, inserter, 16, 2, false, options, FloatFormatMode::scientific);

					break;

				case 'e':
					outputNumberValue(value, inserter, 10, 10, true, options, FloatFormatMode::scientific);

					break;

				case 'E':
					outputNumberValue(value, inserter, 10, 10, false, options, FloatFormatMode::scientific);

					break;
				
				case 'f':
					outputNumberValue(value, inserter, 10, 10, true, options, FloatFormatMode::fixed);

					break;

				case 'F':
					outputNumberValue(value, inserter, 10, 10, false, options, FloatFormatMode::fixed);

					break;

				case 'g':
					outputNumberValue(value, inserter, 10, 10, true, options, FloatFormatMode::regular);

					break;

				case 'G':
					outputNumberValue(value, inserter, 10, 10, false, options, FloatFormatMode::regular);

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
		const field_options& options,
		FloatFormatMode formatMode
	) {
		// first handle prefixes

		switch (options.sign) {
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

		if (formatMode == FloatFormatMode::regular) {
			if (value >= 1) {
				while (value > 1) {
					num.pushBack(digits[static_cast<std::size_t>(std::fmod(value, numBase))]);
					value /= numBase;
				}
			} else num.pushBack('0');
				
			if (options.alternateForm || value > std::numeric_limits<value_type>::epsilon()) num.pushBack('.');

			auto remaining = options.precision;

			for (; value > std::numeric_limits<value_type>::epsilon() && remaining > 0; remaining--) {

			}

			if (options.alternateForm) for (; remaining > 0; remaining--) num.pushBack('0');
		} else if (formatMode == FloatFormatMode::fixed) {

		} else {

		}

		// write the number to the output
		for (auto it = num.rbegin(); it != num.rend() && !inserter.done(); it++) inserter = *it;

		/*
		if (value) {
			while (value) {
				num.pushBack(digits[value % base]);
				value /= base;
			}
		}
		*/
	}
};

template <class CharTy> struct PointerFormatter {
	using value_type = std::uintptr_t;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using field_options = detail::BasicFieldOptions<char_type>;

	using string_type = lsd::BasicString<char_type>;
	using view_type = lsd::BasicStringView<char_type>;

	static constexpr std::size_t pLength = sizeof(std::uintptr_t) * 2;
	static constexpr std::size_t length = pLength + 2;

	static void format(value_type value, back_inserter& inserter, const field_options& options) {
		switch (options.align) {
			case '<':
				if (inserter.done()) return;
				writeToOutput(value, inserter, options);

				for (auto count = options.fillCount; !inserter.done() && count > length; count--) 
					inserter = options.fillChr;

				break;

			case '>':
				for (auto count = options.fillCount; !inserter.done() && count > length; count--) 
						inserter = options.fillChr;

				if (!inserter.done()) writeToOutput(value, inserter, options);

				break;

			case '^':
				if (options.fillCount > length) {
					auto fillc = options.fillCount - length;
					auto half = fillc / 2;
					
					for (auto count = fillc - half; !inserter.done() && count > 0; count--) inserter = options.fillChr;

					if (inserter.done()) return;
					writeToOutput(value, inserter, options);

					for (; !inserter.done() && half > 0; half--) inserter = options.fillChr;
				} else {
					if (inserter.done()) return;
					writeToOutput(value, inserter, options);
				}

				break;
		}
	}

private:
	// output to the iterator

	static void writeToOutput(value_type value, back_inserter& inserter, const field_options& options) {
		if (options.typeFormat.empty()) outputNumberValue(value, inserter, true);
		else {
			switch (options.typeFormat[0]) {
				case 'P':
					outputNumberValue(value, inserter, false);

					break;

				case 'p':
					outputNumberValue(value, inserter, true);

					break;
			}
		}
	}
	static void outputNumberValue(
		value_type value,
		back_inserter& inserter,
		bool lowerCase
	) {
		// first handle prefixes
		if (!inserter.done()) {
			inserter = '0'; // techinically unsafe, but out of bounds won't happen no matter what

			if (!inserter.done()) inserter = (lowerCase ? 'x' : 'X');
			else return;

			if (inserter.done()) return;
		} else return;


		// now handle the number itself

		if (value < 0) value *= -1;

		string_type num;

		if (value) {
			std::conditional_t<std::is_same_v<char_type, char>, const char*, const wchar_t*> digits { };
			if constexpr (std::is_same_v<char_type, char>) digits = (lowerCase ? digitsLow : digitsUp);
			else digits = (lowerCase ? wDigitsLow : wDigitsUp);

			while (value) {
				num.pushBack(digits[value % 16]);
				value /= 16;
			}
		}

		// write padding zeros
		for (auto i = pLength; i > num.size(); i--) inserter = '0';

		// write the number to the output
		for (auto it = num.rbegin(); it != num.rend() && !inserter.done(); it++) inserter = *it;
	}
};

template <class CharTy> struct StringFormatter {
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using field_options = detail::BasicFieldOptions<char_type>;

	template <class StringCharTy> static void format(
		StringCharTy* value, 
		std::size_t length, 
		back_inserter& inserter, 
		const field_options& options) {
		switch (options.align) {
			case '<':
				if (inserter.done()) return;
				writeToOutput<StringCharTy>(value, length, inserter, options.typeFormat.empty() ? '\0' : options.typeFormat[0]);

				for (auto count = options.fillCount; !inserter.done() && count > length; count--) 
					inserter = options.fillChr;

				break;

			case '>':
				for (auto count = options.fillCount; !inserter.done() && count > length; count--) 
					inserter = options.fillChr;

				if (!inserter.done()) writeToOutput<StringCharTy>(value, length, inserter, options.typeFormat.empty() ? '\0' : options.typeFormat[0]);

				break;

			case '^':
				if (options.fillCount > length) {
					auto fillc = options.fillCount - length;
					auto half = fillc / 2;
					
					for (auto count = fillc - half; !inserter.done() && count > 0; count--) inserter = options.fillChr;

					if (inserter.done()) return;
					writeToOutput<StringCharTy>(value, length, inserter, options.typeFormat.empty() ? '\0' : options.typeFormat[0]);

					for (; !inserter.done() && half > 0; half--) inserter = options.fillChr;
				} else {
					if (inserter.done()) return;
					writeToOutput<StringCharTy>(value, length, inserter, options.typeFormat.empty() ? '\0' : options.typeFormat[0]);
				}

				break;
		}
	}

private:
	template <class StringCharTy> static void writeToOutput(
		StringCharTy* data,
		std::size_t length, 
		back_inserter& inserter, 
		char_type typeOptions) {
		switch (typeOptions) {
			case '?':
				for (std::size_t i = 0; !inserter.done() && i < length; i++) {
					auto c = data[i];

					switch (c) {
						case '\t':
							inserter = '\\';
							if (inserter.done()) return;
							inserter = 't';

							break;

						case '\n':
							inserter = '\\';
							if (inserter.done()) return;
							inserter = 'n';

							break;

						case '\r':
							inserter = '\\';
							if (inserter.done()) return;
							inserter = 'r';

							break;

						case '\"':
							inserter = '\\';
							if (inserter.done()) return;
							inserter = '\"';

							break;

						case '\'':
							inserter = '\\';
							if (inserter.done()) return;
							inserter = '\'';

							break;

						case '\\':
							inserter = '\\';
							if (inserter.done()) return;
							inserter = '\\';

							break;

						default:
							inserter = c;

							break;
					}
				}

				break;

			default:
				for (std::size_t i = 0; !inserter.done() && i < length; i++) inserter = data[i];

				break;
		}
	}
};

} // namespace detail


// character formatters

template <class CharTy> struct Formatter<char, CharTy> {
	void format(
		char c, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<char, CharTy>::format(c, inserter, options);
	}
};
template <> struct Formatter<wchar_t, wchar_t> {
	void format(wchar_t c, detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		detail::IntegralFormatter<wchar_t, wchar_t>::format(c, inserter, options);
	}
};


// bool formatter
template <class CharTy> struct Formatter<bool, CharTy> {
	void format(
		bool value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<bool, CharTy>::format(value, inserter, options);
	}
};


// string formatters

template <class CharTy> struct Formatter<CharTy*, CharTy> {
	void format(
		CharTy* value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::template format<CharTy>(value, std::strlen(value), inserter, options);
	}
};
template <class CharTy> struct Formatter<const CharTy*, CharTy> {
	void format(
		const CharTy* value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::template format<const CharTy>(value, std::strlen(value), inserter, options);
	}
};

template <std::size_t Count, class CharTy> struct Formatter<CharTy[Count], CharTy> {
	void format(
		const CharTy (&value)[Count], 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::template format<CharTy>(value, Count, inserter, options);
	}
};

template <class Traits, class Alloc, class CharTy> struct Formatter<BasicString<CharTy, Traits, Alloc>, CharTy> {
	void format(
		const BasicString<CharTy, Traits, Alloc>& value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::template format<const CharTy>(value.data(), value.size(), inserter, options);
	}
};
template <class Traits, class CharTy> struct Formatter<BasicStringView<CharTy, Traits>, CharTy> {
	void format(
		BasicStringView<CharTy, Traits> value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::template format<const CharTy>(value.data(), value.size(), inserter, options);
	}
};


// arithmatic formatters

template <class CharTy> struct Formatter<short, CharTy> {
	void format(
		short value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<short, CharTy>::format(value, inserter, options);
	}
};
template <class CharTy> struct Formatter<unsigned short, CharTy> {
	void format(
		unsigned short value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<unsigned short, CharTy>::format(value, inserter, options);
	}
};
template <class CharTy> struct Formatter<int, CharTy> {
	void format(
		int value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<int, CharTy>::format(value, inserter, options);
	}
};
template <class CharTy> struct Formatter<unsigned int, CharTy> {
	void format(
		unsigned int value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<unsigned int, CharTy>::format(value, inserter, options);
	}
};
template <class CharTy> struct Formatter<long, CharTy> {
	void format(
		long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<long, CharTy>::format(value, inserter, options);
	}
};
template <class CharTy> struct Formatter<unsigned long, CharTy> {
	void format(
		unsigned long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<unsigned long, CharTy>::format(value, inserter, options);
	}
};
template <class CharTy> struct Formatter<long long, CharTy> {
	void format(
		long long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<long long, CharTy>::format(value, inserter, options);
	}
};
template <class CharTy> struct Formatter<unsigned long long, CharTy> {
	void format(
		unsigned long long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<unsigned long long, CharTy>::format(value, inserter, options);
	}
};

template <class CharTy> struct Formatter<float, CharTy> {
	void format(
		float value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::FloatFormatter<float, CharTy>::format(value, inserter, options);
	}
};
template <class CharTy> struct Formatter<double, CharTy> {	
	void format(
		double value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::FloatFormatter<double, CharTy>::format(value, inserter, options);
	}
};
template <class CharTy> struct Formatter<long double, CharTy> {
	void format(
		long double value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::FloatFormatter<long double, CharTy>::format(value, inserter, options);
	}
};


// pointer formatters

template <class CharTy> struct Formatter<std::nullptr_t, CharTy> {
	void format(
		std::nullptr_t,
		detail::BasicFormatBackInserter<CharTy>& inserter,
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::PointerFormatter<CharTy>::format(reinterpret_cast<std::uintptr_t>(nullptr), inserter, options);
	}
};
template <class CharTy> struct Formatter<void*, CharTy> {
	void format(
		void* value,
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::PointerFormatter<CharTy>::format(reinterpret_cast<std::uintptr_t>(value), inserter, options);
	}
};
template <class CharTy> struct Formatter<const void*, CharTy> {
	void format(
		const void* value,
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::PointerFormatter<CharTy>::format(reinterpret_cast<std::uintptr_t>(value), inserter, options);
	}
};


// special type erased argument store formatters

template <class CharTy> struct Formatter<detail::TypeErasedFormatArg<CharTy, detail::BasicFormatBackInserter<CharTy>>, CharTy> {
	void format(
		const detail::TypeErasedFormatArg<CharTy, detail::FormatBackInserter>& fmtArg, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		fmtArg.format(inserter, options);
	}
};

} // namespace lsd
