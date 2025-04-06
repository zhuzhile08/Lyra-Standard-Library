/*************************
 * @file StringFormatters.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation for string formatter classes
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

// String formatter implementation

template <class CharTy> struct StringFormatter {
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using field_options = detail::BasicFieldOptions<char_type>;
	using format_spec = detail::BasicFormatSpec<const char_type*, char_type>;
	using context_type = BasicFormatContext<char_type>;

	template <class StringCharTy> 
	static void format(StringCharTy* value, std::size_t length, context_type& context) {
		auto& inserter = context.out();
		format_spec spec(context);

		switch (spec.align) {
			case '<':
				if (!writeToOutput<StringCharTy>(value, length, inserter)) return;

				for (auto count = spec.width; !inserter.done() && count > length; count--) 
					inserter = spec.fillChr;

				break;

			case '>':
				for (auto count = spec.width; count > length; count--) {
					if (inserter.done()) return;
					inserter = spec.fillChr;
				}

				writeToOutput<StringCharTy>(value, length, inserter);

				break;

			case '^':
				if (spec.width > length) {
					auto fillc = spec.width - length;
					auto half = fillc / 2;
					
					for (auto count = fillc - half; count > 0; count--) {
						if (inserter.done()) return;
						inserter = spec.fillChr;
					}

					if (!writeToOutput<StringCharTy>(value, length, inserter)) return;

					for (; half > 0; half--) {
						if (inserter.done()) return;
						inserter = spec.fillChr;
					}
				} else writeToOutput<StringCharTy>(value, length, inserter);
		}
	}

private:
	template <class StringCharTy> static bool writeToOutput(
		StringCharTy* data,
		std::size_t length, 
		back_inserter& inserter) {
		for (std::size_t i = 0; i < length; i++) {
			if (inserter.done()) return false;
			inserter = data[i];
		}

		return true;
	}
};

} // namespace detail


// String formatters

template <class CharTy> struct Formatter<CharTy*, CharTy> {
	void format(CharTy* value, BasicFormatContext<CharTy>& context) {
		detail::StringFormatter<CharTy>::template format<CharTy>(value, stringLen(value), context);
	}
};
template <class CharTy> struct Formatter<const CharTy*, CharTy> {
	void format(const CharTy* value, BasicFormatContext<CharTy>& context) {
		detail::StringFormatter<CharTy>::template format<const CharTy>(value, stringLen(value), context);
	}
};

template <std::size_t Count, class CharTy> struct Formatter<CharTy[Count], CharTy> {
	void format(const CharTy (&value)[Count], BasicFormatContext<CharTy>& context) {
		detail::StringFormatter<CharTy>::template format<CharTy>(value, Count, context);
	}
};

template <class Traits, class Alloc, class CharTy> struct Formatter<BasicString<CharTy, Traits, Alloc>, CharTy> {
	void format(const BasicString<CharTy, Traits, Alloc>& value, BasicFormatContext<CharTy>& context) {
		detail::StringFormatter<CharTy>::template format<const CharTy>(value.data(), value.size(), context);
	}
};
template <class Traits, class CharTy> struct Formatter<BasicStringView<CharTy, Traits>, CharTy> {
	void format(BasicStringView<CharTy, Traits> value, BasicFormatContext<CharTy>& context) {
		detail::StringFormatter<CharTy>::template format<const CharTy>(value.data(), value.size(), context);
	}
};

} // namespace lsd
