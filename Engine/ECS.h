#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <functional>
#include <tuple>
#include <any>

namespace ECS
{

  // Forward declarations
  class world;
  class entity_handle;

  // Component storage base class
  class component_storage_base
  {
  public:
    virtual ~component_storage_base() = default;
    virtual void remove(size_t entity_id) = 0;
    virtual bool has(size_t entity_id) const = 0;
  };

  // Typed component storage
  template <typename T>
  class component_storage : public component_storage_base
  {
    std::unordered_map<size_t, T> components;

  public:
    T* get(size_t entity_id)
    {
      auto it = components.find(entity_id);
      return it != components.end() ? &it->second : nullptr;
    }

    const T* get(size_t entity_id) const
    {
      auto it = components.find(entity_id);
      return it != components.end() ? &it->second : nullptr;
    }

    T& add(size_t entity_id)
    {
      return components[entity_id];
    }

    void remove(size_t entity_id) override
    {
      components.erase(entity_id);
    }

    bool has(size_t entity_id) const override
    {
      return components.find(entity_id) != components.end();
    }

    auto begin() { return components.begin(); }
    auto end() { return components.end(); }
  };

  // System base class
  class system_base
  {
  public:
    virtual ~system_base() = default;
    virtual void run(world& w) = 0;
  };

  // System implementation
  template <typename... Components>
  class system : public system_base
  {
    using Func = std::function<void(Components &...)>;
    Func func;

  public:
    system(Func f) : func(f) {}

    void run(world& w) override;
  };

  // System builder for fluent API
  template <typename... Components>
  class system_builder
  {
    world* w;

  public:
    system_builder(world* world) : w(world) {}

    void each(std::function<void(Components &...)> func);
  };

  // Entity handle for fluent API
  class entity_handle
  {
    world* w;
    size_t id;

  public:
    entity_handle(world* world, size_t entity_id) : w(world), id(entity_id) {}

    template <typename Func>
    entity_handle& insert(Func func);

    size_t get_id() const { return id; }
  };

  // Main world class
  class world
  {
    size_t next_entity_id = 1;
    std::unordered_map<std::type_index, std::unique_ptr<component_storage_base>> storages;
    std::vector<std::unique_ptr<system_base>> systems;
    std::vector<size_t> entities;

  public:
    // Get or create component storage
    template <typename T>
    component_storage<T>* get_storage()
    {
      auto type_id = std::type_index(typeid(T));
      auto it = storages.find(type_id);

      if (it == storages.end())
      {
        auto storage = std::make_unique<component_storage<T>>();
        auto ptr = storage.get();
        storages[type_id] = std::move(storage);
        return ptr;
      }

      return static_cast<component_storage<T> *>(it->second.get());
    }

    // Create entity
    entity_handle entity()
    {
      size_t id = next_entity_id++;
      entities.push_back(id);
      return entity_handle(this, id);
    }

    // Create system
    template <typename... Components>
    system_builder<Components...> system()
    {
      return system_builder<Components...>(this);
    }

    // Add system to world
    void add_system(std::unique_ptr<system_base> sys)
    {
      systems.push_back(std::move(sys));
    }

    // Progress world (run all systems)
    bool progress()
    {
      for (auto& sys : systems)
      {
        sys->run(*this);
      }
      return true;
    }

    // Get all entities with specific components
    template <typename... Components>
    std::vector<size_t> get_entities_with()
    {
      std::vector<size_t> result;

      for (size_t entity_id : entities)
      {
        if (has_components<Components...>(entity_id))
        {
          result.push_back(entity_id);
        }
      }

      return result;
    }

  private:
    template <typename T, typename... Rest>
    bool has_components(size_t entity_id)
    {
      auto storage = get_storage<T>();
      if (!storage || !storage->has(entity_id))
      {
        return false;
      }

      if constexpr (sizeof...(Rest) > 0)
      {
        return has_components<Rest...>(entity_id);
      }
      return true;
    }
  };

  // --- Start of type deduction helpers for the generic `insert` method ---

  // Base trait for deducing function arguments
  template <typename T>
  struct function_traits : function_traits<decltype(&T::operator())> {};

  // Specialization for member functions (used for lambdas)
  template <typename Class, typename R, typename... Args>
  struct function_traits<R(Class::*)(Args...) const>
  {
    using args_tuple = std::tuple<Args...>;
  };

  // Specialization for std::function
  template <typename R, typename... Args>
  struct function_traits<std::function<R(Args...)>>
  {
    using args_tuple = std::tuple<Args...>;
  };

  // Helper to get the argument tuple from any callable
  template <typename Func>
  using get_lambda_args_t = typename function_traits<Func>::args_tuple;

  // A helper struct to unpack a tuple of types using an index sequence
  template <typename Func, typename Tuple, std::size_t... Is>
  void insert_components_impl(world* w, size_t id, Func&& func, std::index_sequence<Is...>)
  {
    // Create a tuple of references to the newly added components.
    std::tuple<std::remove_reference_t<std::tuple_element_t<Is, Tuple>> &...> component_refs{
        w->get_storage<std::remove_reference_t<std::tuple_element_t<Is, Tuple>>>()->add(id)... };

    // Call the user's function with the component references.
    std::apply(std::forward<Func>(func), component_refs);
  }

  // --- End of type deduction helpers ---

  // Template implementations

  template <typename... Components>
  void system<Components...>::run(world& w)
  {
    // Get all entities that have the required components
    auto entities = w.get_entities_with<std::remove_const_t<Components>...>();

    for (size_t entity_id : entities)
    {
      // Get component pointers for this entity
      std::tuple<Components *...> component_ptrs = {
          w.get_storage<std::remove_const_t<Components>>()->get(entity_id)... };

      // Apply the function if all components exist
      if ((std::get<Components*>(component_ptrs) && ...))
      {
        std::apply([this](Components *...ptrs)
          { func(*ptrs...); }, component_ptrs);
      }
    }
  }

  template <typename... Components>
  void system_builder<Components...>::each(std::function<void(Components &...)> func)
  {
    auto sys = std::make_unique<system<Components...>>(func);
    w->add_system(std::move(sys));
  }

  // The updated, generic insert method.
  template <typename Func>
  entity_handle& entity_handle::insert(Func func)
  {
    using ComponentTuple = get_lambda_args_t<Func>;
    constexpr auto N = std::tuple_size_v<ComponentTuple>;

    // Use the helper function with an index sequence
    insert_components_impl<Func, ComponentTuple>(w, id, std::move(func), std::make_index_sequence<N>{});

    return *this;
  }

} // namespace ECS