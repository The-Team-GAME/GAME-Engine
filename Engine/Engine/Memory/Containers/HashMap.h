//-----------------------------------------------------------------------------------
// file:    HashMap.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Custom hash map container that uses the Memory Manager for all
//          allocations. Replaces std::unordered_map in the ECS. Uses open
//          addressing with linear probing for collision resolution.
//          No STL containers used internally.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../MemoryManager.h"
#include <utility>    // For std::move, std::forward, std::pair
#include <new>        // For placement new
#include <functional> // For std::hash (we only use this for hash computation)
#include <cstring>    // For memset
#include <tuple>      // For tuple_size, tuple_element (structured bindings support)
#include <iterator>   // For iterator_traits

namespace Engine {
  namespace Memory {

    // Forward declarations for structured binding support
    template<typename Key, typename Value, typename Hash>
    class HashMap;

    //=================================================================================
    // HashMap<Key, Value> - Custom hash map (replacement for std::unordered_map)
    //
    // Implementation: Open addressing with linear probing
    // - More cache-friendly than separate chaining
    // - Simpler memory management (single contiguous array)
    //
    // Bucket States:
    // - Empty: Never used
    // - Occupied: Contains valid data
    // - Deleted: Was occupied, now deleted (tombstone)
    //
    // Load Factor: Rehashes when load > 0.75
    //=================================================================================
    template<typename Key, typename Value, typename Hash = std::hash<Key>>
    class HashMap {
    public:
      // Type aliases
      using key_type = Key;
      using mapped_type = Value;
      using value_type = std::pair<const Key, Value>;
      using size_type = size_t;
      using hasher = Hash;

    private:
      // Bucket state enum
      enum class BucketState : u8 {
        Empty = 0,
        Occupied = 1,
        Deleted = 2
      };

      // Internal bucket structure
      struct Bucket {
        Key key;
        Value value;
        BucketState state;

        Bucket() : state(BucketState::Empty) {}
      };

      // Proxy reference type for iterator dereferencing
      // This allows structured bindings to work with our iterator
      struct KeyValuePair {
        Key& first;
        Value& second;

        KeyValuePair(Key& k, Value& v) : first(k), second(v) {}

        // Allow access like std::pair
        operator std::pair<const Key&, Value&>() const {
          return { first, second };
        }
      };

      struct ConstKeyValuePair {
        const Key& first;
        const Value& second;

        ConstKeyValuePair(const Key& k, const Value& v) : first(k), second(v) {}

        operator std::pair<const Key&, const Value&>() const {
          return { first, second };
        }
      };

    public:
      //=================================================================================
      // Iterator (forward only)
      // 
      // The iterator stores a KeyValuePair internally and updates it when dereferenced.
      // This allows returning a reference for structured binding support (auto& [k, v]).
      //=================================================================================
      class iterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = KeyValuePair;
        using difference_type = std::ptrdiff_t;
        using pointer = KeyValuePair*;
        using reference = KeyValuePair&;

        iterator(Bucket* buckets, size_t index, size_t capacity)
          : m_buckets(buckets), m_index(index), m_capacity(capacity), m_current(nullptr) {
          skip_empty();
        }

        // Returns a reference to an internally stored KeyValuePair
        // The KeyValuePair's first/second are references to actual bucket data
        KeyValuePair& operator*() {
          update_current();
          return *m_current;
        }

        KeyValuePair* operator->() {
          update_current();
          return m_current;
        }

        iterator& operator++() {
          if (m_current) {
            m_current->~KeyValuePair();
            m_current = nullptr;
          }
          ++m_index;
          skip_empty();
          return *this;
        }

        iterator operator++(int) {
          iterator tmp = *this;
          ++(*this);
          return tmp;
        }

        bool operator==(const iterator& other) const {
          return m_buckets == other.m_buckets && m_index == other.m_index;
        }

        bool operator!=(const iterator& other) const {
          return !(*this == other);
        }

        Key& key() { return m_buckets[m_index].key; }
        Value& value() { return m_buckets[m_index].value; }

      private:
        void skip_empty() {
          while (m_index < m_capacity && m_buckets[m_index].state != BucketState::Occupied) {
            ++m_index;
          }
        }

        void update_current() {
          if (!m_current && m_index < m_capacity) {
            // Use placement new to construct KeyValuePair in our storage
            m_current = new (&m_storage) KeyValuePair(
              m_buckets[m_index].key,
              m_buckets[m_index].value
            );
          }
        }

        Bucket* m_buckets;
        size_t m_index;
        size_t m_capacity;
        // Storage for KeyValuePair to allow returning reference
        alignas(KeyValuePair) char m_storage[sizeof(KeyValuePair)];
        KeyValuePair* m_current;
      };

      class const_iterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = ConstKeyValuePair;
        using difference_type = std::ptrdiff_t;
        using pointer = const ConstKeyValuePair*;
        using reference = const ConstKeyValuePair&;

        const_iterator(const Bucket* buckets, size_t index, size_t capacity)
          : m_buckets(buckets), m_index(index), m_capacity(capacity), m_current(nullptr) {
          skip_empty();
        }

        const ConstKeyValuePair& operator*() const {
          const_cast<const_iterator*>(this)->update_current();
          return *m_current;
        }

        const ConstKeyValuePair* operator->() const {
          const_cast<const_iterator*>(this)->update_current();
          return m_current;
        }

        const_iterator& operator++() {
          if (m_current) {
            m_current->~ConstKeyValuePair();
            m_current = nullptr;
          }
          ++m_index;
          skip_empty();
          return *this;
        }

        bool operator==(const const_iterator& other) const {
          return m_buckets == other.m_buckets && m_index == other.m_index;
        }

        bool operator!=(const const_iterator& other) const {
          return !(*this == other);
        }

        const Key& key() const { return m_buckets[m_index].key; }
        const Value& value() const { return m_buckets[m_index].value; }

      private:
        void skip_empty() {
          while (m_index < m_capacity && m_buckets[m_index].state != BucketState::Occupied) {
            ++m_index;
          }
        }

        void update_current() {
          if (!m_current && m_index < m_capacity) {
            m_current = new (&m_storage) ConstKeyValuePair(
              m_buckets[m_index].key,
              m_buckets[m_index].value
            );
          }
        }

        const Bucket* m_buckets;
        size_t m_index;
        size_t m_capacity;
        alignas(ConstKeyValuePair) mutable char m_storage[sizeof(ConstKeyValuePair)];
        mutable ConstKeyValuePair* m_current;
      };

      //=================================================================================
      // Constructors / Destructor
      //=================================================================================

      /*=================================================================================
      Function Brief:
      Default constructor - creates an empty map.

      Parameters:
      None.

      Where to call it:
      When declaring a map that will be filled later.
      ==================================================================================*/
      HashMap();

      /*=================================================================================
      Function Brief:
      Constructs a map with initial capacity.

      Parameters:
      initial_capacity - Number of buckets to allocate.

      Where to call it:
      When you know the approximate size needed.
      ==================================================================================*/
      explicit HashMap(size_t initial_capacity);

      /*=================================================================================
      Function Brief:
      Copy constructor.

      Parameters:
      other - Map to copy from.

      Where to call it:
      When a copy is needed.
      ==================================================================================*/
      HashMap(const HashMap& other);

      /*=================================================================================
      Function Brief:
      Move constructor.

      Parameters:
      other - Map to move from.

      Where to call it:
      When transferring ownership.
      ==================================================================================*/
      HashMap(HashMap&& other) noexcept;

      /*=================================================================================
      Function Brief:
      Destructor.

      Parameters:
      None.

      Where to call it:
      Automatically called when map goes out of scope.
      ==================================================================================*/
      ~HashMap();

      //=================================================================================
      // Assignment
      //=================================================================================

      HashMap& operator=(const HashMap& other);
      HashMap& operator=(HashMap&& other) noexcept;

      //=================================================================================
      // Element Access
      //=================================================================================

      /*=================================================================================
      Function Brief:
      Returns a reference to the value associated with key.
      Inserts a default-constructed value if key doesn't exist.

      Parameters:
      key - The key to look up.

      Returns:
      Reference to the value.

      Where to call it:
      When accessing or inserting elements.
      ==================================================================================*/
      Value& operator[](const Key& key);
      Value& operator[](Key&& key);

      /*=================================================================================
      Function Brief:
      Returns an iterator to the element with the specified key, or end() if not found.

      Parameters:
      key - The key to look up.

      Returns:
      Iterator to the element, or end() if not found.

      Where to call it:
      When checking if a key exists and getting its value.
      ==================================================================================*/
      iterator find(const Key& key);
      const_iterator find(const Key& key) const;

      /*=================================================================================
      Function Brief:
      Returns a pointer to the value associated with key, or nullptr if not found.

      Parameters:
      key - The key to look up.

      Returns:
      Pointer to the value, or nullptr.

      Where to call it:
      When checking if a key exists and getting its value (alternative to find).
      ==================================================================================*/
      Value* get(const Key& key);
      const Value* get(const Key& key) const;

      /*=================================================================================
      Function Brief:
      Returns true if the map contains the specified key.

      Parameters:
      key - The key to check.

      Returns:
      True if key exists, false otherwise.

      Where to call it:
      When you only need to check existence, not retrieve the value.
      ==================================================================================*/
      bool contains(const Key& key) const;

      /*=================================================================================
      Function Brief:
      Returns a reference to the value, throws if not found (debug only).

      Parameters:
      key - The key to look up.

      Returns:
      Reference to the value.

      Where to call it:
      When the key is expected to exist.
      ==================================================================================*/
      Value& at(const Key& key);
      const Value& at(const Key& key) const;

      //=================================================================================
      // Capacity
      //=================================================================================

      bool empty() const noexcept { return m_size == 0; }
      size_t size() const noexcept { return m_size; }
      size_t capacity() const noexcept { return m_capacity; }

      //=================================================================================
      // Modifiers
      //=================================================================================

      /*=================================================================================
      Function Brief:
      Inserts a key-value pair. If the key already exists, does nothing.

      Parameters:
      key - The key.
      value - The value.

      Returns:
      Pair of (pointer to value, bool indicating if insertion occurred).

      Where to call it:
      When inserting elements.
      ==================================================================================*/
      std::pair<Value*, bool> insert(const Key& key, const Value& value);
      std::pair<Value*, bool> insert(const Key& key, Value&& value);
      std::pair<Value*, bool> insert(Key&& key, Value&& value);

      /*=================================================================================
      Function Brief:
      Inserts or updates a key-value pair.

      Parameters:
      key - The key.
      value - The value.

      Returns:
      Reference to the value.

      Where to call it:
      When inserting or updating elements.
      ==================================================================================*/
      Value& insert_or_assign(const Key& key, const Value& value);
      Value& insert_or_assign(const Key& key, Value&& value);

      /*=================================================================================
      Function Brief:
      Constructs a value in place.

      Parameters:
      key - The key.
      args - Arguments for value construction.

      Returns:
      Pair of (pointer to value, bool indicating if insertion occurred).

      Where to call it:
      When constructing values in place.
      ==================================================================================*/
      template<typename... Args>
      std::pair<Value*, bool> emplace(const Key& key, Args&&... args);

      /*=================================================================================
      Function Brief:
      Removes a key from the map.

      Parameters:
      key - The key to remove.

      Returns:
      true if the key was found and removed.

      Where to call it:
      When removing elements.
      ==================================================================================*/
      bool erase(const Key& key);

      /*=================================================================================
      Function Brief:
      Removes all elements from the map.

      Parameters:
      None.

      Where to call it:
      When clearing the map.
      ==================================================================================*/
      void clear();

      /*=================================================================================
      Function Brief:
      Returns the number of elements with the given key (0 or 1).

      Parameters:
      key - The key to count.

      Returns:
      0 or 1.

      Where to call it:
      For compatibility with std::unordered_map.
      ==================================================================================*/
      size_t count(const Key& key) const;

      //=================================================================================
      // Iterators
      //=================================================================================

      iterator begin() { return iterator(m_buckets, 0, m_capacity); }
      iterator end() { return iterator(m_buckets, m_capacity, m_capacity); }
      const_iterator begin() const { return const_iterator(m_buckets, 0, m_capacity); }
      const_iterator end() const { return const_iterator(m_buckets, m_capacity, m_capacity); }
      const_iterator cbegin() const { return const_iterator(m_buckets, 0, m_capacity); }
      const_iterator cend() const { return const_iterator(m_buckets, m_capacity, m_capacity); }

      //=================================================================================
      // Hash Policy
      //=================================================================================

      /*=================================================================================
      Function Brief:
      Reserves space for at least the specified number of elements.

      Parameters:
      count - Number of elements to reserve space for.

      Where to call it:
      When you know the required size in advance.
      ==================================================================================*/
      void reserve(size_t count);

    private:
      Bucket* m_buckets;    // Array of buckets
      size_t m_size;        // Number of elements
      size_t m_capacity;    // Number of buckets
      size_t m_tombstones;  // Number of deleted buckets
      Hash m_hasher;        // Hash function

      static constexpr float MAX_LOAD_FACTOR = 0.75f;
      static constexpr size_t MIN_CAPACITY = 8;

      /*=================================================================================
      Function Brief:
      Computes the bucket index for a key.

      Parameters:
      key - The key to hash.

      Returns:
      Bucket index.

      Where to call it:
      Internally when looking up or inserting.
      ==================================================================================*/
      size_t hash_key(const Key& key) const;

      /*=================================================================================
      Function Brief:
      Finds a bucket for the given key (for insertion or lookup).

      Parameters:
      key - The key.
      for_insert - If true, returns first empty/deleted bucket. If false, returns only occupied bucket with matching key.

      Returns:
      Bucket index or m_capacity if not found.

      Where to call it:
      Internally.
      ==================================================================================*/
      size_t find_bucket(const Key& key, bool for_insert) const;

      /*=================================================================================
      Function Brief:
      Rehashes the map to the new capacity.

      Parameters:
      new_capacity - New number of buckets.

      Where to call it:
      Internally when load factor is exceeded.
      ==================================================================================*/
      void rehash(size_t new_capacity);

      /*=================================================================================
      Function Brief:
      Checks if rehashing is needed and does it if so.

      Parameters:
      None.

      Where to call it:
      After insertions.
      ==================================================================================*/
      void maybe_rehash();
    };

    //=================================================================================
    // Template Implementation
    //=================================================================================

    template<typename Key, typename Value, typename Hash>
    HashMap<Key, Value, Hash>::HashMap()
      : m_buckets(nullptr)
      , m_size(0)
      , m_capacity(0)
      , m_tombstones(0)
    {
    }

    template<typename Key, typename Value, typename Hash>
    HashMap<Key, Value, Hash>::HashMap(size_t initial_capacity)
      : m_buckets(nullptr)
      , m_size(0)
      , m_capacity(0)
      , m_tombstones(0)
    {
      reserve(initial_capacity);
    }

    template<typename Key, typename Value, typename Hash>
    HashMap<Key, Value, Hash>::HashMap(const HashMap& other)
      : m_buckets(nullptr)
      , m_size(0)
      , m_capacity(0)
      , m_tombstones(0)
    {
      if (other.m_capacity > 0) {
        rehash(other.m_capacity);
        for (size_t i = 0; i < other.m_capacity; ++i) {
          if (other.m_buckets[i].state == BucketState::Occupied) {
            insert(other.m_buckets[i].key, other.m_buckets[i].value);
          }
        }
      }
    }

    template<typename Key, typename Value, typename Hash>
    HashMap<Key, Value, Hash>::HashMap(HashMap&& other) noexcept
      : m_buckets(other.m_buckets)
      , m_size(other.m_size)
      , m_capacity(other.m_capacity)
      , m_tombstones(other.m_tombstones)
    {
      other.m_buckets = nullptr;
      other.m_size = 0;
      other.m_capacity = 0;
      other.m_tombstones = 0;
    }

    template<typename Key, typename Value, typename Hash>
    HashMap<Key, Value, Hash>::~HashMap() {
      clear();
      if (m_buckets && m_capacity > 0) {
        // Always call deallocate - Memory Manager handles post-shutdown gracefully
        // by falling back to OS delete if pools are already freed
        MemoryManager::instance().deallocate(m_buckets, m_capacity * sizeof(Bucket));
        // Always null out the pointer to prevent double-free
        m_buckets = nullptr;
      }
      m_capacity = 0;
    }

    template<typename Key, typename Value, typename Hash>
    HashMap<Key, Value, Hash>& HashMap<Key, Value, Hash>::operator=(const HashMap& other) {
      if (this != &other) {
        clear();
        if (other.m_capacity > 0) {
          rehash(other.m_capacity);
          for (size_t i = 0; i < other.m_capacity; ++i) {
            if (other.m_buckets[i].state == BucketState::Occupied) {
              insert(other.m_buckets[i].key, other.m_buckets[i].value);
            }
          }
        }
      }
      return *this;
    }

    template<typename Key, typename Value, typename Hash>
    HashMap<Key, Value, Hash>& HashMap<Key, Value, Hash>::operator=(HashMap&& other) noexcept {
      if (this != &other) {
        clear();
        if (m_buckets && m_capacity > 0) {
          // Always call deallocate - Memory Manager handles post-shutdown gracefully
          MemoryManager::instance().deallocate(m_buckets, m_capacity * sizeof(Bucket));
        }

        m_buckets = other.m_buckets;
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_tombstones = other.m_tombstones;

        other.m_buckets = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
        other.m_tombstones = 0;
      }
      return *this;
    }

    template<typename Key, typename Value, typename Hash>
    Value& HashMap<Key, Value, Hash>::operator[](const Key& key) {
      maybe_rehash();

      size_t index = find_bucket(key, true);

      if (m_buckets[index].state != BucketState::Occupied) {
        // Insert new element
        new (&m_buckets[index].key) Key(key);
        new (&m_buckets[index].value) Value();
        if (m_buckets[index].state == BucketState::Deleted) {
          --m_tombstones;
        }
        m_buckets[index].state = BucketState::Occupied;
        ++m_size;
      }

      return m_buckets[index].value;
    }

    template<typename Key, typename Value, typename Hash>
    Value& HashMap<Key, Value, Hash>::operator[](Key&& key) {
      maybe_rehash();

      size_t index = find_bucket(key, true);

      if (m_buckets[index].state != BucketState::Occupied) {
        // Insert new element
        new (&m_buckets[index].key) Key(std::move(key));
        new (&m_buckets[index].value) Value();
        if (m_buckets[index].state == BucketState::Deleted) {
          --m_tombstones;
        }
        m_buckets[index].state = BucketState::Occupied;
        ++m_size;
      }

      return m_buckets[index].value;
    }

    template<typename Key, typename Value, typename Hash>
    typename HashMap<Key, Value, Hash>::iterator HashMap<Key, Value, Hash>::find(const Key& key) {
      if (m_size == 0) return end();

      size_t index = find_bucket(key, false);
      if (index == m_capacity) return end();

      return iterator(m_buckets, index, m_capacity);
    }

    template<typename Key, typename Value, typename Hash>
    typename HashMap<Key, Value, Hash>::const_iterator HashMap<Key, Value, Hash>::find(const Key& key) const {
      if (m_size == 0) return end();

      size_t index = find_bucket(key, false);
      if (index == m_capacity) return end();

      return const_iterator(m_buckets, index, m_capacity);
    }

    template<typename Key, typename Value, typename Hash>
    Value* HashMap<Key, Value, Hash>::get(const Key& key) {
      if (m_size == 0) return nullptr;

      size_t index = find_bucket(key, false);
      if (index == m_capacity) return nullptr;

      return &m_buckets[index].value;
    }

    template<typename Key, typename Value, typename Hash>
    const Value* HashMap<Key, Value, Hash>::get(const Key& key) const {
      if (m_size == 0) return nullptr;

      size_t index = find_bucket(key, false);
      if (index == m_capacity) return nullptr;

      return &m_buckets[index].value;
    }

    template<typename Key, typename Value, typename Hash>
    bool HashMap<Key, Value, Hash>::contains(const Key& key) const {
      if (m_size == 0) return false;

      size_t index = find_bucket(key, false);
      return index != m_capacity;
    }

    template<typename Key, typename Value, typename Hash>
    Value& HashMap<Key, Value, Hash>::at(const Key& key) {
      if (m_size == 0) {
        // Would throw in STL, here we return first bucket (undefined behavior like STL)
        return m_buckets[0].value;
      }

      size_t index = find_bucket(key, false);
      // In debug mode, could throw. For now, just return reference
      return m_buckets[index].value;
    }

    template<typename Key, typename Value, typename Hash>
    const Value& HashMap<Key, Value, Hash>::at(const Key& key) const {
      if (m_size == 0) {
        return m_buckets[0].value;
      }

      size_t index = find_bucket(key, false);
      return m_buckets[index].value;
    }

    template<typename Key, typename Value, typename Hash>
    std::pair<Value*, bool> HashMap<Key, Value, Hash>::insert(const Key& key, const Value& value) {
      maybe_rehash();

      size_t index = find_bucket(key, true);

      if (m_buckets[index].state == BucketState::Occupied) {
        // Key already exists
        return { &m_buckets[index].value, false };
      }

      // Insert new element
      new (&m_buckets[index].key) Key(key);
      new (&m_buckets[index].value) Value(value);
      if (m_buckets[index].state == BucketState::Deleted) {
        --m_tombstones;
      }
      m_buckets[index].state = BucketState::Occupied;
      ++m_size;

      return { &m_buckets[index].value, true };
    }

    template<typename Key, typename Value, typename Hash>
    std::pair<Value*, bool> HashMap<Key, Value, Hash>::insert(const Key& key, Value&& value) {
      maybe_rehash();

      size_t index = find_bucket(key, true);

      if (m_buckets[index].state == BucketState::Occupied) {
        return { &m_buckets[index].value, false };
      }

      new (&m_buckets[index].key) Key(key);
      new (&m_buckets[index].value) Value(std::move(value));
      if (m_buckets[index].state == BucketState::Deleted) {
        --m_tombstones;
      }
      m_buckets[index].state = BucketState::Occupied;
      ++m_size;

      return { &m_buckets[index].value, true };
    }

    template<typename Key, typename Value, typename Hash>
    std::pair<Value*, bool> HashMap<Key, Value, Hash>::insert(Key&& key, Value&& value) {
      maybe_rehash();

      size_t index = find_bucket(key, true);

      if (m_buckets[index].state == BucketState::Occupied) {
        return { &m_buckets[index].value, false };
      }

      new (&m_buckets[index].key) Key(std::move(key));
      new (&m_buckets[index].value) Value(std::move(value));
      if (m_buckets[index].state == BucketState::Deleted) {
        --m_tombstones;
      }
      m_buckets[index].state = BucketState::Occupied;
      ++m_size;

      return { &m_buckets[index].value, true };
    }

    template<typename Key, typename Value, typename Hash>
    Value& HashMap<Key, Value, Hash>::insert_or_assign(const Key& key, const Value& value) {
      maybe_rehash();

      size_t index = find_bucket(key, true);

      if (m_buckets[index].state == BucketState::Occupied) {
        m_buckets[index].value = value;
      }
      else {
        new (&m_buckets[index].key) Key(key);
        new (&m_buckets[index].value) Value(value);
        if (m_buckets[index].state == BucketState::Deleted) {
          --m_tombstones;
        }
        m_buckets[index].state = BucketState::Occupied;
        ++m_size;
      }

      return m_buckets[index].value;
    }

    template<typename Key, typename Value, typename Hash>
    Value& HashMap<Key, Value, Hash>::insert_or_assign(const Key& key, Value&& value) {
      maybe_rehash();

      size_t index = find_bucket(key, true);

      if (m_buckets[index].state == BucketState::Occupied) {
        m_buckets[index].value = std::move(value);
      }
      else {
        new (&m_buckets[index].key) Key(key);
        new (&m_buckets[index].value) Value(std::move(value));
        if (m_buckets[index].state == BucketState::Deleted) {
          --m_tombstones;
        }
        m_buckets[index].state = BucketState::Occupied;
        ++m_size;
      }

      return m_buckets[index].value;
    }

    template<typename Key, typename Value, typename Hash>
    template<typename... Args>
    std::pair<Value*, bool> HashMap<Key, Value, Hash>::emplace(const Key& key, Args&&... args) {
      maybe_rehash();

      size_t index = find_bucket(key, true);

      if (m_buckets[index].state == BucketState::Occupied) {
        return { &m_buckets[index].value, false };
      }

      new (&m_buckets[index].key) Key(key);
      new (&m_buckets[index].value) Value(std::forward<Args>(args)...);
      if (m_buckets[index].state == BucketState::Deleted) {
        --m_tombstones;
      }
      m_buckets[index].state = BucketState::Occupied;
      ++m_size;

      return { &m_buckets[index].value, true };
    }

    template<typename Key, typename Value, typename Hash>
    bool HashMap<Key, Value, Hash>::erase(const Key& key) {
      if (m_size == 0) return false;

      size_t index = find_bucket(key, false);
      if (index == m_capacity) return false;

      // Destroy key and value
      m_buckets[index].key.~Key();
      m_buckets[index].value.~Value();
      m_buckets[index].state = BucketState::Deleted;
      --m_size;
      ++m_tombstones;

      return true;
    }

    template<typename Key, typename Value, typename Hash>
    void HashMap<Key, Value, Hash>::clear() {
      // Guard against null or invalid m_buckets (e.g., after move)
      if (!m_buckets || m_capacity == 0) {
        m_size = 0;
        m_tombstones = 0;
        return;
      }

      for (size_t i = 0; i < m_capacity; ++i) {
        if (m_buckets[i].state == BucketState::Occupied) {
          m_buckets[i].key.~Key();
          m_buckets[i].value.~Value();
          m_buckets[i].state = BucketState::Empty;
        }
        else if (m_buckets[i].state == BucketState::Deleted) {
          m_buckets[i].state = BucketState::Empty;
        }
      }
      m_size = 0;
      m_tombstones = 0;
    }

    template<typename Key, typename Value, typename Hash>
    size_t HashMap<Key, Value, Hash>::count(const Key& key) const {
      return contains(key) ? 1 : 0;
    }

    template<typename Key, typename Value, typename Hash>
    void HashMap<Key, Value, Hash>::reserve(size_t count) {
      size_t needed_capacity = static_cast<size_t>(count / MAX_LOAD_FACTOR) + 1;
      if (needed_capacity > m_capacity) {
        rehash(needed_capacity);
      }
    }

    template<typename Key, typename Value, typename Hash>
    size_t HashMap<Key, Value, Hash>::hash_key(const Key& key) const {
      return m_hasher(key) % m_capacity;
    }

    template<typename Key, typename Value, typename Hash>
    size_t HashMap<Key, Value, Hash>::find_bucket(const Key& key, bool for_insert) const {
      if (m_capacity == 0) return 0;

      size_t start_index = hash_key(key);
      size_t index = start_index;
      size_t first_deleted = m_capacity; // Invalid index

      do {
        BucketState state = m_buckets[index].state;

        if (state == BucketState::Empty) {
          // Key not found
          if (for_insert) {
            return first_deleted != m_capacity ? first_deleted : index;
          }
          return m_capacity;
        }

        if (state == BucketState::Deleted) {
          if (first_deleted == m_capacity) {
            first_deleted = index;
          }
        }
        else if (m_buckets[index].key == key) {
          // Found the key
          return index;
        }

        index = (index + 1) % m_capacity;
      } while (index != start_index);

      // Table is full of deleted entries
      if (for_insert && first_deleted != m_capacity) {
        return first_deleted;
      }

      return m_capacity;
    }

    template<typename Key, typename Value, typename Hash>
    void HashMap<Key, Value, Hash>::rehash(size_t new_capacity) {
      if (new_capacity < MIN_CAPACITY) {
        new_capacity = MIN_CAPACITY;
      }

      // Round up to power of 2 for better distribution
      size_t capacity = MIN_CAPACITY;
      while (capacity < new_capacity) {
        capacity *= 2;
      }
      new_capacity = capacity;

      Bucket* old_buckets = m_buckets;
      size_t old_capacity = m_capacity;

      // Allocate new buckets
      m_buckets = static_cast<Bucket*>(MemoryManager::instance().allocate(new_capacity * sizeof(Bucket)));
      m_capacity = new_capacity;
      m_size = 0;
      m_tombstones = 0;

      // Initialize new buckets
      for (size_t i = 0; i < m_capacity; ++i) {
        m_buckets[i].state = BucketState::Empty;
      }

      // Reinsert old elements
      if (old_buckets) {
        for (size_t i = 0; i < old_capacity; ++i) {
          if (old_buckets[i].state == BucketState::Occupied) {
            insert(std::move(old_buckets[i].key), std::move(old_buckets[i].value));
            old_buckets[i].key.~Key();
            old_buckets[i].value.~Value();
          }
        }

        MemoryManager::instance().deallocate(old_buckets, old_capacity * sizeof(Bucket));
      }
    }

    template<typename Key, typename Value, typename Hash>
    void HashMap<Key, Value, Hash>::maybe_rehash() {
      // Rehash if load factor exceeds threshold or if there are too many tombstones
      if (m_capacity == 0) {
        rehash(MIN_CAPACITY);
      }
      else if ((m_size + m_tombstones + 1) > static_cast<size_t>(m_capacity * MAX_LOAD_FACTOR)) {
        rehash(m_capacity * 2);
      }
    }

  } // namespace Memory
} // namespace Engine
