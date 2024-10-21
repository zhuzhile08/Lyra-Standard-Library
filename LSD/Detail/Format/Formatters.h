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

#include "../../String.h"
#include "../../StringView.h"

#include "FormatCore.h"
#include "FormatArgs.h"

#include <cstddef>

#include <format>

namespace lsd {

namespace detail {

// formatter implementations

template <std::integral IntegralType, class CharTy> struct IntegralFormatter {
public:
	using value_type = IntegralType;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<CharTy>;
	using field_options = detail::BasicFieldOptions<CharTy>;

	using string_type = lsd::BasicString<char_type>;

	static constexpr void format(value_type value, back_inserter& inserter, const field_options& options) {
		switch (options.align) {
			case '<':
				if (inserter.done()) return;
				writeToOutput(value, inserter, options);

				for (auto count = options.fillCount; !inserter.done() && count > 1; count--) inserter = options.fillChr;

				break;

			case '>':
				for (auto count = options.fillCount; !inserter.done() && count > 1; count--) inserter = options.fillChr;
				if (!inserter.done()) writeToOutput(value, inserter, options);

				break;

			case '^': {
				auto fillc = options.fillCount - outputLength(value, options);

				auto half = fillc / 2;
				for (auto count = fillc - half; !inserter.done() && count > 0; count--) inserter = options.fillChr;

				if (inserter.done()) return;
				writeToOutput(value, inserter, options);

				for (; !inserter.done() && half > 0; half--) inserter = options.fillChr;

				break;
			}
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
				case 'X:':
					return numLen(value, 16) + signLen(value, options.sign) + 2;

					break;
				
				case 'o':
					return numLen(value, 8) + signLen(value, options.sign) + 2;

					break;

				case 'd':
					return decNumLen(value) + signLen(value, options.sign);

					break;
				
				case 's':
					if constexpr (!std::same_as<value_type, bool>) return value ? 4 : 5;

					break;

				case 'c':
					if constexpr (!std::same_as<value_type, bool>) return 1;

					break;
			}
		}
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
	}

	// output to the iterator

	static constexpr void writeToOutput(value_type value, back_inserter& inserter, const field_options& options) {
		if (options.typeFormat.empty()) {
			if constexpr (std::same_as<value_type, bool>) outputBoolValue(value, inserter);
			else if constexpr (std::same_as<value_type, char> || std::same_as<value_type, wchar_t>) inserter = value;
			else {
				return decNumLen(value) + signLen(value, options.sign);
			}
		} else {
			switch (options.typeFormat[0]) {
				case 'b':


					break;

				case 'B':
					

					break;

				case 'x':


					break;

				case 'X:':
					

					break;
				
				case 'o':
					

					break;

				case 'd':
					

					break;
				
				case 's':
					if constexpr (!std::same_as<value_type, bool>) outputBoolValue(value, inserter);

					break;

				case 'c':
					if constexpr (!std::same_as<value_type, bool>) {
						if constexpr (std::same_as<value_type, char> || std::same_as<value_type, char>) inserter = value;
						else inserter = static_cast<char_type>(value);
					}

					break;
			}
		}
	}
	static constexpr void outputSign(value_type value, back_inserter& inserter, char_type sign) {
		switch (sign) {
			case '+':
				if (value < 0) inserter = '-';
				
		}
	}
	static constexpr void outputNumberValue() {
		outputSign();

	}
	static constexpr void outputBoolValue(bool value, back_inserter& inserter) {
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

template <class CharTy> struct StringFormatter {
	constexpr static void format(
		const char* data, 
		std::size_t length, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {

	}

	constexpr static void format(
		char* data, 
		std::size_t length, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {

	}
};

} // namespace detail


// character formatters

template <class CharTy> struct Formatter<char, CharTy> {
	constexpr void format(
		char c, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<char, CharTy>::format(c, inserter, options);
	}
};
template <> struct Formatter<wchar_t, wchar_t> {
	constexpr void format(wchar_t c, detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		detail::IntegralFormatter<wchar_t, wchar_t>::format(c, inserter, options);
	}
};


// bool formatter
template <class CharTy> struct Formatter<bool, CharTy> {
	constexpr void format(
		bool value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::IntegralFormatter<bool, CharTy>::format(value, inserter, options);
	}
};


// string formatters

template <class CharTy> struct Formatter<CharTy*, CharTy> {
	constexpr void format(
		CharTy* value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::format(value, std::strlen(value), inserter, options);
	}
};
template <class CharTy> struct Formatter<const CharTy*, CharTy> {
	constexpr void format(
		const CharTy* value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::format(value, std::strlen(value), inserter, options);
	}
};

template <std::size_t Count, class CharTy> struct Formatter<CharTy[Count], CharTy> {
	constexpr void format(
		const CharTy (&value)[Count], 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::format(value, Count, inserter, options);
	}
};

template <class Traits, class Alloc, class CharTy> struct Formatter<BasicString<CharTy, Traits, Alloc>, CharTy> {
	constexpr void format(
		const BasicString<CharTy, Traits, Alloc>& value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::format(value.data(), value.size(), inserter, options);
	}
};
template <class Traits, class CharTy> struct Formatter<BasicStringView<CharTy, Traits>, CharTy> {
	constexpr void format(
		BasicStringView<CharTy, Traits> value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::format(value.data(), value.size(), inserter, options);
	}
};


// arithmatic formatters

template <class CharTy> struct Formatter<short, CharTy> {
	constexpr void format(
		short value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<unsigned short, CharTy> {
	constexpr void format(
		unsigned short value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<int, CharTy> {
	constexpr void format(
		int value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<unsigned int, CharTy> {
	constexpr void format(
		unsigned int value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<long, CharTy> {
	constexpr void format(
		long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<unsigned long, CharTy> {
	constexpr void format(
		unsigned long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<long long, CharTy> {
	constexpr void format(
		long long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<unsigned long long, CharTy> {
	constexpr void format(
		unsigned long long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};

template <class CharTy> struct Formatter<float, CharTy> {
	constexpr void format(
		float value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<double, CharTy> {
	constexpr void format(
		double value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<long double, CharTy> {
	constexpr void format(
		long double value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};


// pointer formatters

template <class CharTy> struct Formatter<std::nullptr_t, CharTy> {
	constexpr void format(
		std::nullptr_t,
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<void*, CharTy> {
	constexpr void format(
		void* value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<const void*, CharTy> {
	constexpr void format(
		const void* value,
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};


// special type erased argument store formatters

template <class CharTy> struct Formatter<detail::TypeErasedFormatArg<CharTy, detail::BasicFormatBackInserter<CharTy>>, CharTy> {
	constexpr void format(
		const detail::TypeErasedFormatArg<CharTy, detail::FormatBackInserter>& fmtArg, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		fmtArg.format(inserter, options);
	}
};

} // namespace lsd
