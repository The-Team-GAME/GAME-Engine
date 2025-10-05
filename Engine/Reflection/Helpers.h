#pragma once

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <type_traits>
#include "Reflect.h"

// Old implementation that does not support nested structs
// Simpler to understand so if you want to see how reflection works, check this out
// template <typename T>
// void print_struct_fields(const T& obj)
// {
//   auto [class_name, fields_tuple] = reflection_fields<T>();

//   std::cout << "Type: " << class_name << " has fields:\n";
//   std::apply([&obj](const auto &...pairs)
//     {
//       ((std::cout << "- " << pairs.first << ": " << (obj.*pairs.second) << "\n"), ...);
//     },
//     fields_tuple
//   );
// }


template <typename T>
void print_struct_fields(const T& obj, int indent = 0)
{
  auto [class_name, fields_tuple] = reflection_fields<T>();

  std::string indent_str(indent, ' ');
  std::cout << indent_str << "Type: " << class_name << " has fields:\n";

  std::apply([&obj, indent](const auto &...pairs)
    {
      ((
        [&]()
        {
          std::string field_indent(indent + 2, ' ');
          std::cout << field_indent << "- " << pairs.first << ": ";

          using FieldType = std::remove_cvref_t<decltype(obj.*pairs.second)>;

          if constexpr (has_reflection_v<FieldType>)
          {
            // Nested struct with reflection
            std::cout << "\n";
            print_struct_fields(obj.*pairs.second, indent + 4);
          }
          else
          {
            // Primitive type
            std::cout << (obj.*pairs.second) << "\n";
          }
        }()
          ), ...);
    },
    fields_tuple
  );
}

// Forward declarations
template <typename T>
std::string serialize_to_json(const T& obj);

template <typename T>
bool deserialize_from_json(T& obj, const std::string& json_str);

// Helper to set JSON value based on type
template<typename T>
void set_json_value(rapidjson::Value& val, const T& data, rapidjson::Document::AllocatorType& allocator)
{
  if constexpr (std::is_same_v<T, std::string>)
  {
    val.SetString(data.c_str(), static_cast<rapidjson::SizeType>(data.length()), allocator);
  }
  else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int32_t>)
  {
    val.SetInt(data);
  }
  else if constexpr (std::is_same_v<T, unsigned int> || std::is_same_v<T, uint32_t>)
  {
    val.SetUint(data);
  }
  else if constexpr (std::is_same_v<T, int64_t>)
  {
    val.SetInt64(data);
  }
  else if constexpr (std::is_same_v<T, uint64_t>)
  {
    val.SetUint64(data);
  }
  else if constexpr (std::is_same_v<T, float>)
  {
    val.SetFloat(data);
  }
  else if constexpr (std::is_same_v<T, double>)
  {
    val.SetDouble(data);
  }
  else if constexpr (std::is_same_v<T, bool>)
  {
    val.SetBool(data);
  }
  else if constexpr (has_reflection_v<T>)
  {
    // Handle nested reflected structs
    auto [class_name, fields_tuple] = reflection_fields<T>();
    val.SetObject();

    std::apply([&](const auto&... pairs)
      {
        ((
          [&]()
          {
            rapidjson::Value key(pairs.first, allocator);
            rapidjson::Value nested_value;

            using FieldType = std::remove_cvref_t<decltype(data.*pairs.second)>;
            set_json_value<FieldType>(nested_value, data.*pairs.second, allocator);

            val.AddMember(key, nested_value, allocator);
          }()
            ), ...);
      },
      fields_tuple
    );
  }
}

// Helper to get value from JSON
template<typename T>
void get_json_value(T& data, const rapidjson::Value& val)
{
  if constexpr (std::is_same_v<T, std::string>)
  {
    if (val.IsString())
      data = val.GetString();
  }
  else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int32_t>)
  {
    if (val.IsInt())
      data = val.GetInt();
  }
  else if constexpr (std::is_same_v<T, unsigned int> || std::is_same_v<T, uint32_t>)
  {
    if (val.IsUint())
      data = val.GetUint();
  }
  else if constexpr (std::is_same_v<T, int64_t>)
  {
    if (val.IsInt64())
      data = val.GetInt64();
  }
  else if constexpr (std::is_same_v<T, uint64_t>)
  {
    if (val.IsUint64())
      data = val.GetUint64();
  }
  else if constexpr (std::is_same_v<T, float>)
  {
    if (val.IsFloat())
      data = val.GetFloat();
  }
  else if constexpr (std::is_same_v<T, double>)
  {
    if (val.IsDouble())
      data = val.GetDouble();
  }
  else if constexpr (std::is_same_v<T, bool>)
  {
    if (val.IsBool())
      data = val.GetBool();
  }
  else if constexpr (has_reflection_v<T>)
  {
    // Handle nested reflected structs
    if (!val.IsObject())
      return;

    auto [class_name, fields_tuple] = reflection_fields<T>();

    std::apply([&](const auto&... pairs)
      {
        ((
          [&]()
          {
            if (val.HasMember(pairs.first))
            {
              using FieldType = std::remove_cvref_t<decltype(data.*pairs.second)>;
              get_json_value<FieldType>(data.*pairs.second, val[pairs.first]);
            }
          }()
            ), ...);
      },
      fields_tuple
    );
  }
}

// Serialize struct to JSON string
template <typename T>
std::string serialize_to_json(const T& obj)
{
  auto [class_name, fields_tuple] = reflection_fields<T>();

  rapidjson::Document doc;
  doc.SetObject();
  auto& allocator = doc.GetAllocator();

  std::apply([&](const auto&... pairs)
    {
      ((
        [&]()
        {
          rapidjson::Value key(pairs.first, allocator);
          rapidjson::Value value;

          using FieldType = std::remove_cvref_t<decltype(obj.*pairs.second)>;
          set_json_value<FieldType>(value, obj.*pairs.second, allocator);

          doc.AddMember(key, value, allocator);
        }()
          ), ...);
    },
    fields_tuple
  );

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);

  return buffer.GetString();
}

// Deserialize JSON string to struct
template <typename T>
bool deserialize_from_json(T& obj, const std::string& json_str)
{
  auto [class_name, fields_tuple] = reflection_fields<T>();

  rapidjson::Document doc;
  doc.Parse(json_str.c_str());

  if (doc.HasParseError())
    return false;

  if (!doc.IsObject())
    return false;

  std::apply([&](const auto&... pairs)
    {
      ((
        [&]()
        {
          if (doc.HasMember(pairs.first))
          {
            using FieldType = std::remove_cvref_t<decltype(obj.*pairs.second)>;
            get_json_value<FieldType>(obj.*pairs.second, doc[pairs.first]);
          }
        }()
          ), ...);
    },
    fields_tuple
  );

  return true;
}