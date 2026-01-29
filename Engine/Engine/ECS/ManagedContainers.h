//-----------------------------------------------------------------------------------
// file:    ManagedContainers.h
// project: Monkey Stole My GPA
//
// author:  [Your Name] (100%)
//          Primary author
//
// email:   [your.email]@digipen.edu
//
// brief:   Additional STL-free container implementations for the custom memory manager.
//          Includes ManagedHashMap (replaces std::unordered_map), ManagedString
//          (replaces std::string for keys), and ManagedUniquePtr (replaces std::unique_ptr).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Memory/MemoryManager.h"
#include "Types.h"
#include <cstring>
#include <cstdlib>
#include <new>
#include <functional>
#include <utility>

namespace Engine {
  namespace ECS {

    // ============================================================================
    // ManagedHashMap - Hash map without STL (open addressing with linear probing)
    // ============================================================================

    /**
     * @brief A hash map implementation without STL containers.
     *
     * Uses open addressing with linear probing for collision resolution.
     * Memory is allocated in a single contiguous block.
     *
     * @tparam K Key type (must be hashable via std::hash or have hash specialization)
     * @tparam V Value type
     */
    template<typename K, typename V>
    class ManagedHashMap {
    public:
      struct Entry {
        K key;
        V value;
        bool occupied;
        bool deleted;  // Tombstone for deleted entries

        Entry() : occupied(false), deleted(false) {}
      };

    private:
      Entry* buckets_;
      size_t capacity_;
      size_t size_;
      size_t tombstone_count_;

      static constexpr size_t INITIAL_CAPACITY = 16;
      static constexpr float MAX_LOAD_FACTOR = 0.7f;
      static constexpr float TOMBSTONE_THRESHOLD = 0.25f;

      size_t hash_key(const K& key) const {
        return std::hash<K>{}(key) % capacity_;
      }

      void rehash(size_t new_capacity) {
        Entry* old_buckets = buckets_;
        size_t old_capacity = capacity_;

        buckets_ = static_cast<Entry*>(std::malloc(new_capacity * sizeof(Entry)));
        if (!buckets_) {
          buckets_ = old_buckets;
          return;
        }

        // Initialize new buckets
        for (size_t i = 0; i < new_capacity; ++i) {
          new (buckets_ + i) Entry();
        }

        capacity_ = new_capacity;
        size_ = 0;
        tombstone_count_ = 0;

        // Reinsert old entries
        if (old_buckets) {
          for (size_t i = 0; i < old_capacity; ++i) {
            if (old_buckets[i].occupied && !old_buckets[i].deleted) {
              insert(std::move(old_buckets[i].key), std::move(old_buckets[i].value));
              old_buckets[i].~Entry();
            }
            else {
              old_buckets[i].~Entry();
            }
          }
          std::free(old_buckets);
        }
      }

      size_t find_slot(const K& key) const {
        size_t index = hash_key(key);
        size_t first_tombstone = capacity_;  // Invalid index

        for (size_t i = 0; i < capacity_; ++i) {
          size_t probe = (index + i) % capacity_;

          if (!buckets_[probe].occupied && !buckets_[probe].deleted) {
            // Empty slot - return tombstone if found, else this slot
            return (first_tombstone < capacity_) ? first_tombstone : probe;
          }

          if (buckets_[probe].deleted) {
            // Remember first tombstone
            if (first_tombstone >= capacity_) {
              first_tombstone = probe;
            }
            continue;
          }

          if (buckets_[probe].key == key) {
            return probe;
          }
        }

        return (first_tombstone < capacity_) ? first_tombstone : capacity_;
      }

    public:
      ManagedHashMap()
        : buckets_(nullptr)
        , capacity_(0)
        , size_(0)
        , tombstone_count_(0)
      {
        rehash(INITIAL_CAPACITY);
      }

      ~ManagedHashMap() {
        if (buckets_) {
          for (size_t i = 0; i < capacity_; ++i) {
            buckets_[i].~Entry();
          }
          std::free(buckets_);
        }
      }

      // Disable copy
      ManagedHashMap(const ManagedHashMap&) = delete;
      ManagedHashMap& operator=(const ManagedHashMap&) = delete;

      // Enable move
      ManagedHashMap(ManagedHashMap&& other) noexcept
        : buckets_(other.buckets_)
        , capacity_(other.capacity_)
        , size_(other.size_)
        , tombstone_count_(other.tombstone_count_)
      {
        other.buckets_ = nullptr;
        other.capacity_ = 0;
        other.size_ = 0;
        other.tombstone_count_ = 0;
      }

      ManagedHashMap& operator=(ManagedHashMap&& other) noexcept {
        if (this != &other) {
          if (buckets_) {
            for (size_t i = 0; i < capacity_; ++i) {
              buckets_[i].~Entry();
            }
            std::free(buckets_);
          }

          buckets_ = other.buckets_;
          capacity_ = other.capacity_;
          size_ = other.size_;
          tombstone_count_ = other.tombstone_count_;

          other.buckets_ = nullptr;
          other.capacity_ = 0;
          other.size_ = 0;
          other.tombstone_count_ = 0;
        }
        return *this;
      }

      void insert(const K& key, const V& value) {
        // Check if we need to grow
        if ((size_ + tombstone_count_ + 1) > capacity_ * MAX_LOAD_FACTOR) {
          rehash(capacity_ * 2);
        }
        // Clean up tombstones if too many
        else if (tombstone_count_ > capacity_ * TOMBSTONE_THRESHOLD) {
          rehash(capacity_);
        }

        size_t slot = find_slot(key);
        if (slot >= capacity_) return;

        if (buckets_[slot].occupied && !buckets_[slot].deleted) {
          // Update existing
          buckets_[slot].value = value;
        }
        else {
          // Insert new
          if (buckets_[slot].deleted) {
            --tombstone_count_;
          }
          buckets_[slot].key = key;
          buckets_[slot].value = value;
          buckets_[slot].occupied = true;
          buckets_[slot].deleted = false;
          ++size_;
        }
      }

      void insert(K&& key, V&& value) {
        if ((size_ + tombstone_count_ + 1) > capacity_ * MAX_LOAD_FACTOR) {
          rehash(capacity_ * 2);
        }
        else if (tombstone_count_ > capacity_ * TOMBSTONE_THRESHOLD) {
          rehash(capacity_);
        }

        size_t slot = find_slot(key);
        if (slot >= capacity_) return;

        if (buckets_[slot].occupied && !buckets_[slot].deleted) {
          buckets_[slot].value = std::move(value);
        }
        else {
          if (buckets_[slot].deleted) {
            --tombstone_count_;
          }
          buckets_[slot].key = std::move(key);
          buckets_[slot].value = std::move(value);
          buckets_[slot].occupied = true;
          buckets_[slot].deleted = false;
          ++size_;
        }
      }

      template<typename... Args>
      V& emplace(const K& key, Args&&... args) {
        if ((size_ + tombstone_count_ + 1) > capacity_ * MAX_LOAD_FACTOR) {
          rehash(capacity_ * 2);
        }

        size_t slot = find_slot(key);

        if (!buckets_[slot].occupied || buckets_[slot].deleted) {
          if (buckets_[slot].deleted) {
            --tombstone_count_;
          }
          buckets_[slot].key = key;
          new (&buckets_[slot].value) V(std::forward<Args>(args)...);
          buckets_[slot].occupied = true;
          buckets_[slot].deleted = false;
          ++size_;
        }

        return buckets_[slot].value;
      }

      void erase(const K& key) {
        size_t slot = find_slot(key);
        if (slot < capacity_ && buckets_[slot].occupied && !buckets_[slot].deleted) {
          if (buckets_[slot].key == key) {
            buckets_[slot].deleted = true;
            --size_;
            ++tombstone_count_;
          }
        }
      }

      // Forward declaration note: Iterator class is defined below.
      // These find methods return raw pointers internally, but we also
      // provide iterator-returning versions after Iterator is defined.

      V* find_ptr(const K& key) {
        if (size_ == 0) return nullptr;

        size_t slot = find_slot(key);
        if (slot < capacity_ && buckets_[slot].occupied && !buckets_[slot].deleted) {
          if (buckets_[slot].key == key) {
            return &buckets_[slot].value;
          }
        }
        return nullptr;
      }

      const V* find_ptr(const K& key) const {
        if (size_ == 0) return nullptr;

        size_t index = hash_key(key);
        for (size_t i = 0; i < capacity_; ++i) {
          size_t probe = (index + i) % capacity_;

          if (!buckets_[probe].occupied && !buckets_[probe].deleted) {
            return nullptr;
          }

          if (buckets_[probe].deleted) continue;

          if (buckets_[probe].key == key) {
            return &buckets_[probe].value;
          }
        }
        return nullptr;
      }

      bool contains(const K& key) const {
        return find_ptr(key) != nullptr;
      }

      V& operator[](const K& key) {
        V* existing = find(key);
        if (existing) return *existing;

        // Insert default
        insert(key, V{});
        return *find(key);
      }

      size_t size() const { return size_; }
      bool empty() const { return size_ == 0; }

      void clear() {
        for (size_t i = 0; i < capacity_; ++i) {
          if (buckets_[i].occupied) {
            buckets_[i].~Entry();
            new (buckets_ + i) Entry();
          }
        }
        size_ = 0;
        tombstone_count_ = 0;
      }

      // Iterator for range-based for loops
      class Iterator {
      private:
        Entry* buckets_;
        size_t capacity_;
        size_t index_;

        void advance_to_valid() {
          while (index_ < capacity_ &&
            (!buckets_[index_].occupied || buckets_[index_].deleted)) {
            ++index_;
          }
        }

      public:
        // KeyValue pair for structured binding support
        struct KeyValueRef {
          const K& first;
          V& second;
        };

        // Constructor for begin() - advances to first valid entry
        Iterator(Entry* b, size_t cap, size_t idx, bool advance = true)
          : buckets_(b), capacity_(cap), index_(idx) {
          if (advance) {
            advance_to_valid();
          }
        }

        KeyValueRef operator*() {
          return KeyValueRef{ buckets_[index_].key, buckets_[index_].value };
        }

        Iterator& operator++() {
          ++index_;
          advance_to_valid();
          return *this;
        }

        bool operator!=(const Iterator& other) const {
          return index_ != other.index_;
        }

        bool operator==(const Iterator& other) const {
          return index_ == other.index_;
        }

        // Access members like std::pair for compatibility with it->first, it->second
        const K& first() const { return buckets_[index_].key; }
        V& second() { return buckets_[index_].value; }
        const K& key() const { return buckets_[index_].key; }
        V& value() { return buckets_[index_].value; }

        // Arrow operator support struct
        struct Proxy {
          const K& first;
          V& second;
        };
        Proxy operator->() { return Proxy{ buckets_[index_].key, buckets_[index_].value }; }
      };

      Iterator begin() { return Iterator(buckets_, capacity_, 0); }
      Iterator end() { return Iterator(buckets_, capacity_, capacity_); }

      // Const iterator
      class ConstIterator {
      private:
        const Entry* buckets_;
        size_t capacity_;
        size_t index_;

        void advance_to_valid() {
          while (index_ < capacity_ &&
            (!buckets_[index_].occupied || buckets_[index_].deleted)) {
            ++index_;
          }
        }

      public:
        // ConstKeyValueRef pair for structured binding support
        struct ConstKeyValueRef {
          const K& first;
          const V& second;
        };

        ConstIterator(const Entry* b, size_t cap, size_t idx, bool advance = true)
          : buckets_(b), capacity_(cap), index_(idx) {
          if (advance) {
            advance_to_valid();
          }
        }

        ConstKeyValueRef operator*() const {
          return ConstKeyValueRef{ buckets_[index_].key, buckets_[index_].value };
        }

        ConstIterator& operator++() {
          ++index_;
          advance_to_valid();
          return *this;
        }

        bool operator!=(const ConstIterator& other) const {
          return index_ != other.index_;
        }

        bool operator==(const ConstIterator& other) const {
          return index_ == other.index_;
        }

        // Access members like std::pair for compatibility
        const K& first() const { return buckets_[index_].key; }
        const V& second() const { return buckets_[index_].value; }
        const K& key() const { return buckets_[index_].key; }
        const V& value() const { return buckets_[index_].value; }

        // Arrow operator support struct
        struct Proxy {
          const K& first;
          const V& second;
        };
        Proxy operator->() const { return Proxy{ buckets_[index_].key, buckets_[index_].value }; }
      };

      ConstIterator begin() const { return ConstIterator(buckets_, capacity_, 0); }
      ConstIterator end() const { return ConstIterator(buckets_, capacity_, capacity_); }

      // Iterator-returning find methods (for compatibility with std::unordered_map)
      Iterator find(const K& key) {
        if (size_ == 0) return end();

        size_t index = hash_key(key);
        for (size_t i = 0; i < capacity_; ++i) {
          size_t probe = (index + i) % capacity_;

          if (!buckets_[probe].occupied && !buckets_[probe].deleted) {
            return end();
          }

          if (buckets_[probe].deleted) continue;

          if (buckets_[probe].key == key) {
            return Iterator(buckets_, capacity_, probe, false);  // Don't advance
          }
        }
        return end();
      }

      ConstIterator find(const K& key) const {
        if (size_ == 0) return end();

        size_t index = hash_key(key);
        for (size_t i = 0; i < capacity_; ++i) {
          size_t probe = (index + i) % capacity_;

          if (!buckets_[probe].occupied && !buckets_[probe].deleted) {
            return end();
          }

          if (buckets_[probe].deleted) continue;

          if (buckets_[probe].key == key) {
            return ConstIterator(buckets_, capacity_, probe, false);  // Don't advance
          }
        }
        return end();
      }
    };

    // ============================================================================
    // ManagedHashSet - Hash set without STL
    // ============================================================================

    template<typename K>
    class ManagedHashSet {
    private:
      struct Entry {
        K key;
        bool occupied;
        bool deleted;

        Entry() : occupied(false), deleted(false) {}
      };

      Entry* buckets_;
      size_t capacity_;
      size_t size_;
      size_t tombstone_count_;

      static constexpr size_t INITIAL_CAPACITY = 16;
      static constexpr float MAX_LOAD_FACTOR = 0.7f;

      size_t hash_key(const K& key) const {
        return std::hash<K>{}(key) % capacity_;
      }

      void rehash(size_t new_capacity) {
        Entry* old_buckets = buckets_;
        size_t old_capacity = capacity_;

        buckets_ = static_cast<Entry*>(std::malloc(new_capacity * sizeof(Entry)));
        for (size_t i = 0; i < new_capacity; ++i) {
          new (buckets_ + i) Entry();
        }

        capacity_ = new_capacity;
        size_ = 0;
        tombstone_count_ = 0;

        if (old_buckets) {
          for (size_t i = 0; i < old_capacity; ++i) {
            if (old_buckets[i].occupied && !old_buckets[i].deleted) {
              insert(std::move(old_buckets[i].key));
            }
            old_buckets[i].~Entry();
          }
          std::free(old_buckets);
        }
      }

      size_t find_slot(const K& key) const {
        size_t index = hash_key(key);
        size_t first_tombstone = capacity_;

        for (size_t i = 0; i < capacity_; ++i) {
          size_t probe = (index + i) % capacity_;

          if (!buckets_[probe].occupied && !buckets_[probe].deleted) {
            return (first_tombstone < capacity_) ? first_tombstone : probe;
          }

          if (buckets_[probe].deleted) {
            if (first_tombstone >= capacity_) first_tombstone = probe;
            continue;
          }

          if (buckets_[probe].key == key) {
            return probe;
          }
        }

        return (first_tombstone < capacity_) ? first_tombstone : capacity_;
      }

    public:
      ManagedHashSet() : buckets_(nullptr), capacity_(0), size_(0), tombstone_count_(0) {
        rehash(INITIAL_CAPACITY);
      }

      ~ManagedHashSet() {
        if (buckets_) {
          for (size_t i = 0; i < capacity_; ++i) {
            buckets_[i].~Entry();
          }
          std::free(buckets_);
        }
      }

      ManagedHashSet(const ManagedHashSet&) = delete;
      ManagedHashSet& operator=(const ManagedHashSet&) = delete;

      ManagedHashSet(ManagedHashSet&& other) noexcept
        : buckets_(other.buckets_), capacity_(other.capacity_)
        , size_(other.size_), tombstone_count_(other.tombstone_count_) {
        other.buckets_ = nullptr;
        other.capacity_ = 0;
        other.size_ = 0;
      }

      void insert(const K& key) {
        if ((size_ + tombstone_count_ + 1) > capacity_ * MAX_LOAD_FACTOR) {
          rehash(capacity_ * 2);
        }

        size_t slot = find_slot(key);
        if (slot >= capacity_) return;

        if (!buckets_[slot].occupied || buckets_[slot].deleted) {
          if (buckets_[slot].deleted) --tombstone_count_;
          buckets_[slot].key = key;
          buckets_[slot].occupied = true;
          buckets_[slot].deleted = false;
          ++size_;
        }
      }

      void insert(K&& key) {
        if ((size_ + tombstone_count_ + 1) > capacity_ * MAX_LOAD_FACTOR) {
          rehash(capacity_ * 2);
        }

        size_t slot = find_slot(key);
        if (slot >= capacity_) return;

        if (!buckets_[slot].occupied || buckets_[slot].deleted) {
          if (buckets_[slot].deleted) --tombstone_count_;
          buckets_[slot].key = std::move(key);
          buckets_[slot].occupied = true;
          buckets_[slot].deleted = false;
          ++size_;
        }
      }

      void erase(const K& key) {
        size_t slot = find_slot(key);
        if (slot < capacity_ && buckets_[slot].occupied && !buckets_[slot].deleted) {
          if (buckets_[slot].key == key) {
            buckets_[slot].deleted = true;
            --size_;
            ++tombstone_count_;
          }
        }
      }

      bool contains(const K& key) const {
        if (size_ == 0) return false;

        size_t index = hash_key(key);
        for (size_t i = 0; i < capacity_; ++i) {
          size_t probe = (index + i) % capacity_;

          if (!buckets_[probe].occupied && !buckets_[probe].deleted) {
            return false;
          }
          if (buckets_[probe].deleted) continue;
          if (buckets_[probe].key == key) return true;
        }
        return false;
      }

      // count method for compatibility with std::unordered_set
      size_t count(const K& key) const {
        return contains(key) ? 1 : 0;
      }

      size_t size() const { return size_; }
      bool empty() const { return size_ == 0; }

      void clear() {
        for (size_t i = 0; i < capacity_; ++i) {
          if (buckets_[i].occupied) {
            buckets_[i].~Entry();
            new (buckets_ + i) Entry();
          }
        }
        size_ = 0;
        tombstone_count_ = 0;
      }

      // Iterator
      class Iterator {
      private:
        Entry* buckets_;
        size_t capacity_;
        size_t index_;

        void advance_to_valid() {
          while (index_ < capacity_ &&
            (!buckets_[index_].occupied || buckets_[index_].deleted)) {
            ++index_;
          }
        }

      public:
        Iterator(Entry* b, size_t cap, size_t idx) : buckets_(b), capacity_(cap), index_(idx) {
          advance_to_valid();
        }

        const K& operator*() const { return buckets_[index_].key; }

        Iterator& operator++() {
          ++index_;
          advance_to_valid();
          return *this;
        }

        bool operator!=(const Iterator& other) const { return index_ != other.index_; }
      };

      Iterator begin() { return Iterator(buckets_, capacity_, 0); }
      Iterator end() { return Iterator(buckets_, capacity_, capacity_); }
    };

    // ============================================================================
    // ManagedUniquePtr - Simple unique pointer without STL
    // ============================================================================

    template<typename T>
    class ManagedUniquePtr {
    private:
      T* ptr_;

    public:
      ManagedUniquePtr() : ptr_(nullptr) {}
      explicit ManagedUniquePtr(T* p) : ptr_(p) {}

      ~ManagedUniquePtr() {
        if (ptr_) {
          ptr_->~T();
          std::free(ptr_);
        }
      }

      // Disable copy
      ManagedUniquePtr(const ManagedUniquePtr&) = delete;
      ManagedUniquePtr& operator=(const ManagedUniquePtr&) = delete;

      // Enable move
      ManagedUniquePtr(ManagedUniquePtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
      }

      ManagedUniquePtr& operator=(ManagedUniquePtr&& other) noexcept {
        if (this != &other) {
          if (ptr_) {
            ptr_->~T();
            std::free(ptr_);
          }
          ptr_ = other.ptr_;
          other.ptr_ = nullptr;
        }
        return *this;
      }

      T* get() const { return ptr_; }
      T* operator->() const { return ptr_; }
      T& operator*() const { return *ptr_; }

      explicit operator bool() const { return ptr_ != nullptr; }

      T* release() {
        T* tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
      }

      void reset(T* p = nullptr) {
        if (ptr_) {
          ptr_->~T();
          std::free(ptr_);
        }
        ptr_ = p;
      }

      template<typename... Args>
      static ManagedUniquePtr<T> make(Args&&... args) {
        void* mem = std::malloc(sizeof(T));
        if (!mem) return ManagedUniquePtr<T>(nullptr);
        return ManagedUniquePtr<T>(new (mem) T(std::forward<Args>(args)...));
      }
    };

    // Helper function similar to std::make_unique
    template<typename T, typename... Args>
    ManagedUniquePtr<T> make_managed(Args&&... args) {
      return ManagedUniquePtr<T>::make(std::forward<Args>(args)...);
    }

    // ============================================================================
    // ManagedFixedArray - Fixed-size array allocated on heap
    // ============================================================================

    template<typename T, size_t N>
    class ManagedFixedArray {
    private:
      T data_[N];

    public:
      ManagedFixedArray() {
        for (size_t i = 0; i < N; ++i) {
          new (data_ + i) T();
        }
      }

      ~ManagedFixedArray() {
        for (size_t i = 0; i < N; ++i) {
          data_[i].~T();
        }
      }

      T& operator[](size_t index) { return data_[index]; }
      const T& operator[](size_t index) const { return data_[index]; }

      constexpr size_t size() const { return N; }

      T* begin() { return data_; }
      T* end() { return data_ + N; }
      const T* begin() const { return data_; }
      const T* end() const { return data_ + N; }
    };

  } // namespace ECS
} // namespace Engine
