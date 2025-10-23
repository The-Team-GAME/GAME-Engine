#pragma once

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <string>
#include <type_traits>
#include <optional>
#include "GenericValue.h"

namespace ReflectionLibrary
{


  // Helper function to convert GenericValue to RapidJSON Value
  static void generic_value_to_rapidjson(const GenericValue& value, rapidjson::Value& json_val, rapidjson::Document::AllocatorType& allocator)
  {
    if (value.isNull())
    {
      json_val.SetNull();
    }
    else if (value.isBool())
    {
      json_val.SetBool(value.getBool());
    }
    else if (value.isInt())
    {
      json_val.SetInt64(value.getInt());
    }
    else if (value.isDouble())
    {
      json_val.SetDouble(value.getDouble());
    }
    else if (value.isString())
    {
      const auto& str = value.getString();
      json_val.SetString(str.c_str(), static_cast<rapidjson::SizeType>(str.length()), allocator);
    }
    else if (value.isArray())
    {
      json_val.SetArray();
      const auto& arr = value.getArray();
      for (const auto& elem : arr)
      {
        rapidjson::Value elem_val;
        generic_value_to_rapidjson(elem, elem_val, allocator);
        json_val.PushBack(elem_val, allocator);
      }
    }
    else if (value.isObject())
    {
      json_val.SetObject();
      const auto& obj = value.getObject();
      for (const auto& [key, val] : obj)
      {
        rapidjson::Value key_val(key.c_str(), static_cast<rapidjson::SizeType>(key.length()), allocator);
        rapidjson::Value val_json;
        generic_value_to_rapidjson(val, val_json, allocator);
        json_val.AddMember(key_val, val_json, allocator);
      }
    }
  }

  // Helper function to convert RapidJSON Value to GenericValue
  static GenericValue rapidjson_value_to_generic_value(const rapidjson::Value& json_val)
  {
    if (json_val.IsNull())
    {
      return GenericValue();
    }
    else if (json_val.IsBool())
    {
      return GenericValue(json_val.GetBool());
    }
    else if (json_val.IsInt64())
    {
      return GenericValue(json_val.GetInt64());
    }
    else if (json_val.IsInt())
    {
      return GenericValue(static_cast<int64_t>(json_val.GetInt()));
    }
    else if (json_val.IsUint64())
    {
      return GenericValue(static_cast<int64_t>(json_val.GetUint64()));
    }
    else if (json_val.IsUint())
    {
      return GenericValue(static_cast<int64_t>(json_val.GetUint()));
    }
    else if (json_val.IsDouble())
    {
      return GenericValue(json_val.GetDouble());
    }
    else if (json_val.IsString())
    {
      return GenericValue(std::string(json_val.GetString()));
    }
    else if (json_val.IsArray())
    {
      Array arr;
      for (auto it = json_val.Begin(); it != json_val.End(); ++it)
      {
        arr.push_back(rapidjson_value_to_generic_value(*it));
      }
      return GenericValue(std::move(arr));
    }
    else if (json_val.IsObject())
    {
      Object obj;
      for (auto it = json_val.MemberBegin(); it != json_val.MemberEnd(); ++it)
      {
        obj[it->name.GetString()] = rapidjson_value_to_generic_value(it->value);
      }
      return GenericValue(std::move(obj));
    }

    return GenericValue();
  }

  static std::string generic_value_to_json(const GenericValue& value, bool pretty)
  {
    rapidjson::Document doc;
    auto& allocator = doc.GetAllocator();

    generic_value_to_rapidjson(value, doc, allocator);

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

  static std::optional<GenericValue> generic_value_from_json(const std::string& json_str)
  {
    rapidjson::Document doc;
    doc.Parse(json_str.c_str());

    if (doc.HasParseError())
    {
      return std::nullopt;
    }

    return rapidjson_value_to_generic_value(doc);
  }

  // Serialize object to JSON string
  template <typename T>
  static std::string to_json(T& obj, bool pretty = false)
  {
    GenericValue generic = to_generic_value(obj);
    return generic_value_to_json(generic, pretty);
  }

  // Deserialize object from JSON string
  template <typename T>
  static std::optional<T> from_json(const std::string& json_str)
  {
    std::optional<GenericValue> generic = generic_value_from_json(json_str);
    if (!generic)
      return std::nullopt;

    return from_generic_value<T>(generic.value());
  }

} // namespace ReflectionLibrary