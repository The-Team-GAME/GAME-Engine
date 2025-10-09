#pragma once

#include <string>
#include <type_traits>
#include <memory>
#include <functional>
#include <unordered_map>
#include "Reflect.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

template <typename T>
void serialize_to_json(const T& obj, rapidjson::Value& json_obj, rapidjson::Document::AllocatorType& allocator)
{
  if constexpr (has_adaptor_v<T>)
  {
    auto view = reflection_adaptor<T>::serialize(obj);
    auto fields = reflection_info<typename reflection_adaptor<T>::view_type>::fields;
    std::apply([&](const auto&... pairs) {
      ([&]() {
        rapidjson::Value key(pairs.first, allocator);
        const auto& field_value = view.*pairs.second;
        using FieldType = std::remove_cvref_t<decltype(field_value)>;

        if constexpr (has_reflection_v<FieldType> || has_adaptor_v<FieldType>) {
          rapidjson::Value nested_obj(rapidjson::kObjectType);
          serialize_to_json(field_value, nested_obj, allocator);
          json_obj.AddMember(key, nested_obj, allocator);
        }
        else if constexpr (std::is_same_v<FieldType, std::string>) {
          rapidjson::Value value(field_value.c_str(), allocator);
          json_obj.AddMember(key, value, allocator);
        }
        else if constexpr (std::is_same_v<FieldType, bool>) {
          rapidjson::Value value(field_value);
          json_obj.AddMember(key, value, allocator);
        }
        else if constexpr (std::is_integral_v<FieldType> && std::is_unsigned_v<FieldType>) {
          rapidjson::Value value(static_cast<uint64_t>(field_value));
          json_obj.AddMember(key, value, allocator);
        }
        else if constexpr (std::is_integral_v<FieldType>) {
          rapidjson::Value value(static_cast<int64_t>(field_value));
          json_obj.AddMember(key, value, allocator);
        }
        else if constexpr (std::is_floating_point_v<FieldType>) {
          rapidjson::Value value(static_cast<double>(field_value));
          json_obj.AddMember(key, value, allocator);
        }
        else {
          rapidjson::Value value(field_value);
          json_obj.AddMember(key, value, allocator);
        }
        }(), ...);
      }, fields);
  }
  else if constexpr (has_reflection_v<T>)
  {
    auto fields = reflection_info<T>::fields;
    std::apply([&](const auto&... pairs) {
      ([&]() {
        rapidjson::Value key(pairs.first, allocator);
        const auto& field_value = obj.*pairs.second;
        using FieldType = std::remove_cvref_t<decltype(field_value)>;

        if constexpr (has_reflection_v<FieldType> || has_adaptor_v<FieldType>) {
          rapidjson::Value nested_obj(rapidjson::kObjectType);
          serialize_to_json(field_value, nested_obj, allocator);
          json_obj.AddMember(key, nested_obj, allocator);
        }
        else if constexpr (std::is_same_v<FieldType, std::string>) {
          rapidjson::Value value(field_value.c_str(), allocator);
          json_obj.AddMember(key, value, allocator);
        }
        else if constexpr (std::is_same_v<FieldType, bool>) {
          rapidjson::Value value(field_value);
          json_obj.AddMember(key, value, allocator);
        }
        else if constexpr (std::is_integral_v<FieldType> && std::is_unsigned_v<FieldType>) {
          rapidjson::Value value(static_cast<uint64_t>(field_value));
          json_obj.AddMember(key, value, allocator);
        }
        else if constexpr (std::is_integral_v<FieldType>) {
          rapidjson::Value value(static_cast<int64_t>(field_value));
          json_obj.AddMember(key, value, allocator);
        }
        else if constexpr (std::is_floating_point_v<FieldType>) {
          rapidjson::Value value(static_cast<double>(field_value));
          json_obj.AddMember(key, value, allocator);
        }
        else {
          rapidjson::Value value(field_value);
          json_obj.AddMember(key, value, allocator);
        }
        }(), ...);
      }, fields);
  }
}

template <typename T>
std::string to_json(const T& obj, bool pretty = true)
{
  rapidjson::Document doc;
  doc.SetObject();
  auto& allocator = doc.GetAllocator();

  serialize_to_json(obj, doc, allocator);

  rapidjson::StringBuffer buffer;
  if (pretty) {
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
  }
  else {
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
  }

  return buffer.GetString();
}

template <typename T>
void deserialize_from_json(T& obj, const rapidjson::Value& json_obj)
{
  if constexpr (has_adaptor_v<T>)
  {
    typename reflection_adaptor<T>::view_type view;
    auto fields = reflection_info<typename reflection_adaptor<T>::view_type>::fields;

    std::apply([&](const auto&... pairs) {
      ([&]() {
        if (json_obj.HasMember(pairs.first)) {
          auto& field_ref = view.*pairs.second;
          using FieldType = std::remove_cvref_t<decltype(field_ref)>;

          if constexpr (has_reflection_v<FieldType> || has_adaptor_v<FieldType>) {
            deserialize_from_json(field_ref, json_obj[pairs.first]);
          }
          else if constexpr (std::is_same_v<FieldType, std::string>) {
            field_ref = json_obj[pairs.first].GetString();
          }
          else if constexpr (std::is_same_v<FieldType, bool>) {
            field_ref = json_obj[pairs.first].GetBool();
          }
          else if constexpr (std::is_integral_v<FieldType> && std::is_unsigned_v<FieldType>) {
            field_ref = static_cast<FieldType>(json_obj[pairs.first].GetUint64());
          }
          else if constexpr (std::is_integral_v<FieldType>) {
            field_ref = static_cast<FieldType>(json_obj[pairs.first].GetInt64());
          }
          else if constexpr (std::is_floating_point_v<FieldType>) {
            field_ref = static_cast<FieldType>(json_obj[pairs.first].GetDouble());
          }
        }
        }(), ...);
      }, fields);

    obj = reflection_adaptor<T>::deserialize(view);
  }
  else if constexpr (has_reflection_v<T>)
  {
    auto fields = reflection_info<T>::fields;
    std::apply([&](const auto&... pairs) {
      ([&]() {
        if (json_obj.HasMember(pairs.first)) {
          auto& field_ref = obj.*pairs.second;
          using FieldType = std::remove_cvref_t<decltype(field_ref)>;

          if constexpr (has_reflection_v<FieldType> || has_adaptor_v<FieldType>) {
            deserialize_from_json(field_ref, json_obj[pairs.first]);
          }
          else if constexpr (std::is_same_v<FieldType, std::string>) {
            field_ref = json_obj[pairs.first].GetString();
          }
          else if constexpr (std::is_same_v<FieldType, bool>) {
            field_ref = json_obj[pairs.first].GetBool();
          }
          else if constexpr (std::is_integral_v<FieldType> && std::is_unsigned_v<FieldType>) {
            field_ref = static_cast<FieldType>(json_obj[pairs.first].GetUint64());
          }
          else if constexpr (std::is_integral_v<FieldType>) {
            field_ref = static_cast<FieldType>(json_obj[pairs.first].GetInt64());
          }
          else if constexpr (std::is_floating_point_v<FieldType>) {
            field_ref = static_cast<FieldType>(json_obj[pairs.first].GetDouble());
          }
        }
        }(), ...);
      }, fields);
  }
}

template <typename T>
T from_json(const std::string& json_str)
{
  rapidjson::Document doc;
  doc.Parse(json_str.c_str());

  if (doc.HasParseError()) {
    throw std::runtime_error("JSON parse error");
  }

  T obj{};
  deserialize_from_json(obj, doc);
  return obj;
}

// Type-erased serialization interface
class TypeErasedSerializer {
public:
  virtual ~TypeErasedSerializer() = default;
  virtual void serialize(const void* obj, rapidjson::Value& json_obj, rapidjson::Document::AllocatorType& allocator) const = 0;
  virtual void deserialize(void* obj, const rapidjson::Value& json_obj) const = 0;
  virtual std::string get_type_name() const = 0;
};

// Concrete implementation for specific types
template<typename T>
class TypedSerializer : public TypeErasedSerializer {
public:
  void serialize(const void* obj, rapidjson::Value& json_obj, rapidjson::Document::AllocatorType& allocator) const override {
    const T* typed_obj = static_cast<const T*>(obj);
    serialize_to_json(*typed_obj, json_obj, allocator);
  }

  void deserialize(void* obj, const rapidjson::Value& json_obj) const override {
    T* typed_obj = static_cast<T*>(obj);
    deserialize_from_json(*typed_obj, json_obj);
  }

  std::string get_type_name() const override {
    return get_class_name<T>();
  }
};

// Type registry for runtime type resolution
class TypeRegistry {
public:
  static TypeRegistry& instance() {
    static TypeRegistry registry;
    return registry;
  }

  template<typename T>
  void register_type(const std::string& type_name) {
    serializers_[type_name] = std::make_unique<TypedSerializer<T>>();
  }

  const TypeErasedSerializer* get_serializer(const std::string& type_name) const {
    auto it = serializers_.find(type_name);
    return it != serializers_.end() ? it->second.get() : nullptr;
  }

private:
  std::unordered_map<std::string, std::unique_ptr<TypeErasedSerializer>> serializers_;
};

// Type-erased serialization functions
inline std::string to_json_erased(const void* obj, const std::string& type_name, bool pretty = true) {
  const auto* serializer = TypeRegistry::instance().get_serializer(type_name);
  if (!serializer) {
    throw std::runtime_error("Type not registered: " + type_name);
  }

  rapidjson::Document doc;
  doc.SetObject();
  auto& allocator = doc.GetAllocator();

  // Add type metadata
  rapidjson::Value type_value(type_name.c_str(), allocator);
  doc.AddMember("__type", type_value, allocator);

  // Serialize data
  rapidjson::Value data_obj(rapidjson::kObjectType);
  serializer->serialize(obj, data_obj, allocator);
  doc.AddMember("data", data_obj, allocator);

  rapidjson::StringBuffer buffer;
  if (pretty) {
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
  }
  else {
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
  }

  return buffer.GetString();
}

inline void from_json_erased(void* obj, const std::string& type_name, const std::string& json_str) {
  const auto* serializer = TypeRegistry::instance().get_serializer(type_name);
  if (!serializer) {
    throw std::runtime_error("Type not registered: " + type_name);
  }

  rapidjson::Document doc;
  doc.Parse(json_str.c_str());

  if (doc.HasParseError()) {
    throw std::runtime_error("JSON parse error");
  }

  if (!doc.HasMember("data")) {
    throw std::runtime_error("Invalid JSON format: missing 'data' field");
  }

  serializer->deserialize(obj, doc["data"]);
}

// Helper to automatically deserialize with type detection
template<typename T>
T from_json_auto(const std::string& json_str) {
  rapidjson::Document doc;
  doc.Parse(json_str.c_str());

  if (doc.HasParseError()) {
    throw std::runtime_error("JSON parse error");
  }

  // Check if type metadata exists
  if (doc.HasMember("__type") && doc.HasMember("data")) {
    T obj{};
    from_json_erased(&obj, get_class_name<T>(), json_str);
    return obj;
  }
  else {
    // Fallback to direct deserialization
    return from_json<T>(json_str);
  }
}


// // Macro to auto-register types
// #define REGISTER_TYPE(TYPE) \
//     namespace { \
//         struct TYPE##_Registrar { \
//             TYPE##_Registrar() { \
//                 TypeRegistry::instance().register_type<TYPE>(get_class_name<TYPE>()); \
//             } \
//         }; \
//         static TYPE##_Registrar TYPE##_registrar_instance; \
//     }




// Unified macro that works for both regular and namespaced types
#define REGISTER_TYPE(TYPE) \
    namespace { \
        struct CONCAT(CONCAT(TypeRegistrar_, __LINE__), _) { \
            CONCAT(CONCAT(TypeRegistrar_, __LINE__), _)() { \
                TypeRegistry::instance().register_type<TYPE>(get_class_name<TYPE>()); \
            } \
        }; \
        static CONCAT(CONCAT(TypeRegistrar_, __LINE__), _) CONCAT(CONCAT(type_registrar_instance_, __LINE__), _); \
    }


