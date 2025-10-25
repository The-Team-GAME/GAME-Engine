#pragma once

#include "GenericValue.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

namespace ReflectionLibrary
{
  // GenericValueProxy implementation
  inline GenericValueProxy::GenericValueProxy(GenericValue* parent, const std::string& key, std::vector<std::string> path)
    : parent_(parent), key_(key), index_(0), is_object_access_(true), path_(std::move(path))
  {
    path_.push_back(key);
  }

  inline GenericValueProxy::GenericValueProxy(GenericValue* parent, size_t index, std::vector<std::string> path)
    : parent_(parent), key_(), index_(index), is_object_access_(false), path_(std::move(path))
  {
    path_.push_back("[" + std::to_string(index) + "]");
  }

  inline std::string GenericValueProxy::build_path_string() const
  {
    std::ostringstream oss;
    for (size_t i = 0; i < path_.size(); ++i)
    {
      if (i > 0 && path_[i][0] != '[')
        oss << ".";
      oss << path_[i];
    }
    return oss.str();
  }

  inline void GenericValueProxy::ensure_writable()
  {
    if (!parent_)
      throw std::runtime_error("Invalid proxy: null parent");

    if (is_object_access_)
    {
      if (!parent_->isObject() && !parent_->isNull())
      {
        throw std::runtime_error("Cannot access key '" + key_ + "' on non-object type at: " + build_path_string());
      }
      if (parent_->isNull())
      {
        parent_->data_ = Object{};
      }
    }
    else
    {
      if (!parent_->isArray() && !parent_->isNull())
      {
        throw std::runtime_error("Cannot index non-array type at: " + build_path_string());
      }
      if (parent_->isNull())
      {
        // Auto-create array and resize to accommodate the index
        parent_->data_ = Array{};
      }
      // Resize array if necessary
      auto& arr = parent_->getArray();
      if (index_ >= arr.size())
      {
        arr.resize(index_ + 1);
      }
    }
  }

  inline GenericValue& GenericValueProxy::get_writable()
  {
    ensure_writable();
    if (is_object_access_)
    {
      return parent_->getObject()[key_];
    }
    else
    {
      return parent_->getArray()[index_];
    }
  }

  inline const GenericValue& GenericValueProxy::get() const
  {
    if (!parent_)
      throw std::runtime_error("Invalid proxy: null parent");

    if (is_object_access_)
    {
      if (!parent_->isObject())
      {
        throw std::runtime_error("Cannot access key '" + key_ + "' on non-object type at: " + build_path_string());
      }
      const auto& obj = parent_->getObject();
      auto it = obj.find(key_);
      if (it == obj.end())
      {
        throw std::out_of_range("Key '" + key_ + "' not found at: " + build_path_string());
      }
      return it->second;
    }
    else
    {
      if (!parent_->isArray())
      {
        throw std::runtime_error("Cannot index non-array type at: " + build_path_string());
      }
      const auto& arr = parent_->getArray();
      if (index_ >= arr.size())
      {
        throw std::out_of_range("Array index " + std::to_string(index_) + " out of bounds at: " + build_path_string());
      }
      return arr[index_];
    }
  }

  inline GenericValueProxy::operator GenericValue() const
  {
    return get();
  }

  // Assignment operators
  inline GenericValueProxy& GenericValueProxy::operator=(const GenericValue& val)
  {
    get_writable() = val;
    return *this;
  }

  inline GenericValueProxy& GenericValueProxy::operator=(GenericValue&& val)
  {
    get_writable() = std::move(val);
    return *this;
  }

  inline GenericValueProxy& GenericValueProxy::operator=(bool b)
  {
    get_writable() = GenericValue(b);
    return *this;
  }

  inline GenericValueProxy& GenericValueProxy::operator=(int i)
  {
    get_writable() = GenericValue(i);
    return *this;
  }

  inline GenericValueProxy& GenericValueProxy::operator=(int64_t i)
  {
    get_writable() = GenericValue(i);
    return *this;
  }

  inline GenericValueProxy& GenericValueProxy::operator=(unsigned int i)
  {
    get_writable() = GenericValue(i);
    return *this;
  }

  inline GenericValueProxy& GenericValueProxy::operator=(double d)
  {
    get_writable() = GenericValue(d);
    return *this;
  }

  inline GenericValueProxy& GenericValueProxy::operator=(const char* s)
  {
    get_writable() = GenericValue(s);
    return *this;
  }

  inline GenericValueProxy& GenericValueProxy::operator=(const std::string& s)
  {
    get_writable() = GenericValue(s);
    return *this;
  }

  inline GenericValueProxy& GenericValueProxy::operator=(const Array& a)
  {
    get_writable() = GenericValue(a);
    return *this;
  }

  inline GenericValueProxy& GenericValueProxy::operator=(const Object& o)
  {
    get_writable() = GenericValue(o);
    return *this;
  }

  // Chaining
  inline GenericValueProxy GenericValueProxy::operator[](const std::string& key)
  {
    GenericValue& val = get_writable();
    return GenericValueProxy(&val, key, path_);
  }

  inline GenericValueProxy GenericValueProxy::operator[](const char* key)
  {
    return (*this)[std::string(key)];
  }

  inline GenericValueProxy GenericValueProxy::operator[](size_t index)
  {
    GenericValue& val = get_writable();
    return GenericValueProxy(&val, index, path_);
  }

  inline GenericValueProxy GenericValueProxy::operator[](int index)
  {
    return (*this)[static_cast<size_t>(index)];
  }

  // Type checking through proxy
  inline bool GenericValueProxy::isNull() const { return get().isNull(); }
  inline bool GenericValueProxy::isBool() const { return get().isBool(); }
  inline bool GenericValueProxy::isInt() const { return get().isInt(); }
  inline bool GenericValueProxy::isDouble() const { return get().isDouble(); }
  inline bool GenericValueProxy::isString() const { return get().isString(); }
  inline bool GenericValueProxy::isArray() const { return get().isArray(); }
  inline bool GenericValueProxy::isObject() const { return get().isObject(); }

  // Getters through proxy
  inline Boolean GenericValueProxy::getBool() const { return get().getBool(); }
  inline Integer GenericValueProxy::getInt() const { return get().getInt(); }
  inline Double GenericValueProxy::getDouble() const { return get().getDouble(); }
  inline const String& GenericValueProxy::getString() const { return get().getString(); }
  inline const Array& GenericValueProxy::getArray() const { return get().getArray(); }
  inline Array& GenericValueProxy::getArray() { return get_writable().getArray(); }
  inline const Object& GenericValueProxy::getObject() const { return get().getObject(); }
  inline Object& GenericValueProxy::getObject() { return get_writable().getObject(); }

  inline size_t GenericValueProxy::size() const { return get().size(); }
  inline bool GenericValueProxy::has(const std::string& key) const { return get().has(key); }
  inline void GenericValueProxy::push_back(const GenericValue& val) { get_writable().push_back(val); }
  inline void GenericValueProxy::push_back(GenericValue&& val) { get_writable().push_back(std::move(val)); }

  // GenericValue implementation
  inline GenericValue::GenericValue() : data_(Null{}) {}
  inline GenericValue::GenericValue(bool b) : data_(b) {}
  inline GenericValue::GenericValue(int i) : data_(static_cast<Integer>(i)) {}
  inline GenericValue::GenericValue(int64_t i) : data_(i) {}
  inline GenericValue::GenericValue(unsigned int i) : data_(static_cast<Integer>(i)) {}
  inline GenericValue::GenericValue(double d) : data_(d) {}
  inline GenericValue::GenericValue(const char* s) : data_(String(s)) {}
  inline GenericValue::GenericValue(const String& s) : data_(s) {}
  inline GenericValue::GenericValue(String&& s) : data_(std::move(s)) {}
  inline GenericValue::GenericValue(const Array& a) : data_(a) {}
  inline GenericValue::GenericValue(Array&& a) : data_(std::move(a)) {}
  inline GenericValue::GenericValue(const Object& o) : data_(o) {}
  inline GenericValue::GenericValue(Object&& o) : data_(std::move(o)) {}

  inline bool GenericValue::isNull() const { return std::holds_alternative<Null>(data_); }
  inline bool GenericValue::isBool() const { return std::holds_alternative<Boolean>(data_); }
  inline bool GenericValue::isInt() const { return std::holds_alternative<Integer>(data_); }
  inline bool GenericValue::isDouble() const { return std::holds_alternative<Double>(data_); }
  inline bool GenericValue::isString() const { return std::holds_alternative<String>(data_); }
  inline bool GenericValue::isArray() const { return std::holds_alternative<Array>(data_); }
  inline bool GenericValue::isObject() const { return std::holds_alternative<Object>(data_); }

  inline Boolean GenericValue::getBool() const
  {
    if (!isBool())
      throw std::runtime_error("GenericValue is not a boolean");
    return std::get<Boolean>(data_);
  }

  inline Integer GenericValue::getInt() const
  {
    if (!isInt())
      throw std::runtime_error("GenericValue is not an integer");
    return std::get<Integer>(data_);
  }

  inline Double GenericValue::getDouble() const
  {
    if (!isDouble())
      throw std::runtime_error("GenericValue is not a double");
    return std::get<Double>(data_);
  }

  inline const String& GenericValue::getString() const
  {
    if (!isString())
      throw std::runtime_error("GenericValue is not a string");
    return std::get<String>(data_);
  }

  inline String& GenericValue::getString()
  {
    if (!isString())
      throw std::runtime_error("GenericValue is not a string");
    return std::get<String>(data_);
  }

  inline const Array& GenericValue::getArray() const
  {
    if (!isArray())
      throw std::runtime_error("GenericValue is not an array");
    return std::get<Array>(data_);
  }

  inline Array& GenericValue::getArray()
  {
    if (!isArray())
      throw std::runtime_error("GenericValue is not an array");
    return std::get<Array>(data_);
  }

  inline const Object& GenericValue::getObject() const
  {
    if (!isObject())
      throw std::runtime_error("GenericValue is not an object");
    return std::get<Object>(data_);
  }

  inline Object& GenericValue::getObject()
  {
    if (!isObject())
      throw std::runtime_error("GenericValue is not an object");
    return std::get<Object>(data_);
  }

  inline GenericValueProxy GenericValue::operator[](const std::string& key)
  {
    return GenericValueProxy(this, key);
  }

  inline GenericValueProxy GenericValue::operator[](const char* key)
  {
    return GenericValueProxy(this, std::string(key));
  }

  inline GenericValueProxy GenericValue::operator[](size_t index)
  {
    return GenericValueProxy(this, index);
  }

  inline GenericValueProxy GenericValue::operator[](int index)
  {
    return GenericValueProxy(this, static_cast<size_t>(index));
  }

  inline const GenericValue& GenericValue::operator[](const std::string& key) const
  {
    if (!isObject())
    {
      throw std::runtime_error("Cannot access key on non-object type");
    }
    const auto& obj = getObject();
    auto it = obj.find(key);
    if (it == obj.end())
    {
      throw std::out_of_range("Key not found: " + key);
    }
    return it->second;
  }

  inline const GenericValue& GenericValue::operator[](const char* key) const
  {
    return (*this)[std::string(key)];
  }

  inline const GenericValue& GenericValue::operator[](size_t index) const
  {
    if (!isArray())
    {
      throw std::runtime_error("Cannot index non-array type");
    }
    const auto& arr = getArray();
    if (index >= arr.size())
    {
      throw std::out_of_range("Array index out of bounds");
    }
    return arr[index];
  }

  inline const GenericValue& GenericValue::operator[](int index) const
  {
    return (*this)[static_cast<size_t>(index)];
  }

  inline bool GenericValue::has(const std::string& key) const
  {
    if (!isObject())
      return false;
    return getObject().find(key) != getObject().end();
  }

  inline size_t GenericValue::size() const
  {
    if (isArray())
      return getArray().size();
    if (isObject())
      return getObject().size();
    if (isString())
      return getString().size();
    return 0;
  }

  inline void GenericValue::push_back(const GenericValue& val)
  {
    if (!isArray())
    {
      data_ = Array{};
    }
    getArray().push_back(val);
  }

  inline void GenericValue::push_back(GenericValue&& val)
  {
    if (!isArray())
    {
      data_ = Array{};
    }
    getArray().push_back(std::move(val));
  }

  inline const GenericValue::VariantType& GenericValue::variant() const { return data_; }
  inline GenericValue::VariantType& GenericValue::variant() { return data_; }

  // Convert any C++ type to GenericValue
  template <typename T>
  inline GenericValue to_generic_value(const T& obj)
  {
    using ValueType = std::decay_t<T>;

    if constexpr (std::is_same_v<ValueType, bool>)
    {
      return GenericValue(obj);
    }
    else if constexpr (std::is_integral_v<ValueType>)
    {
      return GenericValue(static_cast<int64_t>(obj));
    }
    else if constexpr (std::is_floating_point_v<ValueType>)
    {
      return GenericValue(static_cast<double>(obj));
    }
    else if constexpr (std::is_same_v<ValueType, std::string> ||
      std::is_same_v<ValueType, const char*> ||
      std::is_same_v<ValueType, char*>)
    {
      return GenericValue(String(obj));
    }
    else if constexpr (is_pair_v<ValueType>)
    {
      Object result;
      result["first"] = to_generic_value(obj.first);
      result["second"] = to_generic_value(obj.second);
      return GenericValue(std::move(result));
    }
    else if constexpr (is_tuple_v<ValueType>)
    {
      Array result;
      std::apply([&](const auto &...args)
        { ((result.push_back(to_generic_value(args))), ...); },
        obj);
      return GenericValue(std::move(result));
    }
    else if constexpr (is_iterable_v<ValueType>)
    {
      // Handle maps specially
      if constexpr (requires { typename ValueType::key_type; typename ValueType::mapped_type; })
      {
        Array result;
        for (const auto& [key, val] : obj)
        {
          Object pair;
          pair["first"] = to_generic_value(key);
          pair["second"] = to_generic_value(val);
          result.push_back(GenericValue(std::move(pair)));
        }
        return GenericValue(std::move(result));
      }
      else
      {
        // Sequential containers
        Array result;
        for (const auto& elem : obj)
        {
          result.push_back(to_generic_value(elem));
        }
        return GenericValue(std::move(result));
      }
    }
    else if constexpr (is_reflectable_v<ValueType>)
    {
      Object result;
      auto& obj_ref = const_cast<ValueType&>(obj);
      for_each_field(obj_ref, [&](std::string_view name, auto& field_value, int)
        { result[std::string(name)] = to_generic_value(field_value); });
      return GenericValue(std::move(result));
    }
    else
    {
      // Fallback for unsupported types
      return GenericValue();
    }
  }

  // Convert GenericValue back to C++ type
  template <typename T>
  inline bool from_generic_value(T& obj, const GenericValue& value)
  {
    using ValueType = std::decay_t<T>;

    if constexpr (std::is_same_v<ValueType, bool>)
    {
      if (!value.isBool())
        return false;
      obj = value.getBool();
      return true;
    }
    else if constexpr (std::is_integral_v<ValueType>)
    {
      if (!value.isInt())
        return false;
      obj = static_cast<ValueType>(value.getInt());
      return true;
    }
    else if constexpr (std::is_floating_point_v<ValueType>)
    {
      if (value.isDouble())
        obj = static_cast<ValueType>(value.getDouble());
      else if (value.isInt())
        obj = static_cast<ValueType>(value.getInt());
      else
        return false;
      return true;
    }
    else if constexpr (std::is_same_v<ValueType, std::string>)
    {
      if (!value.isString())
        return false;
      obj = value.getString();
      return true;
    }
    else if constexpr (is_pair_v<ValueType>)
    {
      if (!value.isObject())
        return false;
      if (value.has("first") && !from_generic_value(obj.first, value["first"]))
        return false;
      if (value.has("second") && !from_generic_value(obj.second, value["second"]))
        return false;
      return true;
    }
    else if constexpr (is_tuple_v<ValueType>)
    {
      if (!value.isArray())
        return false;
      const auto& arr = value.getArray();
      size_t idx = 0;
      bool success = true;
      std::apply([&](auto &...args)
        { (([&]()
          {
            if (idx < arr.size() && success)
              success = from_generic_value(args, arr[idx]);
            ++idx; }()),
          ...); },
        obj);
      return success;
    }
    else if constexpr (is_iterable_v<ValueType>)
    {
      if (!value.isArray())
        return false;
      const auto& arr = value.getArray();
      obj.clear();

      // Handle maps differently from vectors/lists
      if constexpr (requires { typename ValueType::key_type; typename ValueType::mapped_type; })
      {
        for (const auto& elem : arr)
        {
          std::pair<typename ValueType::key_type, typename ValueType::mapped_type> pair{};
          if (!from_generic_value(pair, elem))
            return false;
          obj.insert(std::move(pair));
        }
      }
      else
      {
        for (const auto& elem : arr)
        {
          typename ValueType::value_type val{};
          if (!from_generic_value(val, elem))
            return false;
          obj.insert(obj.end(), std::move(val));
        }
      }
      return true;
    }
    else if constexpr (is_reflectable_v<ValueType>)
    {
      if (!value.isObject())
        return false;
      bool success = true;
      for_each_field(obj, [&](std::string_view name, auto& field_value, int)
        {
          std::string key(name);
          if (value.has(key))
          {
            if (!from_generic_value(field_value, value[key]))
              success = false;
          } });
          return success;
    }
    else
    {
      return false;
    }
  }

  // Convenience function to convert from GenericValue
  template <typename T>
  inline std::optional<T> from_generic_value(const GenericValue& value)
  {
    T result{};
    if (!from_generic_value(result, value))
    {
      return std::nullopt;
    }
    return result;
  }



} // namespace ReflectionLibrary