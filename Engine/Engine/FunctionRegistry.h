//-----------------------------------------------------------------------------------
// file:    FunctionRegistry.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Generic function registry system for storing and retrieving callbacks
//          by string names. Enables serialization of function pointers by using
//          named lookups. Supports any function signature through template parameters.
//          Used throughout the engine for state machines, event callbacks, and more.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <functional>
#include <unordered_map>
#include <string>
#include <typeindex>
#include <any>
#include "Logger.h"
#include <vector>

namespace Engine {

  // Forward declaration
  template<typename FuncSignature>
  struct FunctionRegistry;

  // ============================================================================
  // Serializable Function Wrapper
  // ============================================================================

  /// Type-safe wrapper for serializable function names
  /// Stores just the function name as a string for serialization
  struct SerializableFunction {
    std::string name;

    SerializableFunction() = default;

    SerializableFunction(const std::string& function_name)
      : name(function_name)
    {
    }

    SerializableFunction(const char* function_name)
      : name(function_name)
    {
    }

    // Implicit conversion to string for backwards compatibility
    operator const std::string& () const {
      return name;
    }

    // Assignment operators
    SerializableFunction& operator=(const std::string& function_name) {
      name = function_name;
      return *this;
    }

    SerializableFunction& operator=(const char* function_name) {
      name = function_name;
      return *this;
    }

    // Comparison operators
    bool operator==(const SerializableFunction& other) const {
      return name == other.name;
    }

    bool operator==(const std::string& other) const {
      return name == other;
    }

    bool operator!=(const SerializableFunction& other) const {
      return name != other.name;
    }

    bool operator!=(const std::string& other) const {
      return name != other;
    }

    // Check if function name is set
    bool empty() const {
      return name.empty();
    }

    // Clear the function name
    void clear() {
      name.clear();
    }

    // Get function with explicit signature (implemented after FunctionRegistry)
    template<typename Signature>
    std::function<Signature> get() const;

    // Check if function exists with explicit signature (implemented after FunctionRegistry)
    template<typename Signature>
    bool exists() const;
  };

  // ============================================================================
  // Generic Function Registry
  // ============================================================================

  /// Generic registry for storing functions with string names
  /// Template parameter is the function signature (e.g., void(int, float))
  template<typename FuncSignature>
  struct FunctionRegistry {
    using FunctionType = std::function<FuncSignature>;

    // Static storage for registered functions
    static std::unordered_map<std::string, FunctionType>& get_functions() {
      static std::unordered_map<std::string, FunctionType> functions;
      return functions;
    }

    /// Register a function with a string name
    static void register_function(const std::string& name, FunctionType func) {
      auto& functions = get_functions();
      if (functions.find(name) != functions.end()) {
        Log::warning("FunctionRegistry: Overwriting existing function '{}'", name);
      }
      functions[name] = func;
    }

    /// Get a function by name (returns empty std::function if not found)
    static FunctionType get_function(const std::string& name) {
      auto& functions = get_functions();
      auto it = functions.find(name);
      return (it != functions.end()) ? it->second : FunctionType{};
    }

    /// Check if a function with this name exists
    static bool has_function(const std::string& name) {
      auto& functions = get_functions();
      return functions.find(name) != functions.end();
    }

    /// Unregister a function
    static void unregister_function(const std::string& name) {
      auto& functions = get_functions();
      functions.erase(name);
    }

    /// Clear all registered functions
    static void clear() {
      auto& functions = get_functions();
      functions.clear();
    }

    /// Get all registered function names (for debugging/editor)
    static std::vector<std::string> get_all_names() {
      auto& functions = get_functions();
      std::vector<std::string> names;
      names.reserve(functions.size());
      for (const auto& [name, _] : functions) {
        names.push_back(name);
      }
      return names;
    }

    /// Get the number of registered functions
    static size_t size() {
      auto& functions = get_functions();
      return functions.size();
    }
  };

  // ============================================================================
  // SerializableFunction Method Implementations
  // ============================================================================

  template<typename Signature>
  inline std::function<Signature> SerializableFunction::get() const {
    return FunctionRegistry<Signature>::get_function(name);
  }

  template<typename Signature>
  inline bool SerializableFunction::exists() const {
    return FunctionRegistry<Signature>::has_function(name);
  }

  // ============================================================================
  // Multi-Signature Function Registry (Advanced)
  // ============================================================================

  /// Registry that can store multiple function signatures under the same name
  /// Use this when you need type-safe storage of different callback types
  class MultiFunctionRegistry {
  public:
    static MultiFunctionRegistry& instance() {
      static MultiFunctionRegistry registry;
      return registry;
    }

    /// Register a function with a specific signature
    template<typename FuncSignature>
    void register_function(const std::string& name, std::function<FuncSignature> func) {
      auto key = make_key<FuncSignature>(name);
      if (functions_.find(key) != functions_.end()) {
        Log::warning("MultiFunctionRegistry: Overwriting existing function '{}'", name);
      }
      functions_[key] = std::make_any<std::function<FuncSignature>>(func);
    }

    /// Get a function with a specific signature
    template<typename FuncSignature>
    std::function<FuncSignature> get_function(const std::string& name) const {
      auto key = make_key<FuncSignature>(name);
      auto it = functions_.find(key);
      if (it == functions_.end()) {
        return std::function<FuncSignature>{};
      }

      try {
        return std::any_cast<std::function<FuncSignature>>(it->second);
      }
      catch (const std::bad_any_cast&) {
        Log::error("MultiFunctionRegistry: Type mismatch for function '{}'", name);
        return std::function<FuncSignature>{};
      }
    }

    /// Check if a function with this name and signature exists
    template<typename FuncSignature>
    bool has_function(const std::string& name) const {
      auto key = make_key<FuncSignature>(name);
      return functions_.find(key) != functions_.end();
    }

    /// Unregister a function with a specific signature
    template<typename FuncSignature>
    void unregister_function(const std::string& name) {
      auto key = make_key<FuncSignature>(name);
      functions_.erase(key);
    }

    /// Clear all registered functions
    void clear() {
      functions_.clear();
    }

  private:
    template<typename FuncSignature>
    std::string make_key(const std::string& name) const {
      return name + "##" + typeid(FuncSignature).name();
    }

    std::unordered_map<std::string, std::any> functions_;

    MultiFunctionRegistry() = default;
    MultiFunctionRegistry(const MultiFunctionRegistry&) = delete;
    MultiFunctionRegistry& operator=(const MultiFunctionRegistry&) = delete;
  };

  // ============================================================================
  // Function Traits — deduce signature from function pointer type
  // ============================================================================

  template<typename T>
  struct function_traits;

  // Free functions
  template<typename R, typename... Args>
  struct function_traits<R(*)(Args...)> {
    using signature = R(Args...);
  };

  // Member functions
  template<typename R, typename C, typename... Args>
  struct function_traits<R(C::*)(Args...)> {
    using signature = R(Args...);
  };

  template<typename R, typename C, typename... Args>
  struct function_traits<R(C::*)(Args...) const> {
    using signature = R(Args...);
  };

  // ============================================================================
  // Helper Macros for Auto-Registration
  // ============================================================================

  // Token-paste helpers (extra indirection forces __COUNTER__ expansion)
#define _REG_FUNC_CONCAT_IMPL(a, b) a##b
#define _REG_FUNC_CONCAT(a, b) _REG_FUNC_CONCAT_IMPL(a, b)

  /// Register a function at static initialization time.
  /// The registry key is the stringified function reference (e.g. "Namespace::func").
  /// The function signature is auto-deduced — no need to pass it explicitly.
  ///
  /// Usage: REGISTER_FUNCTION(MyNamespace::my_callback)
#define REGISTER_FUNCTION_IMPL(FUNC, ID) \
    namespace { \
      struct _REG_FUNC_CONCAT(AutoFuncReg_, ID) { \
        _REG_FUNC_CONCAT(AutoFuncReg_, ID)() { \
          using _Sig = typename Engine::function_traits<decltype(&FUNC)>::signature; \
          Engine::FunctionRegistry<_Sig>::register_function(#FUNC, FUNC); \
        } \
      }; \
      static _REG_FUNC_CONCAT(AutoFuncReg_, ID) _REG_FUNC_CONCAT(s_auto_func_reg_, ID); \
    }

#define REGISTER_FUNCTION(FUNC) REGISTER_FUNCTION_IMPL(FUNC, __COUNTER__)

  /// Register a function in the multi-signature registry
  /// Usage: REGISTER_MULTI_FUNCTION(my_callback, void(int, float), my_function)
#define REGISTER_MULTI_FUNCTION(NAME, SIGNATURE, FUNC) \
    namespace { \
      struct MultiFunctionRegistrar_##NAME { \
        MultiFunctionRegistrar_##NAME() { \
          Engine::MultiFunctionRegistry::instance().register_function<SIGNATURE>(#NAME, FUNC); \
        } \
      }; \
      static MultiFunctionRegistrar_##NAME multi_function_registrar_##NAME##_; \
    }

} // namespace Engine


REFLECT_FIELDS(Engine::SerializableFunction, name)