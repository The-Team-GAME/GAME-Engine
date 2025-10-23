#pragma once

#include <variant>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <type_traits>
#include <sstream>
#include <optional>
#include "Concepts.h"
#include "Helpers.h"

namespace ReflectionLibrary
{
  // Forward declarations
  class GenericValue;
  class GenericValueProxy;

  // Type aliases
  using Null = std::monostate;
  using Boolean = bool;
  using Integer = int64_t;
  using Double = double;
  using String = std::string;
  using Array = std::vector<GenericValue>;
  using Object = std::map<std::string, GenericValue>;

  // Proxy class for lazy evaluation and implicit conversions
  class GenericValueProxy
  {
  private:
    GenericValue* parent_;
    std::string key_;
    size_t index_;
    bool is_object_access_;
    std::vector<std::string> path_; // For better error messages

  public:
    GenericValueProxy(GenericValue* parent, const std::string& key, std::vector<std::string> path = {});
    GenericValueProxy(GenericValue* parent, size_t index, std::vector<std::string> path = {});

    // Conversion to GenericValue for reading
    operator GenericValue() const;
    const GenericValue& get() const;

    // Assignment operators for writing (implicit conversions)
    GenericValueProxy& operator=(const GenericValue& val);
    GenericValueProxy& operator=(GenericValue&& val);
    GenericValueProxy& operator=(bool b);
    GenericValueProxy& operator=(int i);
    GenericValueProxy& operator=(int64_t i);
    GenericValueProxy& operator=(unsigned int i);
    GenericValueProxy& operator=(double d);
    GenericValueProxy& operator=(const char* s);
    GenericValueProxy& operator=(const std::string& s);
    GenericValueProxy& operator=(const Array& a);
    GenericValueProxy& operator=(const Object& o);

    // Chaining - object access
    GenericValueProxy operator[](const std::string& key);
    GenericValueProxy operator[](const char* key);

    // Chaining - array access
    GenericValueProxy operator[](size_t index);
    GenericValueProxy operator[](int index);

    // Type checking
    bool isNull() const;
    bool isBool() const;
    bool isInt() const;
    bool isDouble() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    // Getters
    Boolean getBool() const;
    Integer getInt() const;
    Double getDouble() const;
    const String& getString() const;
    const Array& getArray() const;
    Array& getArray();
    const Object& getObject() const;
    Object& getObject();

    // Utility
    size_t size() const;
    bool has(const std::string& key) const;
    void push_back(const GenericValue& val);
    void push_back(GenericValue&& val);

  private:
    void ensure_writable();
    GenericValue& get_writable();
    std::string build_path_string() const;
  };

  // Main GenericValue class for dynamic type storage
  class GenericValue
  {
  public:
    using VariantType = std::variant<Null, Boolean, Integer, Double, String, Array, Object>;

  private:
    VariantType data_;

  public:
    // Constructors
    GenericValue();
    GenericValue(bool b);
    GenericValue(int i);
    GenericValue(int64_t i);
    GenericValue(unsigned int i);
    GenericValue(double d);
    GenericValue(const char* s);
    GenericValue(const String& s);
    GenericValue(String&& s);
    GenericValue(const Array& a);
    GenericValue(Array&& a);
    GenericValue(const Object& o);
    GenericValue(Object&& o);

    // Type checking
    bool isNull() const;
    bool isBool() const;
    bool isInt() const;
    bool isDouble() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    // Getters with type checking
    Boolean getBool() const;
    Integer getInt() const;
    Double getDouble() const;
    const String& getString() const;
    String& getString();
    const Array& getArray() const;
    Array& getArray();
    const Object& getObject() const;
    Object& getObject();

    // Proxy-based access - returns proxy for lazy evaluation
    GenericValueProxy operator[](const std::string& key);
    GenericValueProxy operator[](const char* key);
    GenericValueProxy operator[](size_t index);
    GenericValueProxy operator[](int index);

    // Const versions for read-only access
    const GenericValue& operator[](const std::string& key) const;
    const GenericValue& operator[](const char* key) const;
    const GenericValue& operator[](size_t index) const;
    const GenericValue& operator[](int index) const;

    // Check if key exists
    bool has(const std::string& key) const;

    // Get array/object/string size
    size_t size() const;

    // Push back for arrays
    void push_back(const GenericValue& val);
    void push_back(GenericValue&& val);

    // Get underlying variant for advanced use
    const VariantType& variant() const;
    VariantType& variant();

    // Friend class for internal access
    friend class GenericValueProxy;
  };

  // Convert any C++ type to GenericValue
  template <typename T>
  GenericValue to_generic_value(const T& obj);

  // Convert GenericValue back to C++ type
  template <typename T>
  bool from_generic_value(T& obj, const GenericValue& value);

  // Convenience function to convert from GenericValue
  template <typename T>
  std::optional<T> from_generic_value(const GenericValue& value);

} // namespace ReflectionLibrary

#include "GenericValue.inl"