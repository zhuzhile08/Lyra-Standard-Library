/*************************
 * @file PointerFormatters.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation for pointer formatter classes
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

// pointer formatter implementation

template <class CharTy> struct PointerFormatter {
	using value_type = std::uintptr_t;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using field_options = detail::BasicFieldOptions<char_type>;
	using format_spec = detail::BasicFormatSpec<char_type>;

	using string_type = lsd::BasicString<char_type>;
	using view_type = lsd::BasicStringView<char_type>;

	static constexpr std::size_t pLength = sizeof(std::uintptr_t) * 2;
	static constexpr std::size_t length = pLength + 2;

	static void format(value_type value, back_inserter& inserter, const field_options& options) {
		format_spec spec(options);

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
	// output to the iterator

	static void writeToOutput(value_type value, back_inserter& inserter, const format_spec& spec) {
		if (spec.typeFormat.empty()) outputNumberValue(value, inserter, true);
		else {
			switch (spec.typeFormat[0]) {
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

} // namespace detail


// pointer formatters

template <class CharTy> struct Formatter<std::nullptr_t, CharTy> {
	void format(
		std::nullptr_t,
		detail::BasicFormatBackInserter<CharTy>& inserter,
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::PointerFormatter<CharTy>::format(reinterpret_cast<std::uintptr_t>(nullptr), inserter, spec);
	}
};
template <class CharTy> struct Formatter<void*, CharTy> {
	void format(
		void* value,
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::PointerFormatter<CharTy>::format(reinterpret_cast<std::uintptr_t>(value), inserter, spec);
	}
};
template <class CharTy> struct Formatter<const void*, CharTy> {
	void format(
		const void* value,
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		detail::PointerFormatter<CharTy>::format(reinterpret_cast<std::uintptr_t>(value), inserter, spec);
	}
};

} // namespace lsd
