#pragma once

#include <string>
#include <string_view>
#include <iostream>

namespace ReflectionLibrary
{
    template <typename T>
    static void print_value(std::string_view name, T &value, int nest_level)
    {
        std::string indent(2 + nest_level * 2, ' ');
        using ValueType = std::decay_t<T>;
        if constexpr (std::is_same_v<ValueType, bool>)
        {
            std::cout << indent << name << " = " << (value ? "true" : "false") << "\n";
        }
        else if constexpr (std::is_same_v<ValueType, char> || std::is_same_v<ValueType, unsigned char>)
        {
            std::cout << indent << name << " = '" << value << "'\n";
        }
        else if constexpr (std::is_arithmetic_v<ValueType>)
        {
            // Other arithmetic types (int, float, etc.)
            std::cout << indent << name << " = " << value << "\n";
        }
        else if constexpr (std::is_same_v<ValueType, std::string>)
        {
            std::cout << indent << name << " = \"" << value << "\"\n";
        }
        else if constexpr (is_iterable_v<ValueType>)
        {
            // Iterable containers (arrays, vectors, lists, maps, etc.)
            std::cout << indent << name << " = [\n";
            int i = 0;
            for (auto &elem : value)
            {
                print_value(std::to_string(i++), elem, nest_level + 1);
            }
            std::cout << indent << "]\n";
        }
        else if constexpr (is_pair_v<ValueType>)
        {
            std::cout << indent << name << " = {\n";
            print_value("first", value.first, nest_level + 1);
            print_value("second", value.second, nest_level + 1);
            std::cout << indent << "}\n";
        }
        else if constexpr (is_tuple_v<ValueType>)
        {
            std::cout << indent << name << " = (\n";
            std::apply([&](auto &...args)
                       {
                int i = 0;
                ((print_value(std::to_string(i++), args, nest_level + 1)), ...); }, value);
            std::cout << indent << ")\n";
        }
        else if constexpr (is_reflectable_v<ValueType>)
        {
            // Nested reflectable type - print header and recurse into fields
            std::cout << indent << name << " (" << ReflectionInfo<ValueType>::class_name << "):\n";
            for_each_field(value, [&](std::string_view sub_name, auto &sub_value, int)
                           { print_value(sub_name, sub_value, nest_level + 1); }, nest_level + 1);
        }
        else
        {
            // Fallback for other non-reflectable types
            std::cout << indent << name << " = [non-reflectable type]\n";
        }
    }

    template <typename T>
    static void print(T &obj)
    {
        std::cout << "Type: " << ReflectionInfo<T>::class_name << "\n";
        for_each_field(obj, [](std::string_view name, auto &value, int nest_level)
                       { print_value(name, value, nest_level); }, 0);
    }
} // namespace ReflectionLibrary
