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
//          named lookups. Uses a single string-keyed singleton so that all
//          function signatures share one registry instance — safe across DLL
//          boundaries. Used throughout the engine for state machines, event
//          callbacks, easing functions, and more.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <functional>
#include <unordered_map>
#include <string>
#include <typeinfo>
#include <any>
#include "Logger.h"
#include "EngineExport.h"
#include <vector>

namespace Engine {

  // Forward declaration
  class FunctionRegistry;

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
  // Unified Function Registry (string-keyed singleton)
  // ============================================================================

  /// Single registry for all function signatures. Uses typeid(Sig).name() to partition
  /// functions by signature, and std::any for type-erased storage. String keys are
  /// safe across DLL reload boundaries (no dangling type_info pointers).
  class FunctionRegistry {
  public:
    ENGINE_API static FunctionRegistry& instance();

    /// Register a function with a string name under a specific signature
    template<typename Sig>
    void register_function(const std::string& name, std::function<Sig> func) {
      std::string key = typeid(Sig).name();
      auto& inner = functions_[key];
      if (inner.count(name)) {
        Log::warning("FunctionRegistry: Overwriting existing function '{}'", name);
      }
      inner[name] = std::make_any<std::function<Sig>>(std::move(func));
    }

    /// Get a function by name (returns empty std::function if not found)
    template<typename Sig>
    std::function<Sig> get_function(const std::string& name) const {
      std::string key = typeid(Sig).name();
      auto outer_it = functions_.find(key);
      if (outer_it == functions_.end()) return {};
      auto inner_it = outer_it->second.find(name);
      if (inner_it == outer_it->second.end()) return {};
      try {
        return std::any_cast<std::function<Sig>>(inner_it->second);
      }
      catch (const std::bad_any_cast&) {
        Log::error("FunctionRegistry: Type mismatch for function '{}'", name);
        return {};
      }
    }

    /// Check if a function with this name and signature exists
    template<typename Sig>
    bool has_function(const std::string& name) const {
      std::string key = typeid(Sig).name();
      auto outer_it = functions_.find(key);
      if (outer_it == functions_.end()) return false;
      return outer_it->second.count(name) > 0;
    }

    /// Unregister a function
    template<typename Sig>
    void unregister_function(const std::string& name) {
      std::string key = typeid(Sig).name();
      auto outer_it = functions_.find(key);
      if (outer_it != functions_.end()) {
        outer_it->second.erase(name);
      }
    }

    /// Get all registered function names for a given signature (for editor/debug)
    template<typename Sig>
    std::vector<std::string> get_all_names() const {
      std::string key = typeid(Sig).name();
      auto outer_it = functions_.find(key);
      if (outer_it == functions_.end()) return {};
      std::vector<std::string> names;
      names.reserve(outer_it->second.size());
      for (const auto& [n, _] : outer_it->second) {
        names.push_back(n);
      }
      return names;
    }

    /// Clear all functions of a given signature
    template<typename Sig>
    void clear() {
      functions_.erase(std::string(typeid(Sig).name()));
    }

    /// Clear all registered functions (all signatures)
    void clear_all() {
      functions_.clear();
    }

    /// Get count of functions with a given signature
    template<typename Sig>
    size_t size() const {
      std::string key = typeid(Sig).name();
      auto outer_it = functions_.find(key);
      return (outer_it != functions_.end()) ? outer_it->second.size() : 0;
    }

  private:
    // Two-level map: signature name (typeid.name()) -> { name -> any(std::function<Sig>) }
    std::unordered_map<std::string, std::unordered_map<std::string, std::any>> functions_;

    FunctionRegistry() = default;
    FunctionRegistry(const FunctionRegistry&) = delete;
    FunctionRegistry& operator=(const FunctionRegistry&) = delete;
  };

  // ============================================================================
  // SerializableFunction Method Implementations
  // ============================================================================

  template<typename Signature>
  inline std::function<Signature> SerializableFunction::get() const {
    return FunctionRegistry::instance().get_function<Signature>(name);
  }

  template<typename Signature>
  inline bool SerializableFunction::exists() const {
    return FunctionRegistry::instance().has_function<Signature>(name);
  }

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
          Engine::FunctionRegistry::instance().register_function<_Sig>(#FUNC, std::function<_Sig>(FUNC)); \
        } \
      }; \
      static _REG_FUNC_CONCAT(AutoFuncReg_, ID) _REG_FUNC_CONCAT(s_auto_func_reg_, ID); \
    }

#define REGISTER_FUNCTION(FUNC) REGISTER_FUNCTION_IMPL(FUNC, __COUNTER__)

} // namespace Engine


REFLECT_FIELDS(Engine::SerializableFunction, name)