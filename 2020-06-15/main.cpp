#include <algorithm>
#include <any>
#include <array>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <regex>
#include <set>
#include <stack>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

template<typename T>
concept IterableChars = requires(T a)
{
    {
        *a.begin()
    }
    ->char;
    {a.end()};
};

template<typename T>
concept ContiguousChars = IterableChars<T>&& requires(T a)
{
    {
        a.data()
    }
    ->const char*;
    {
        a.size()
    }
    ->size_t;
};

template<typename T>
concept CharValue = std::is_same<std::remove_reference_t<std::decay_t<T>>, char>::value;

template<typename T>
concept ToCharsConvertible = !CharValue<T> && requires(T a)
{
    {
        std::to_chars(nullptr, nullptr, a)
    }
    ->std::to_chars_result;
};

template<typename T>
struct ToCharsCoverage
{
};

template<ToCharsConvertible T>
struct ToCharsCoverage<T>
{
    static inline auto to_chars(char* buf_start, char* buf_end, T value) -> std::to_chars_result
    {
        return std::to_chars(buf_start, buf_end, value);
    }
};

template<>
struct ToCharsCoverage<double>
{
    static inline auto to_chars(char* buf_start, char* buf_end, double value) -> std::to_chars_result
    {
        auto count = snprintf(buf_start, buf_end - buf_start, "%g", value);
        return {buf_start + count, std::errc()};
    }
};

template<>
struct ToCharsCoverage<float>
{
    static inline auto to_chars(char* buf_start, char* buf_end, float value) -> std::to_chars_result
    {
        auto count = snprintf(buf_start, buf_end - buf_start, "%g", value);
        return {buf_start + count, std::errc()};
    }
};

template<typename T>
concept ToCharsCoverageConvertible = !CharValue<T> && requires(T a)
{
    {
        ToCharsCoverage<T>::to_chars(nullptr, nullptr, a)
    }
    ->std::to_chars_result;
};

template<typename T>
concept StringBuilderStreamable = IterableChars<T> || ContiguousChars<T> || ToCharsCoverageConvertible<T> ||
                                  CharValue<T> || std::is_same<std::remove_const_t<std::decay_t<T>>, const char*>::value;

template<typename T>
concept IterableStringBuilderStreamables = !IterableChars<T> && requires(T a)
{
    {
        *a.begin();
    }
    ->StringBuilderStreamable;
    {a.end()};
};

template<typename T>
concept LocaleInfo = requires(T)
{
    {
        T::newline
    }
    ->StringBuilderStreamable;
};

template<LocaleInfo Locale>
class stringbuilder_base
{
public:
    static constexpr auto newline = Locale::newline;

    stringbuilder_base() noexcept : m_data() {}
    stringbuilder_base(std::string init) noexcept : m_data(std::move(init)) {}

    stringbuilder_base(const stringbuilder_base&) noexcept = default;
    stringbuilder_base(stringbuilder_base&&) noexcept      = default;
    auto operator=(const stringbuilder_base&) noexcept -> stringbuilder_base& = default;
    auto operator=(stringbuilder_base&&) noexcept -> stringbuilder_base& = default;

    ~stringbuilder_base() = default;

    auto reserve(size_t capacity) -> void { m_data.reserve(capacity); }

    /*template<typename T>
    auto operator<<(T&& val) -> stringbuilder_base&;*/

    template<IterableChars T>
    auto operator<<(T&& char_container) -> stringbuilder_base&
    {
        m_data.append(char_container.begin(), char_container.end());
        return *this;
    }

    template<ContiguousChars T>
    auto operator<<(T&& contiguous_chars) -> stringbuilder_base&
    {
        m_data.append(contiguous_chars.data(), contiguous_chars.size());
        return *this;
    }

    template<ToCharsCoverageConvertible T>
    auto operator<<(T&& to_chars_convertible) -> stringbuilder_base&
    {
        constexpr size_t buffer_size = 32; // big enough for the largest number (long double)
        const auto       start_size  = m_data.size();
        m_data.resize(start_size + buffer_size);

        auto* insertion_ptr = m_data.data() + start_size;

        auto [new_end_ptr, error_code] =
            ToCharsCoverage<T>::to_chars(insertion_ptr, insertion_ptr + buffer_size, to_chars_convertible);

        if (error_code == std::errc())
        {
            m_data.resize(new_end_ptr - m_data.data());
        }
        else
        {
            throw std::runtime_error("to_chars failed");
        }

        return *this;
    }

    template<CharValue T>
    auto operator<<(T c) -> stringbuilder_base&
    {
        m_data += c;
        return *this;
    }

    template<IterableStringBuilderStreamables T>
    auto operator<<(T&& data) -> stringbuilder_base&
    {
        for (auto&& item : data)
        {
            *this << item;
        }
        return *this;
    }

    auto operator<<(const char* ptr) -> stringbuilder_base&
    {
        m_data.append(ptr);
        return *this;
    }

    auto copy_result() const -> std::string { return m_data; }
    auto finish() && noexcept -> std::string { return std::move(m_data); }

private:
    std::string m_data;
};

struct NixLocale
{
    static constexpr char newline = '\n';
};
using stringbuilder = stringbuilder_base<NixLocale>;

auto main(int argc, char** argv, char** envp) -> int32_t
{
    (void)argc;
    (void)argv;
    (void)envp;

    // *to_chars

    // *double \ = to_chars..?
    // *int    /

    // *newline - our type/value...?

    // *char*

    // HAVE begin() and end()
    // *string_view - char
    // *string - char
    // *vector<char> - char

    // *list<char> - char

    // vector<char*> - char*
    // *char

    stringbuilder b;
    b << "Hello, World!" << 13.37 << 1337 << stringbuilder::newline;

    std::string_view derp{"lolol"};
    b << derp;

    std::vector<char> herp{'y', 'a', 's'};

    std::string flerp{"flerp"};

    b << herp << flerp << stringbuilder::newline;

    std::list<char> herp_list{herp.begin(), herp.end()};
    b << herp_list << stringbuilder::newline;

    std::vector many_strs{"hi", "there", "noob"};

    b << many_strs << stringbuilder::newline;

    std::cout << std::move(b).finish() << std::endl;

    return 0;
}
