/*************************
 * @file Format.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Formatting utility and library
 * @brief Formatting specification: <"{" <index>: "}">
 * 
 * @date 2024-06-18
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Iterators.h"
#include "String.h"
#include "StringView.h"
#include "UniquePointer.h"
#include "FunctionPointer.h"
#include "Utility.h"

#include <cctype>
#include <type_traits>
#include <concepts>

namespace lsd {

template <class Ty, class CharTy = char> struct Formatter;


class FormatError : public std::runtime_error {
public:
	FormatError(const lsd::String& message) : std::runtime_error(message.cStr()) {
		m_message.append(message).pushBack('!');
	}
	FormatError(const char* message) : std::runtime_error(message) {
		m_message.append(message).pushBack('!');
	}
	FormatError(const FormatError&) = default;
	FormatError(FormatError&&) = default;

	FormatError& operator=(const FormatError&) = default;
	FormatError& operator=(FormatError&&) = default;

	const char* what() const noexcept override {
		return m_message.cStr();
	}

private:
	lsd::String m_message { "Program terminated with lyra::FormatError: " }; // disregards the standard requirement that the formatting error has to equal the message
};


namespace detail {

template <class CharTy> class BasicFormatBackInserter {
public:
	static_assert(std::is_same_v<CharTy, char> || std::is_same_v<CharTy, wchar_t>, "lsd::detail::BasicFormatBackInserter: Format back insert iterator only accepts char and wchar_t as valid types for template argument CharTy!");

	using char_type = CharTy;
	using container = BasicFormatBackInserter;

	template <class PushBack, class Done> constexpr BasicFormatBackInserter(void* const container, PushBack&& pb, Done&& d) : 
		m_container(container), 
		m_pushBack(std::forward<PushBack>(pb)), 
		m_done(std::forward<Done>(d)) { }
	
	constexpr container& operator=(char_type value) {
		m_pushBack(m_container, value); // do not check for doneness since the formatter and context does that
		return *this;
	}

	constexpr container& operator*() { return *this; }
	constexpr container& operator++() { return *this; }
	constexpr container& operator++(int) { return *this; }

	constexpr bool done() {
		return m_done(m_container);
	}

private:
	void* const m_container;
	Function<void(void*, const char_type&)> m_pushBack;
	Function<bool(void*)> m_done;
};

using FormatBackInserter = BasicFormatBackInserter<char>;
using WFormatBackInserter = BasicFormatBackInserter<wchar_t>;


template <class CharTy> struct BasicRuntimeFormatString {
public:
	static_assert(std::is_same_v<CharTy, char> || std::is_same_v<CharTy, wchar_t>, "lsd::BasicRuntimeFormatString: Runtime format string only accepts char and wchar_t as valid types for template argument CharTy!");

	using view_type = BasicStringView<CharTy>;
	using container = BasicRuntimeFormatString;

	BasicRuntimeFormatString(view_type v) noexcept : m_view(v) { }
	BasicRuntimeFormatString(const container&) noexcept = delete;
	BasicRuntimeFormatString(container&&) noexcept = delete;

private:
	view_type m_view;

	template <class, class...> friend class BasicFormatString;
};

using RuntimeFormatString = BasicRuntimeFormatString<char>;
using WRuntimeFormatString = BasicRuntimeFormatString<wchar_t>;


template <class CharTy> class BasicFormatVerifier {
public:
	static_assert(std::is_same_v<CharTy, char> || std::is_same_v<CharTy, wchar_t>, "lsd::BasicRuntimeFormatString: Runtime format string only accepts char and wchar_t as valid types for template argument CharTy!");

	using view_type = BasicStringView<CharTy>;

	BasicFormatVerifier() = delete;
	BasicFormatVerifier& operator=(const BasicFormatVerifier&) = delete;
	BasicFormatVerifier& operator=(BasicFormatVerifier&&) = delete;

	static void verifyRuntime(view_type fmt) {
		
	}
	static consteval void verifyCompileTime(view_type fmt) {
		
	}

private:

};

using FormatVerifier = BasicFormatVerifier<char>;
using WFormatVerifier = BasicFormatVerifier<wchar_t>;

} // namespace detail

inline detail::RuntimeFormatString runtimeFormat(StringView fmt) noexcept {
	return detail::RuntimeFormatString(fmt);
}
inline detail::WRuntimeFormatString runtimeFormat(WStringView fmt) noexcept {
	return detail::WRuntimeFormatString(fmt);
}


template <class CharTy, class... Args> struct BasicFormatString {
public:
	static_assert(std::is_same_v<CharTy, char> || std::is_same_v<CharTy, wchar_t>, "lsd::BasicFormatString: Format string only accepts char and wchar_t as valid types for template argument CharTy!");

	using view_type = BasicStringView<CharTy>;
	using runtime_type = detail::BasicRuntimeFormatString<CharTy>;
	using verifier_type = detail::BasicFormatVerifier<CharTy>;

	template <class Ty> consteval BasicFormatString(const Ty& s) requires std::is_convertible_v<const Ty&, view_type> : m_view(s) {
		verifier_type::verifyCompileTime(m_view);
	}
	BasicFormatString(runtime_type s) noexcept : m_view(s.m_view) {
		verifier_type::verifyRuntime(m_view);
	}

	constexpr view_type get() const noexcept {
		return m_view;
	}

private:
	view_type m_view;
};

template <class... Args> using FormatString = BasicFormatString<char, std::type_identity_t<Args>...>;
template <class... Args> using WFormatString = BasicFormatString<wchar_t, std::type_identity_t<Args>...>;


template <class CharTy> class BasicFormatContext {
public:
	static_assert(std::is_same_v<CharTy, char> || std::is_same_v<CharTy, wchar_t>, "lsd::BasicFormatContext: Format context only accepts char and wchar_t as valid types for template argument CharTy!");

	using char_type = CharTy;
	
	using iterator = detail::BasicFormatBackInserter<char_type>;
	using view_type = BasicStringView<CharTy>;
	using view_iterator = typename view_type::const_iterator;

	template <class Ty> using formatter_type = Formatter<Ty, char_type>;

private:
	struct ReplacementFieldOptions {
		bool isReplacementField = true; // false means that the inserted character is just an escaped '{'
		bool useDefaultFormat = false;
		std::size_t fieldIndex = 0; // index of the current field in the format string
		std::size_t argumentIndex = 0; // index of the requested formatting argument
	};

	using field_options = ReplacementFieldOptions;

public:
	BasicFormatContext() = delete;
	BasicFormatContext& operator=(const BasicFormatContext&) = delete;
	BasicFormatContext& operator=(BasicFormatContext&&) = delete;

	template <class... Args> static void format(iterator outputIt, view_type fmt, Args&&... args) {
		field_options options { };

		for (auto it = fmt.begin(); it < fmt.end() && !outputIt.done(); it++) {
			switch (*it) {
				case char_type('{'):
					options = parseReplacementField(it, fmt.end(), options.fieldIndex);

					if (!options.isReplacmentField) outputIt = *it;
					else {

					}

					break;
				
				case char_type('}'):
					outputIt = *++it; // the reason why I don't put an extra switch block here to check for syntax is because the string is already checked for validity
					break;

				default:
					outputIt = *it;
					break;
			}
		}
	}

private:
	static field_options parseReplacementField(view_iterator& it, view_iterator end, std::size_t prevFieldIndex) {
		field_options fieldOptions;
		fieldOptions.fieldIndex = ++prevFieldIndex;

		char_type* helper { };
		
		switch (*++it) { // since the second character only has a few valid options
			case char_type('{'):
				fieldOptions.isReplacementField = false;
				--fieldOptions.fieldIndex;
				return fieldOptions;

				break;
			
			case char_type('0'):
			case char_type('1'):
			case char_type('2'):
			case char_type('3'):
			case char_type('4'):
			case char_type('5'):
			case char_type('6'):
			case char_type('7'):
			case char_type('8'):
			case char_type('9'):
				helper = it;

				for (; it < end && std::isdigit(*it); it++);
				if constexpr (std::is_same_v<char_type, char>) fieldOptions.argumentIndex = std::strtoull(&*it, &helper, 10);
				else fieldOptions.argumentIndex = std::wcstoull(&*it, &helper, 10);

				break;

			case char_type('}'):
				break;
		}

		for (; it < end; ++it) {
			
		}

		return fieldOptions;
	}
};

using FormatContext = BasicFormatContext<char>;
using WFormatContext = BasicFormatContext<wchar_t>;


template <> struct Formatter<char, char> {

};
template <> struct Formatter<char, wchar_t> {
	
};
template <> struct Formatter<wchar_t, wchar_t> {
	
};


template <class... Args> inline String format(FormatString<Args...> fmt, Args&&... args) {
	String out;
	FormatContext::format(
		detail::FormatBackInserter(
			&out,
			[](void* out, const char& v) { static_cast<String*>(out)->pushBack(v); },
			[](void* out) { return false; }
		), 
		fmt.get(), 
		std::forward<Args>(args)...
	);
	return out;
}
template <class... Args> inline WString format(WFormatString<Args...> fmt, Args&&... args) {
	WString out;
	FormatContext::format(
		detail::FormatBackInserter(
			&out,
			[](void* out, const wchar_t& v) { static_cast<WString*>(out)->pushBack(v); },
			[](void* out) { return false; }
		), 
		fmt.get(), 
		std::forward<Args>(args)...
	);
	return out;
}


template <class OutputIt, class... Args> inline OutputIt formatTo(OutputIt it, std::size_t n, FormatString<Args...> fmt, Args&&... args) {
	
}
template <class OutputIt, class... Args> inline OutputIt formatTo(OutputIt it, std::size_t n, WFormatString<Args...> fmt, Args&&... args) {
	
}


template <class... Args> inline void print(FormatString<Args...> fmt, Args&&... args) {
	print(stdout, fmt, std::forward<Args>(args)...);
}
template <class... Args> inline void print(std::FILE* stream, FormatString<Args...> fmt, Args&&... args) {
	
}
template <class... Args> inline void println(FormatString<Args...> fmt, Args&&... args) {
	println(stdout, fmt, std::forward<Args>(args)...);
}
template <class... Args> inline void println(std::FILE* stream, FormatString<Args...> fmt, Args&&... args) {
	
}

} // namespace lsd
