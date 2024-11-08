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

// string formatter implementation

template <class CharTy> struct StringFormatter {
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using field_options = detail::BasicFieldOptions<char_type>;
	using format_spec = detail::BasicGeneralFormatSpec<char_type>;
	using context_type = BasicFormatContext<char_type>;

	template <class StringCharTy> 
	static void format(StringCharTy* value, std::size_t length, context_type& context) {
		auto& inserter = context.out();
		format_spec spec(context);

		switch (spec.align) {
			case '<':
				if (inserter.done()) return;
				writeToOutput<StringCharTy>(value, length, inserter, spec.typeFormat.empty() ? '\0' : spec.typeFormat[0]);

				for (auto count = spec.width; !inserter.done() && count > length; count--) 
					inserter = spec.fillChr;

				break;

			case '>':
				for (auto count = spec.width; !inserter.done() && count > length; count--) 
					inserter = spec.fillChr;

				if (!inserter.done()) writeToOutput<StringCharTy>(value, length, inserter, spec.typeFormat.empty() ? '\0' : spec.typeFormat[0]);

				break;

			case '^':
				if (spec.width > length) {
					auto fillc = spec.width - length;
					auto half = fillc / 2;
					
					for (auto count = fillc - half; !inserter.done() && count > 0; count--) inserter = spec.fillChr;

					if (inserter.done()) return;
					writeToOutput<StringCharTy>(value, length, inserter, spec.typeFormat.empty() ? '\0' : spec.typeFormat[0]);

					for (; !inserter.done() && half > 0; half--) inserter = spec.fillChr;
				} else {
					if (inserter.done()) return;
					writeToOutput<StringCharTy>(value, length, inserter, spec.typeFormat.empty() ? '\0' : spec.typeFormat[0]);
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


// string formatters

template <class CharTy> struct Formatter<CharTy*, CharTy> {
	void format(CharTy* value, BasicFormatContext<CharTy>& context) {
		detail::StringFormatter<CharTy>::template format<CharTy>(value, std::strlen(value), context);
	}
};
template <class CharTy> struct Formatter<const CharTy*, CharTy> {
	void format(const CharTy* value, BasicFormatContext<CharTy>& context) {
		detail::StringFormatter<CharTy>::template format<const CharTy>(value, std::strlen(value), context);
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
