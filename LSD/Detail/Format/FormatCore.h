/*************************
 * @file FormatCore.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Core components for formatting library
 * 
 * @date 2024-08-20
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "../../UniquePointer.h"
#include "../../FunctionPointer.h"
#include "../../String.h"
#include "../../StringView.h"

#include <type_traits>
#include <exception>

namespace lsd {

// Forward declarations

class FormatError;

template <class> struct BasicFieldOptions;
template <class> class BasicFormatBackInserter;

template <class> class BasicFormatVerifier;

template <class> struct BasicRuntimeFormatString;
template <class, class...> struct BasicFormatString;

template <class, class> class BasicFormatSpec;

template <class> class BasicFormatArg;
template <class> class BasicFormatArgs;

template <class> struct BasicFormatContext;

template <class, class> struct Formatter {
	constexpr Formatter() = default;
	constexpr Formatter(const Formatter&) = default;
	constexpr Formatter& operator=(const Formatter&) = default;
};


namespace detail {

template <class> class TypeErasedFormatArg;
template <class> class BasicFormatArgStoreEmptyValue;
template <class, class...> class BasicFormatArgStore;

}


// Formatting exception
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
	lsd::String m_message { "Program terminated with lyra::FormatError: " }; // Disregards the standard requirement that the formatting error has to equal the message
};


namespace detail {

// Replacement field parsed options

template <class CharTy> struct BasicFieldOptions {
	using char_type = CharTy;

	std::size_t fieldIndex = std::numeric_limits<std::size_t>::max(); // Index of the current field in the format string
	std::size_t argumentIndex = 0; // Index of the requested formatting argument

	bool hasArrayIndex = false;
	std::size_t arrayIndex = 0;

	BasicStringView<char_type> formatSpec;
};

using FieldOptions = BasicFieldOptions<char>;
using WFieldOptions = BasicFieldOptions<wchar_t>;


// Type-errased back insert iterater

template <class CharTy> class BasicFormatBackInserter {
private:
	class Counter {
	public:
		template <class Done> Counter(Done&& d, std::size_t count) :
			m_done(std::forward<Done>(d)),
			m_count(count) { }
		constexpr Counter(Counter&&) = default;
		
		constexpr bool operator()(void* container) {
			return m_done(container, m_count);
		}

	private:
		bool (*m_done)(void*, std::size_t&);

		std::size_t m_count;
	};

public:
	static_assert(std::is_same_v<CharTy, char> || std::is_same_v<CharTy, wchar_t>, "lsd::detail::BasicFormatBackInserter: Format back insert iterator only accepts char and wchar_t as valid types for template argument CharTy!");

	using char_type = CharTy;
	using container = BasicFormatBackInserter;

	template <class PushBack, class Done> constexpr BasicFormatBackInserter(void* const container, PushBack&& pb, Done&& d, std::size_t count = 0) : 
		m_container(container), 
		m_pushBack(std::forward<PushBack>(pb)), 
		m_done(std::forward<Done>(d), count) { }
	constexpr BasicFormatBackInserter(BasicFormatBackInserter&& other) = default;

	constexpr container& operator=(char_type value) {
		m_pushBack(m_container, value); // Do not check for doneness since the formatter and context does that
		return *this;
	}

	constexpr container& operator*() { return *this; }
	constexpr container& operator++() { return *this; }
	constexpr container& operator++(int) { return *this; }

	template <class Container> constexpr Container& get() {
		return *static_cast<Container*>(m_container);
	}
	constexpr bool done() {
		return m_done(m_container);
	}

private:
	void* const m_container;
	void (*m_pushBack)(void*, const char_type&);
	Counter m_done;
};

using FormatBackInserter = BasicFormatBackInserter<char>;
using WFormatBackInserter = BasicFormatBackInserter<wchar_t>;


// Format verifier

template <class CharTy, class... Args> class BasicFormatVerifier {
public:
	static_assert(std::is_same_v<CharTy, char> || std::is_same_v<CharTy, wchar_t>, "lsd::BasicRuntimeFormatString: Runtime format string only accepts char and wchar_t as valid types for template argument CharTy!");

	using view_type = BasicStringView<CharTy>;

	constexpr BasicFormatVerifier() = delete;
	constexpr BasicFormatVerifier& operator=(const BasicFormatVerifier&) = delete;
	constexpr BasicFormatVerifier& operator=(BasicFormatVerifier&&) = delete;

	static void verifyRuntime(view_type fmt) {
		/*
		else throw FormatError("lsd::BasicFormatContext()::format(): Can't take array element from non array-like argument type");
		else throw FormatError("lsd::BasicFormatContext()::format(): Argument index out of bounds");
		else throw FormatError("lsd::BasicFormatContext()::format(): Format field found when no arguments were provided");
		*/
	}
	static consteval void verifyCompileTime(view_type fmt) {
		
	}

private:

};

using FormatVerifier = BasicFormatVerifier<char>;
using WFormatVerifier = BasicFormatVerifier<wchar_t>;


// Runtime checked formatting string type

template <class CharTy> struct BasicRuntimeFormatString {
public:
	static_assert(std::is_same_v<CharTy, char> || std::is_same_v<CharTy, wchar_t>, "lsd::BasicRuntimeFormatString: Runtime format string only accepts char and wchar_t as valid types for template argument CharTy!");

	using view_type = BasicStringView<CharTy>;
	using container = BasicRuntimeFormatString;
	using verifier_type = detail::BasicFormatVerifier<CharTy>;

	BasicRuntimeFormatString(view_type v) noexcept : m_view(v) {
		verifier_type::verifyRuntime(v);
	}
	BasicRuntimeFormatString(const container&) noexcept = delete;
	BasicRuntimeFormatString(container&&) noexcept = delete;

private:
	view_type m_view;

	template <class, class...> friend struct ::lsd::BasicFormatString;
};

using RuntimeFormatString = BasicRuntimeFormatString<char>;
using WRuntimeFormatString = BasicRuntimeFormatString<wchar_t>;


// Formatter utility

// Utility variables

static constexpr auto digitsLow = "0123456789abcdefghijklmnopqrstuvpxyz";
static constexpr auto digitsUp = "0123456789ABCDEFGHIJKLMNOPQRSTUVPXYZ";

static constexpr auto wDigitsLow = L"0123456789abcdefghijklmnopqrstuvpxyz";
static constexpr auto wDigitsUp = L"0123456789ABCDEFGHIJKLMNOPQRSTUVPXYZ";

} // namespace detail


// Formatting string types

[[nodiscard]] inline detail::RuntimeFormatString runtimeFormat(StringView fmt) noexcept {
	return detail::RuntimeFormatString(fmt);
}
[[nodiscard]] inline detail::WRuntimeFormatString runtimeFormat(WStringView fmt) noexcept {
	return detail::WRuntimeFormatString(fmt);
}
[[deprecated]] [[nodiscard]] inline detail::RuntimeFormatString runtime_format(StringView fmt) noexcept {
	return detail::RuntimeFormatString(fmt);
}
[[deprecated]] [[nodiscard]] inline detail::WRuntimeFormatString runtime_format(WStringView fmt) noexcept {
	return detail::WRuntimeFormatString(fmt);
}


// Formatting string

template <class CharTy, class... Args> struct BasicFormatString {
public:
	static_assert(std::is_same_v<CharTy, char> || std::is_same_v<CharTy, wchar_t>, "lsd::BasicFormatString: Format string only accepts char and wchar_t as valid types for template argument CharTy!");

	using view_type = BasicStringView<CharTy>;
	using runtime_type = detail::BasicRuntimeFormatString<CharTy>;
	using verifier_type = detail::BasicFormatVerifier<CharTy, Args...>;

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

} // namespace lsd
