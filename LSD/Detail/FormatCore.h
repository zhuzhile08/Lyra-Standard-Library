/*************************
 * @file FormatCore.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Core components for formatting library
 * 
 * @date 2024-08-20
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "../String.h"
#include "../StringView.h"

#include <type_traits>
#include <exception>

namespace lsd {

// forward declarations
template <class> struct BasicFieldOptions;
class FormatError;
template <class> class BasicFormatBackInserter;
template <class> struct BasicRuntimeFormatString;
template <class> class BasicFormatVerifier;
template <class> class BasicFormatArg;

template <class, class...> struct BasicFormatString;

// formatting exception
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

// replacement field parsed options
template <class CharTy> struct BasicFieldOptions {
	bool isReplacementField = true; // false means that the inserted character is just an escaped '{'
	bool useDefaultFormat = false;

	std::size_t fieldIndex = std::numeric_limits<std::size_t>::max(); // index of the current field in the format string
	std::size_t argumentIndex = 0; // index of the requested formatting argument
	std::size_t arrayIndex = 0;

	char fillChr = ' ';
	char align = '<';
	
	char sign = '-';
	bool negativeZero = true; // true means the format uses negative zero
	bool alternateForm = false;

	std::size_t fillCount = 0;

	BasicStringView<CharTy> typeFormat;
};

using FieldOptions = BasicFieldOptions<char>;
using WFieldOptions = BasicFieldOptions<wchar_t>;


// type-errased back insert iterater

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


// unchecked runtime formatting string type

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

	template <class, class...> friend struct ::lsd::BasicFormatString;
};

using RuntimeFormatString = BasicRuntimeFormatString<char>;
using WRuntimeFormatString = BasicRuntimeFormatString<wchar_t>;


// format verifier

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


// formatting string types

inline detail::RuntimeFormatString runtimeFormat(StringView fmt) noexcept {
	return detail::RuntimeFormatString(fmt);
}
inline detail::WRuntimeFormatString runtimeFormat(WStringView fmt) noexcept {
	return detail::WRuntimeFormatString(fmt);
}
[[deprecated]] inline detail::RuntimeFormatString runtime_format(StringView fmt) noexcept {
	return detail::RuntimeFormatString(fmt);
}
[[deprecated]] inline detail::WRuntimeFormatString runtime_format(WStringView fmt) noexcept {
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


// format argument store

template <class ContextType> class BasicFormatArg {
private:
	class Handle {
	private:
		const void* m_object;

	};

	using char_type = ContextType::char_type;
	using iterator = ContextType::iterator;
	using handle_type = Handle;
	using variant_type = std::variant<
		std::monostate,
		bool,
		char_type,
		int,
		unsigned int,
		long long int,
		unsigned long long int,
		float,
		double,
		long double,
		const char_type*,
		BasicStringView<char_type>,
		const void*,
		handle_type
	>;

public:
	template <class Ty> constexpr BasicFormatArg(Ty&& value) noexcept {
		using type = std::remove_const_t<std::remove_reference_t<Ty>>;

		if constexpr (std::is_same_v<bool, type> || std::is_same_v<char_type, type> || std::is_same_v<float, type> || std::is_same_v<double, type> || std::is_same_v<long double, type>) m_value = value;
		else if constexpr (std::is_same_v<char, type> || std::is_same_v<wchar_t, char_type>) m_value = implicitCast<wchar_t>(implicitCast<unsigned char>(value));
		else if constexpr (std::is_integral_v<type> && sizeof(type) <= sizeof(int)) m_value = implicitCast<int>(value);
		else if constexpr (std::is_integral_v<type> && std::is_unsigned_v<type> && sizeof(type) <= sizeof(unsigned int)) m_value = implicitCast<unsigned int>(value);
		else if constexpr (std::is_integral_v<type> && sizeof(type) <= sizeof(long long)) m_value = implicitCast<long long>(value);
		else if constexpr (std::is_integral_v<type> && std::is_unsigned_v<type> && sizeof(type) <= sizeof(unsigned long long)) m_value = implicitCast<unsigned long long>(value);
		else if constexpr (std::is_same_v<BasicStringView<char_type>, type> || std::is_same_v<BasicString<char_type>, type>) m_value = BasicStringView<char_type>(value.data(), value.size());
		else if constexpr (std::is_same_v<std::decay_t<type>, char_type*> || std::is_same_v<std::decay_t<type>, const char_type*>) m_value = implicitCast<const char_type*>(value);
		else if constexpr (std::is_void_v<std::remove_pointer_t<type>> || std::is_null_pointer_v<type>) m_value = implicitCast<const void*>(value);
		// else 
	}

	constexpr explicit operator bool() const noexcept {
		return !std::holds_alternative<std::monostate>(m_value);
	}

	constexpr void format(iterator outputIt, detail::BasicFieldOptions<char_type> options) const {

	}

private:
	variant_type m_value;
};

using FormatArg = BasicFormatArg<char>;
using WFormatArg = BasicFormatArg<wchar_t>;

} // namespace lsd
