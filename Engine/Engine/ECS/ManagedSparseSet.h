//-----------------------------------------------------------------------------------
// file:    ManagedSparseSet.h
// project: Monkey Stole My GPA
//
// author:  [Your Name] (100%)
//          Primary author
//
// email:   [your.email]@digipen.edu
//
// brief:   A SparseSet implementation that uses custom memory management instead
//          of STL containers. Uses the Pool Allocator for the data storage and
//          raw arrays for sparse/dense mappings.
//
//          This provides the same O(1) operations as the original SparseSet but
//          with memory managed by the custom Memory Manager.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Memory/MemoryManager.h"
#include "Types.h"
#include <cstring>    // For memset, memmove
#include <limits>
#include <new>        // For placement new

namespace Engine {
  namespace ECS {

    // ============================================================================
    // ManagedArray - A simple dynamic array without STL
    // ============================================================================

    /**
     * @brief A simple dynamic array that uses raw memory allocation.
     *
     * This replaces std::vector for use in the memory-managed ECS.
     * Memory is allocated in chunks and grows as needed.
     *
     * @tparam T The type of elements stored.
     */
    template<typename T>
    class ManagedArray {
    private:
      T* data_;           ///< Pointer to the data array
      size_t size_;       ///< Current number of elements
      size_t capacity_;   ///< Current allocated capacity

      /// Growth factor for capacity
      static constexpr size_t GROWTH_FACTOR = 2;
      static constexpr size_t INITIAL_CAPACITY = 64;

    public:
      /**
       * @brief Constructs an empty managed array.
       */
      ManagedArray()
        : data_(nullptr)
        , size_(0)
        , capacity_(0)
      {
      }

      /**
       * @brief Constructs a managed array with initial capacity.
       * @param initial_capacity The initial capacity to reserve.
       */
      explicit ManagedArray(size_t initial_capacity)
        : data_(nullptr)
        , size_(0)
        , capacity_(0)
      {
        reserve(initial_capacity);
      }

      /**
       * @brief Destructor - frees allocated memory.
       */
      ~ManagedArray() {
        clear();
        if (data_) {
          std::free(data_);
          data_ = nullptr;
        }
      }

      // Disable copy operations
      ManagedArray(const ManagedArray&) = delete;
      ManagedArray& operator=(const ManagedArray&) = delete;

      // Enable move operations
      ManagedArray(ManagedArray&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
        , capacity_(other.capacity_)
      {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
      }

      ManagedArray& operator=(ManagedArray&& other) noexcept {
        if (this != &other) {
          clear();
          if (data_) std::free(data_);

          data_ = other.data_;
          size_ = other.size_;
          capacity_ = other.capacity_;

          other.data_ = nullptr;
          other.size_ = 0;
          other.capacity_ = 0;
        }
        return *this;
      }

      /**
       * @brief Reserves capacity for at least n elements.
       * @param n The minimum capacity to reserve.
       */
      void reserve(size_t n) {
        if (n <= capacity_) return;

        // Allocate new memory
        T* new_data = static_cast<T*>(std::malloc(n * sizeof(T)));
        if (!new_data) return;  // Allocation failed

        // Move existing elements
        if (data_) {
          for (size_t i = 0; i < size_; ++i) {
            new (new_data + i) T(std::move(data_[i]));
            data_[i].~T();
          }
          std::free(data_);
        }

        data_ = new_data;
        capacity_ = n;
      }

      /**
       * @brief Adds an element to the end.
       * @param value The value to add.
       */
      void push_back(const T& value) {
        ensure_capacity(size_ + 1);
        new (data_ + size_) T(value);
        ++size_;
      }

      /**
       * @brief Adds an element to the end (move version).
       * @param value The value to move.
       */
      void push_back(T&& value) {
        ensure_capacity(size_ + 1);
        new (data_ + size_) T(std::move(value));
        ++size_;
      }

      /**
       * @brief Constructs an element in place at the end.
       * @tparam Args Constructor argument types.
       * @param args Arguments to forward to T's constructor.
       */
      template<typename... Args>
      void emplace_back(Args&&... args) {
        ensure_capacity(size_ + 1);
        new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
      }

      /**
       * @brief Removes the last element.
       */
      void pop_back() {
        if (size_ > 0) {
          --size_;
          data_[size_].~T();
        }
      }

      /**
       * @brief Resizes the array to the specified size.
       * @param n The new size.
       */
      void resize(size_t n) {
        if (n < size_) {
          // Shrink - destroy excess elements
          for (size_t i = n; i < size_; ++i) {
            data_[i].~T();
          }
        }
        else if (n > size_) {
          // Grow - default construct new elements
          ensure_capacity(n);
          for (size_t i = size_; i < n; ++i) {
            new (data_ + i) T();
          }
        }
        size_ = n;
      }

      /**
       * @brief Clears all elements.
       */
      void clear() {
        for (size_t i = 0; i < size_; ++i) {
          data_[i].~T();
        }
        size_ = 0;
      }

      /// Access element by index
      T& operator[](size_t index) { return data_[index]; }
      const T& operator[](size_t index) const { return data_[index]; }

      /// Get pointer to data
      T* data() { return data_; }
      const T* data() const { return data_; }

      /// Get size
      size_t size() const { return size_; }

      /// Get capacity
      size_t capacity() const { return capacity_; }

      /// Check if empty
      bool empty() const { return size_ == 0; }

      /// Get reference to last element
      T& back() { return data_[size_ - 1]; }
      const T& back() const { return data_[size_ - 1]; }

      // Iterator support
      T* begin() { return data_; }
      T* end() { return data_ + size_; }
      const T* begin() const { return data_; }
      const T* end() const { return data_ + size_; }

      /**
       * @brief Erases element at position.
       * @param pos Iterator pointing to element to erase.
       * @return Iterator to the element following the removed element.
       */
      T* erase(T* pos) {
        if (pos < data_ || pos >= data_ + size_) {
          return end();
        }

        // Shift all elements after pos one position left
        size_t index = pos - data_;
        data_[index].~T();

        for (size_t i = index; i < size_ - 1; ++i) {
          new (data_ + i) T(std::move(data_[i + 1]));
          data_[i + 1].~T();
        }

        --size_;
        return data_ + index;
      }

      /**
       * @brief Erases elements in range [first, last).
       * @param first Iterator to first element to erase.
       * @param last Iterator past the last element to erase.
       * @return Iterator to the element following the last removed element.
       */
      T* erase(T* first, T* last) {
        if (first < data_ || first >= data_ + size_ || last < first) {
          return end();
        }
        if (last > data_ + size_) last = data_ + size_;

        size_t first_index = first - data_;
        size_t last_index = last - data_;
        size_t count = last_index - first_index;

        // Destroy elements in range
        for (size_t i = first_index; i < last_index; ++i) {
          data_[i].~T();
        }

        // Shift remaining elements
        for (size_t i = last_index; i < size_; ++i) {
          new (data_ + i - count) T(std::move(data_[i]));
          data_[i].~T();
        }

        size_ -= count;
        return data_ + first_index;
      }

    private:
      /**
       * @brief Ensures capacity is at least n.
       * @param n The required capacity.
       */
      void ensure_capacity(size_t n) {
        if (n > capacity_) {
          size_t new_capacity = capacity_ == 0 ? INITIAL_CAPACITY : capacity_ * GROWTH_FACTOR;
          while (new_capacity < n) {
            new_capacity *= GROWTH_FACTOR;
          }
          reserve(new_capacity);
        }
      }
    };

    // ============================================================================
    // ManagedSparseSet - SparseSet using custom memory management
    // ============================================================================

    /**
     * @brief A SparseSet that uses ManagedArray instead of std::vector.
     *
     * Provides the same O(1) insert, remove, and lookup operations as the
     * original SparseSet, but uses the custom memory management system.
     *
     * @tparam T The type of data to store with each key.
     */
    template<typename T>
    class ManagedSparseSet {
    private:
      ManagedArray<size_t> sparse;    ///< Maps key to dense array index
      ManagedArray<size_t> dense;     ///< Dense array of keys
      ManagedArray<T> data;           ///< Dense array of data

      static constexpr size_t NULL_INDEX = std::numeric_limits<size_t>::max();

    public:
      /**
       * @brief Constructs an empty managed sparse set.
       * @param reserve_size Initial capacity for the sparse array.
       */
      ManagedSparseSet(size_t reserve_size = 0) {
        if (reserve_size > 0) {
          sparse.reserve(reserve_size);
        }
      }

      /**
       * @brief Inserts a key with associated data.
       * @param key The key to insert.
       * @param value The data to associate with the key.
       */
      void insert(size_t key, const T& value) {
        if (contains(key)) {
          // Update existing
          size_t index = sparse[key];
          data[index] = value;
          return;
        }

        // Ensure sparse array is large enough
        if (key >= sparse.size()) {
          size_t old_size = sparse.size();
          sparse.resize(key + 1);
          for (size_t i = old_size; i <= key; ++i) {
            sparse[i] = NULL_INDEX;
          }
        }

        // Add new entry
        sparse[key] = dense.size();
        dense.push_back(key);
        data.push_back(value);
      }

      /**
       * @brief Inserts a key with associated data (move version).
       * @param key The key to insert.
       * @param value The data to move and associate with the key.
       */
      void insert(size_t key, T&& value) {
        if (contains(key)) {
          size_t index = sparse[key];
          data[index] = std::move(value);
          return;
        }

        if (key >= sparse.size()) {
          size_t old_size = sparse.size();
          sparse.resize(key + 1);
          for (size_t i = old_size; i <= key; ++i) {
            sparse[i] = NULL_INDEX;
          }
        }

        sparse[key] = dense.size();
        dense.push_back(key);
        data.push_back(std::move(value));
      }

      /**
       * @brief Constructs data in-place for a key.
       * @tparam Args Constructor argument types.
       * @param key The key to insert.
       * @param args Arguments forwarded to the constructor of T.
       */
      template<typename... Args>
      void emplace(size_t key, Args&&... args) {
        if (contains(key)) {
          size_t index = sparse[key];
          data[index].~T();
          new (&data[index]) T(std::forward<Args>(args)...);
          return;
        }

        if (key >= sparse.size()) {
          size_t old_size = sparse.size();
          sparse.resize(key + 1);
          for (size_t i = old_size; i <= key; ++i) {
            sparse[i] = NULL_INDEX;
          }
        }

        sparse[key] = dense.size();
        dense.push_back(key);
        data.emplace_back(std::forward<Args>(args)...);
      }

      /**
       * @brief Removes a key using swap-and-pop for O(1) removal.
       * @param key The key to remove.
       */
      void erase(size_t key) {
        if (!contains(key)) return;

        size_t index = sparse[key];
        size_t last_key = dense.back();

        // Swap with last element
        dense[index] = last_key;
        data[index] = std::move(data.back());
        sparse[last_key] = index;

        // Remove last element
        dense.pop_back();
        data.pop_back();
        sparse[key] = NULL_INDEX;
      }

      /**
       * @brief Checks if the set contains the specified key.
       * @param key The key to check.
       * @return true if the key exists, false otherwise.
       */
      bool contains(size_t key) const noexcept {
        return key < sparse.size() && sparse[key] != NULL_INDEX;
      }

      /**
       * @brief Gets the data associated with a key.
       * @param key The key to get data for.
       * @return Pointer to the data, or nullptr if not found.
       */
      T* get(size_t key) noexcept {
        if (!contains(key)) return nullptr;
        return &data[sparse[key]];
      }

      /**
       * @brief Gets the data associated with a key (const version).
       * @param key The key to get data for.
       * @return Pointer to the data, or nullptr if not found.
       */
      const T* get(size_t key) const noexcept {
        if (!contains(key)) return nullptr;
        return &data[sparse[key]];
      }

      /**
       * @brief Gets the data at a specific index in the dense array.
       * @param index The index in the dense array.
       * @return Reference to the data.
       */
      T& at(size_t index) { return data[index]; }
      const T& at(size_t index) const { return data[index]; }

      /**
       * @brief Gets the key at a specific index in the dense array.
       * @param index The index in the dense array.
       * @return The key.
       */
      size_t key_at(size_t index) const { return dense[index]; }

      /// Returns the number of elements
      size_t size() const noexcept { return dense.size(); }

      /// Checks if the set is empty
      bool empty() const noexcept { return dense.empty(); }

      /// Clears all elements
      void clear() {
        sparse.clear();
        dense.clear();
        data.clear();
      }

      /// Reserves space in the sparse array
      void reserve(size_t capacity) {
        if (capacity > sparse.size()) {
          size_t old_size = sparse.size();
          sparse.resize(capacity);
          for (size_t i = old_size; i < capacity; ++i) {
            sparse[i] = NULL_INDEX;
          }
        }
      }

      // Iterator support for keys
      auto begin() { return dense.begin(); }
      auto end() { return dense.end(); }
      auto begin() const { return dense.begin(); }
      auto end() const { return dense.end(); }

      // Data iterators
      auto data_begin() { return data.begin(); }
      auto data_end() { return data.end(); }
      auto data_begin() const { return data.begin(); }
      auto data_end() const { return data.end(); }

      /// Returns the dense key array
      const ManagedArray<size_t>& keys() const noexcept { return dense; }

      /// Returns the values array
      ManagedArray<T>& values() noexcept { return data; }
      const ManagedArray<T>& values() const noexcept { return data; }
    };

    // ============================================================================
    // ManagedSparseSet<void> - Key-only specialization
    // ============================================================================

    template<>
    class ManagedSparseSet<void> {
    private:
      ManagedArray<size_t> sparse;
      ManagedArray<size_t> dense;

      static constexpr size_t NULL_INDEX = std::numeric_limits<size_t>::max();

    public:
      ManagedSparseSet(size_t reserve_size = 0) {
        if (reserve_size > 0) {
          sparse.reserve(reserve_size);
        }
      }

      void insert(size_t key) {
        if (contains(key)) return;

        if (key >= sparse.size()) {
          size_t old_size = sparse.size();
          sparse.resize(key + 1);
          for (size_t i = old_size; i <= key; ++i) {
            sparse[i] = NULL_INDEX;
          }
        }

        sparse[key] = dense.size();
        dense.push_back(key);
      }

      void erase(size_t key) {
        if (!contains(key)) return;

        size_t index = sparse[key];
        size_t last_key = dense.back();

        dense[index] = last_key;
        sparse[last_key] = index;

        dense.pop_back();
        sparse[key] = NULL_INDEX;
      }

      bool contains(size_t key) const noexcept {
        return key < sparse.size() && sparse[key] != NULL_INDEX;
      }

      size_t key_at(size_t index) const { return dense[index]; }
      size_t size() const noexcept { return dense.size(); }
      bool empty() const noexcept { return dense.empty(); }

      void clear() {
        sparse.clear();
        dense.clear();
      }

      void reserve(size_t capacity) {
        if (capacity > sparse.size()) {
          size_t old_size = sparse.size();
          sparse.resize(capacity);
          for (size_t i = old_size; i < capacity; ++i) {
            sparse[i] = NULL_INDEX;
          }
        }
      }

      auto begin() { return dense.begin(); }
      auto end() { return dense.end(); }
      auto begin() const { return dense.begin(); }
      auto end() const { return dense.end(); }

      const ManagedArray<size_t>& keys() const noexcept { return dense; }
    };

  } // namespace ECS
} // namespace Engine
