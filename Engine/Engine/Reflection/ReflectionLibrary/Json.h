#pragma once

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <string>
#include <type_traits>
#include <optional>

namespace ReflectionLibrary
{
    // Serialize value to JSON
    template <typename T>
    static void serialize_value(const T &value, rapidjson::Value &json_val, rapidjson::Document::AllocatorType &allocator)
    {
        using ValueType = std::decay_t<T>;

        if constexpr (std::is_same_v<ValueType, bool>)
        {
            json_val.SetBool(value);
        }
        else if constexpr (std::is_integral_v<ValueType> && std::is_signed_v<ValueType>)
        {
            if constexpr (sizeof(ValueType) <= sizeof(int))
            {
                json_val.SetInt(static_cast<int>(value));
            }
            else
            {
                json_val.SetInt64(static_cast<int64_t>(value));
            }
        }
        else if constexpr (std::is_integral_v<ValueType> && std::is_unsigned_v<ValueType>)
        {
            if constexpr (sizeof(ValueType) <= sizeof(unsigned int))
            {
                json_val.SetUint(static_cast<unsigned int>(value));
            }
            else
            {
                json_val.SetUint64(static_cast<uint64_t>(value));
            }
        }
        else if constexpr (std::is_floating_point_v<ValueType>)
        {
            json_val.SetDouble(static_cast<double>(value));
        }
        else if constexpr (std::is_same_v<ValueType, std::string>)
        {
            json_val.SetString(value.c_str(), static_cast<rapidjson::SizeType>(value.length()), allocator);
        }
        else if constexpr (std::is_same_v<ValueType, const char *> || std::is_same_v<ValueType, char *>)
        {
            json_val.SetString(value, allocator);
        }
        else if constexpr (is_pair_v<ValueType>)
        {
            json_val.SetObject();
            rapidjson::Value first_val, second_val;
            serialize_value(value.first, first_val, allocator);
            serialize_value(value.second, second_val, allocator);
            json_val.AddMember("first", first_val, allocator);
            json_val.AddMember("second", second_val, allocator);
        }
        else if constexpr (is_tuple_v<ValueType>)
        {
            json_val.SetArray();
            std::apply([&](const auto &...args)
                       { ((
                              [&]()
                              {
                                  rapidjson::Value elem;
                                  serialize_value(args, elem, allocator);
                                  json_val.PushBack(elem, allocator);
                              }()),
                          ...); }, value);
        }
        else if constexpr (is_iterable_v<ValueType>)
        {
            json_val.SetArray();
            for (const auto &elem : value)
            {
                rapidjson::Value elem_val;
                serialize_value(elem, elem_val, allocator);
                json_val.PushBack(elem_val, allocator);
            }
        }
        else if constexpr (is_reflectable_v<ValueType>)
        {
            json_val.SetObject();
            auto &obj_ref = const_cast<ValueType &>(value);
            for_each_field(obj_ref, [&](std::string_view name, auto &field_value, int)
                           {
                rapidjson::Value field_val;
                serialize_value(field_value, field_val, allocator);
                rapidjson::Value key(name.data(), static_cast<rapidjson::SizeType>(name.length()), allocator);
                json_val.AddMember(key, field_val, allocator); });
        }
        else
        {
            // Fallback for unsupported types
            json_val.SetNull();
        }
    }

    // Deserialize value from JSON
    template <typename T>
    static bool deserialize_value(T &value, const rapidjson::Value &json_val)
    {
        using ValueType = std::decay_t<T>;

        if constexpr (std::is_same_v<ValueType, bool>)
        {
            if (!json_val.IsBool())
                return false;
            value = json_val.GetBool();
        }
        else if constexpr (std::is_integral_v<ValueType> && std::is_signed_v<ValueType>)
        {
            if (json_val.IsInt64())
                value = static_cast<ValueType>(json_val.GetInt64());
            else if (json_val.IsInt())
                value = static_cast<ValueType>(json_val.GetInt());
            else
                return false;
        }
        else if constexpr (std::is_integral_v<ValueType> && std::is_unsigned_v<ValueType>)
        {
            if (json_val.IsUint64())
                value = static_cast<ValueType>(json_val.GetUint64());
            else if (json_val.IsUint())
                value = static_cast<ValueType>(json_val.GetUint());
            else
                return false;
        }
        else if constexpr (std::is_floating_point_v<ValueType>)
        {
            if (json_val.IsDouble())
                value = static_cast<ValueType>(json_val.GetDouble());
            else if (json_val.IsInt())
                value = static_cast<ValueType>(json_val.GetInt());
            else
                return false;
        }
        else if constexpr (std::is_same_v<ValueType, std::string>)
        {
            if (!json_val.IsString())
                return false;
            value = json_val.GetString();
        }
        else if constexpr (is_pair_v<ValueType>)
        {
            if (!json_val.IsObject())
                return false;
            if (json_val.HasMember("first") && !deserialize_value(value.first, json_val["first"]))
                return false;
            if (json_val.HasMember("second") && !deserialize_value(value.second, json_val["second"]))
                return false;
        }
        else if constexpr (is_tuple_v<ValueType>)
        {
            if (!json_val.IsArray())
                return false;
            size_t idx = 0;
            bool success = true;
            std::apply([&](auto &...args)
                       { (([&]()
                           {
                    if (idx < json_val.Size() && success)
                        success = deserialize_value(args, json_val[static_cast<rapidjson::SizeType>(idx)]);
                    ++idx; }()),
                          ...); }, value);
            return success;
        }
        else if constexpr (is_iterable_v<ValueType>)
        {
            if (!json_val.IsArray())
                return false;
            value.clear();

            // Handle maps/sets differently from vectors/lists
            if constexpr (requires { typename ValueType::key_type; typename ValueType::mapped_type; })
            {
                // Map type
                for (auto it = json_val.Begin(); it != json_val.End(); ++it)
                {
                    std::pair<typename ValueType::key_type, typename ValueType::mapped_type> elem{};
                    if (!deserialize_value(elem, *it))
                        return false;
                    value.insert(std::move(elem));
                }
            }
            else
            {
                // Sequential container
                for (auto it = json_val.Begin(); it != json_val.End(); ++it)
                {
                    typename ValueType::value_type elem{};
                    if (!deserialize_value(elem, *it))
                        return false;
                    value.insert(value.end(), std::move(elem));
                }
            }
        }
        else if constexpr (is_reflectable_v<ValueType>)
        {
            if (!json_val.IsObject())
                return false;
            bool success = true;
            for_each_field(value, [&](std::string_view name, auto &field_value, int)
                           {
                if (json_val.HasMember(name.data()))
                {
                    if (!deserialize_value(field_value, json_val[name.data()]))
                        success = false;
                } });
            return success;
        }

        return true;
    }

    // Serialize object to JSON string
    template <typename T>
    static std::string to_json(T &obj, bool pretty = false)
    {
        rapidjson::Document doc;
        doc.SetObject();
        auto &allocator = doc.GetAllocator();

        for_each_field(obj, [&](std::string_view name, auto &field_value, int)
                       {
            rapidjson::Value field_val;
            serialize_value(field_value, field_val, allocator);
            rapidjson::Value key(name.data(), static_cast<rapidjson::SizeType>(name.length()), allocator);
            doc.AddMember(key, field_val, allocator); });

        rapidjson::StringBuffer buffer;
        if (pretty)
        {
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);
        }
        else
        {
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);
        }

        return buffer.GetString();
    }

    // Deserialize object from JSON string
    template <typename T>
    static std::optional<T> from_json(const std::string &json_str)
    {
        rapidjson::Document doc;
        doc.Parse(json_str.c_str());

        if (doc.HasParseError() || !doc.IsObject())
            return std::nullopt;

        T obj{};
        bool success = true;
        for_each_field(obj, [&](std::string_view name, auto &field_value, int)
                       {
        if (doc.HasMember(name.data()))
        {
            if (!deserialize_value(field_value, doc[name.data()]))
                success = false;
        } });

        if (!success)
            return std::nullopt;

        return obj;
    }

} // namespace ReflectionLibrary