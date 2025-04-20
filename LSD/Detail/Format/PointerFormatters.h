/*************************
 * @file PointerFormatters.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation for pointer formatter classes
 * 
 * @date 2024-10-31
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

// Pointer formatter implementation

template <class CharTy> struct PointerFormatter {
	using value_type = std::uintptr_t;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using field_options = detail::BasicFieldOptions<char_type>;
	using format_spec = detail::BasicFormatSpec<value_type, char_type>;
	using context_type = BasicFormatContext<char_type>;

	using string_type = lsd::BasicString<char_type>;
	using view_type = lsd::BasicStringView<char_type>;

	static constexpr std::size_t pLength = sizeof(std::uintptr_t) * 2;
	static constexpr std::size_t length = pLength + 2;

	static void format(value_type value, context_type& context) {
		auto& inserter = context.out();
		format_spec spec(context);

		switch (spec.align) {
			case '<':
				if (!writeToOutput(value, inserter, spec)) return;

				for (auto count = spec.width; count > length; count--) 
					inserter = spec.fillChr;

				break;

			case '>':
				for (auto count = spec.width; count > length; count--) {
					if (inserter.done()) return;
					inserter = spec.fillChr;
				}

				writeToOutput(value, inserter, spec);

				break;

			case '^':
				if (spec.width > length) {
					auto fillc = spec.width - length;
					auto half = fillc / 2;
					
					for (auto count = fillc - half; count > 0; count--) {
						if (inserter.done()) return;
						inserter = spec.fillChr;
					}

					if (!writeToOutput(value, inserter, spec)) return;

					for (; !inserter.done() && half > 0; half--) inserter = spec.fillChr;
				} else writeToOutput(value, inserter, spec);
		}
	}

private:
	static bool writeToOutput(value_type value, back_inserter& inserter, const format_spec& spec) {
		if (spec.typeFormat.empty()) return outputNumberValue(value, inserter, true);
		else {
			switch (spec.typeFormat[0]) {
				case 'P':
					return outputNumberValue(value, inserter, false);

				case 'p':
					return outputNumberValue(value, inserter, true);
			}
		}

		return false;
	}

	static bool outputNumberValue(
		value_type value,
		back_inserter& inserter,
		bool lowerCase
	) {
		// First handle prefixes
		if (inserter.done()) return false;
		inserter = '0';

		if (inserter.done()) return false;
		inserter = (lowerCase ? 'x' : 'X');


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

		// Write padding zeros
		for (auto i = pLength; i > num.size(); i--) {
			if (inserter.done()) return false;
			inserter = '0';
		}

		// Write the number to the output
		for (auto it = num.rbegin(); it != num.rend(); it++) {
			if (inserter.done()) return false;
			inserter = *it;
		}

		return true;
	}
};

} // namespace detail


// Pointer formatters

template <class CharTy> struct Formatter<std::nullptr_t, CharTy> {
	void format(std::nullptr_t, BasicFormatContext<CharTy>& context) {
		detail::PointerFormatter<CharTy>::format(reinterpret_cast<std::uintptr_t>(nullptr), context);
	}
};
template <class CharTy> struct Formatter<void*, CharTy> {
	void format(void* value, BasicFormatContext<CharTy>& context) {
		detail::PointerFormatter<CharTy>::format(reinterpret_cast<std::uintptr_t>(value), context);
	}
};
template <class CharTy> struct Formatter<const void*, CharTy> {
	void format(const void* value, BasicFormatContext<CharTy>& context) {
		detail::PointerFormatter<CharTy>::format(reinterpret_cast<std::uintptr_t>(value), context);
	}
};

} // namespace lsd
