#include <algorithm>
#include <any>
#include <array>
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

template<typename... T>
struct overloaded : T...
{
    using T::operator()...;
};

template<typename... T>
overloaded(T...)->overloaded<T...>;

// variant type that supports values of the following types:
// int64_t, uint64_t, double, std::string, bool, array, string-keyed map
class Variant
{
public:
    using VariantPtr            = std::unique_ptr<Variant>;
    using VariantArray          = std::vector<VariantPtr>;
    using VariantMap            = std::unordered_map<std::string, VariantPtr>;
    using InitializationVariant = std::variant<std::nullopt_t, int64_t, uint64_t, double, std::string, bool, Variant>;
    // scalar constructor
    // initializer list constructor
    // default constructor

    Variant() : m_data(std::nullopt) {}

    Variant(const Variant& copy) : m_data(std::nullopt) { *this = copy; }
    auto operator=(const Variant& copy) -> Variant&
    {
        std::visit(
            overloaded{[this](const VariantArray& value) {
                           VariantArray new_value;
                           new_value.reserve(value.size());

                           for (const auto& item : value)
                           {
                               // item is std::unique_ptr<Variant>
                               new_value.push_back(std::make_unique<Variant>(*item));
                           }

                           m_data = std::move(new_value);
                       },
                       [this](const VariantMap& value) {
                           VariantMap new_value;
                           new_value.reserve(value.size());

                           for (const auto& [key, item] : value)
                           {
                               // item is std::unique_ptr<Variant>
                               new_value[key] = std::make_unique<Variant>(*item);
                           }

                           m_data = std::move(new_value);
                       },
                       [this](const auto& value) { m_data = value; }},
            copy.m_data);

        return *this;
    }

    Variant(Variant&&) = default;
    auto operator=(Variant &&) -> Variant& = default;

    Variant(InitializationVariant value) : m_data(std::nullopt)
    {
        std::visit(
            overloaded{[this](Variant value) { *this = std::move(value); }, // m_data = std::move(value.m_data); },
                       [this](auto value) { m_data = std::move(value); }},
            std::move(value));
    }

    Variant(std::initializer_list<InitializationVariant> list) : m_data(std::nullopt)
    {
        VariantArray new_array;
        new_array.reserve(list.size());

        for (auto& item : list)
        {
            std::visit(
                [&new_array](auto value) { new_array.push_back(std::make_unique<Variant>(std::move(value))); },
                std::move(item));
        }

        m_data = std::move(new_array);
    }

    Variant(std::initializer_list<std::pair<std::string, InitializationVariant>> list) : m_data(std::nullopt)
    {
        VariantMap new_map;
        new_map.reserve(list.size());

        for (auto& item_pair : list)
        {
            std::visit(
                [&new_map, &item_pair](auto value) {
                    new_map[std::move(item_pair.first)] = std::make_unique<Variant>(std::move(value));
                },
                std::move(item_pair.second));
        }

        m_data = std::move(new_map);
    }

    auto PrettyPrint(std::ostream& stream) -> void { pretty_print_impl(0, stream); }

private:
    auto pretty_print_impl(size_t tab_count, std::ostream& stream) -> void
    {
        std::vector<char> tabs(tab_count + 1, '\t');
        // no one line
        std::visit(
            overloaded{[&stream, tab_count, &tabs](const VariantArray& value) {
                           stream << '[' << std::endl;

                           bool first = true;
                           for (const auto& item : value)
                           {
                               if (!first)
                               {
                                   stream << ',' << std::endl;
                               }
                               first = false;

                               stream.write(tabs.data(), tab_count + 1);
                               item->pretty_print_impl(tab_count + 1, stream);
                           }

                           stream << std::endl;
                           stream.write(tabs.data(), tab_count);
                           stream << ']';
                       },
                       [&stream, tab_count, &tabs](const VariantMap& value) {
                           stream << '{' << std::endl;

                           bool first = true;
                           for (const auto& [key, item] : value)
                           {
                               if (!first)
                               {
                                   stream << ',' << std::endl;
                               }
                               first = false;

                               stream.write(tabs.data(), tab_count + 1);
                               stream << '"' << key << "\": ";
                               item->pretty_print_impl(tab_count + 1, stream);
                           }

                           stream << std::endl;
                           stream.write(tabs.data(), tab_count);
                           stream << '}';
                       },
                       [&stream](std::nullopt_t) { stream << "null"; },
                       [&stream](const std::string& value) { stream << '"' << value << '"'; },
                       [&stream](const auto& value) { stream << value; }},
            m_data);
    }
    std::variant<std::nullopt_t, int64_t, uint64_t, double, std::string, bool, VariantArray, VariantMap> m_data;
};

auto main(int argc, char** argv, char** envp) -> int32_t
{
    (void)argc;
    (void)argv;
    (void)envp;

    std::vector<int64_t> derp{5, 10};
    std::vector<int64_t> flerp({5, 10});
    std::vector<int64_t> herp{};
    std::vector<int64_t> burp();

    for (const auto& item : derp)
    {
        std::cout << "derp: " << item << std::endl;
    }
    for (const auto& item : flerp)
    {
        std::cout << "flerp: " << item << std::endl;
    }

    Variant a(5l);
    Variant b{5l};
    Variant c{1l,
              2l,
              3l,
              4l,
              "derp",
              Variant{1l, 3l, 3l, 7l},
              Variant{{"key", "value"},
                      {"leet", 13.37},
                      {"some string", Variant{"nested", "array"}},
                      {"nested key", Variant{{"nested", "map"}}}}};

    std::cout << "a:" << std::endl;
    a.PrettyPrint(std::cout);
    std::cout << std::endl << std::endl;

    std::cout << "b:" << std::endl;
    b.PrettyPrint(std::cout);
    std::cout << std::endl << std::endl;

    std::cout << "c:" << std::endl;
    c.PrettyPrint(std::cout);
    std::cout << std::endl << std::endl;

    Variant d{{"a", std::move(a)}, {"b", std::move(b)}, {"c", std::move(c)}};

    std::cout << "d:" << std::endl;
    d.PrettyPrint(std::cout);
    std::cout << std::endl << std::endl;

    return 0;
}
