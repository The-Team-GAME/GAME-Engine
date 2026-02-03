//-----------------------------------------------------------------------------------
// file:    MCSparseSet.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Custom sparse set container that uses the Memory Manager for all
//          allocations. Provides O(1) insertion, deletion, and lookup operations
//          while maintaining dense iteration. Replaces the std::vector-based
//          SparseSet in the ECS. No STL containers used internally.
//
//          MC prefix = Memory Containers (to distinguish from original SparseSet)
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Array.h"
#include "../MemoryConfig.h"
#include <limits>
#include <type_traits>

namespace Engine {
    namespace Memory {

        //=================================================================================
        // MCSparseSet<T> - Custom sparse set using Memory Manager
        //
        // A sparse set provides O(1) insertion, deletion, and lookup while maintaining
        // cache-friendly dense iteration. It uses two arrays:
        // - Sparse array: Maps keys to indices in the dense array (can be sparse)
        // - Dense array: Contains keys (and optionally data) in contiguous memory
        //
        // Key Benefits:
        // - O(1) insert, erase, and contains operations
        // - Cache-friendly iteration (dense array is contiguous)
        // - No memory fragmentation (uses Memory Manager pools)
        //
        // Memory Layout:
        // Sparse: [?, 2, ?, 0, 1, ?, ...]  // sparse[key] = index in dense
        // Dense:  [3, 4, 1]                 // The keys in insertion order
        // Data:   [D3, D4, D1]              // Data associated with each key
        //=================================================================================
        template<typename T = void>
        class MCSparseSet {
        public:
            static constexpr size_t NULL_INDEX = std::numeric_limits<size_t>::max();

            //=================================================================================
            // Constructors / Destructor
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Default constructor - creates an empty sparse set.

            Parameters:
            None.

            Where to call it:
            When declaring a sparse set.
            ==================================================================================*/
            MCSparseSet();

            /*=================================================================================
            Function Brief:
            Constructs a sparse set with reserved capacity.

            Parameters:
            reserve_size - Initial capacity for the sparse array.

            Where to call it:
            When you know the maximum key value in advance.
            ==================================================================================*/
            explicit MCSparseSet(size_t reserve_size);

            /*=================================================================================
            Function Brief:
            Copy constructor.

            Parameters:
            other - Sparse set to copy from.

            Where to call it:
            When a copy is needed.
            ==================================================================================*/
            MCSparseSet(const MCSparseSet& other);

            /*=================================================================================
            Function Brief:
            Move constructor.

            Parameters:
            other - Sparse set to move from.

            Where to call it:
            When transferring ownership.
            ==================================================================================*/
            MCSparseSet(MCSparseSet&& other) noexcept;

            /*=================================================================================
            Function Brief:
            Destructor.

            Parameters:
            None.

            Where to call it:
            Automatically called when set goes out of scope.
            ==================================================================================*/
            ~MCSparseSet() = default;

            //=================================================================================
            // Assignment
            //=================================================================================

            MCSparseSet& operator=(const MCSparseSet& other);
            MCSparseSet& operator=(MCSparseSet&& other) noexcept;

            //=================================================================================
            // Modifiers
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Inserts a key into the sparse set. If the key already exists, does nothing.

            Parameters:
            key - The key to insert.

            Where to call it:
            When adding a key to the set.
            ==================================================================================*/
            void insert(size_t key);

            /*=================================================================================
            Function Brief:
            Inserts a key with associated data into the sparse set.

            Parameters:
            key - The key to insert.
            value - The data to associate with the key.

            Where to call it:
            When adding a key with data.
            ==================================================================================*/
            void insert(size_t key, const T& value);

            /*=================================================================================
            Function Brief:
            Inserts a key with associated data (move version).

            Parameters:
            key - The key to insert.
            value - The data to move.

            Where to call it:
            When adding a key with data using move semantics.
            ==================================================================================*/
            void insert(size_t key, T&& value);

            /*=================================================================================
            Function Brief:
            Constructs data in-place for a key.

            Parameters:
            key - The key to insert.
            args - Arguments forwarded to T's constructor.

            Where to call it:
            When constructing data directly.
            ==================================================================================*/
            template<typename... Args>
            void emplace(size_t key, Args&&... args);

            /*=================================================================================
            Function Brief:
            Removes a key from the sparse set using swap-and-pop for O(1) removal.

            Parameters:
            key - The key to remove.

            Where to call it:
            When removing a key from the set.
            ==================================================================================*/
            void erase(size_t key);

            /*=================================================================================
            Function Brief:
            Removes all elements from the set.

            Parameters:
            None.

            Where to call it:
            When clearing the set.
            ==================================================================================*/
            void clear();

            //=================================================================================
            // Lookup
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Checks if the sparse set contains the specified key.

            Parameters:
            key - The key to check.

            Returns:
            true if the key exists.

            Where to call it:
            When checking for key existence.
            ==================================================================================*/
            bool contains(size_t key) const noexcept;

            /*=================================================================================
            Function Brief:
            Gets the data associated with a key. Returns nullptr if not found.

            Parameters:
            key - The key to look up.

            Returns:
            Pointer to the data, or nullptr.

            Where to call it:
            When accessing data by key.
            ==================================================================================*/
            T* get(size_t key) noexcept;
            const T* get(size_t key) const noexcept;

            /*=================================================================================
            Function Brief:
            Gets the data at a specific index in the dense array.

            Parameters:
            index - The index in the dense array.

            Returns:
            Reference to the data.

            Where to call it:
            When iterating over data.
            ==================================================================================*/
            T& at(size_t index);
            const T& at(size_t index) const;

            /*=================================================================================
            Function Brief:
            Gets the key at a specific index in the dense array.

            Parameters:
            index - The index in the dense array.

            Returns:
            The key.

            Where to call it:
            When iterating over keys.
            ==================================================================================*/
            size_t key_at(size_t index) const;

            //=================================================================================
            // Capacity
            //=================================================================================

            size_t size() const noexcept { return m_dense.size(); }
            bool empty() const noexcept { return m_dense.empty(); }

            /*=================================================================================
            Function Brief:
            Reserves space in the sparse array.

            Parameters:
            capacity - The number of keys to reserve space for.

            Where to call it:
            When you know the maximum key in advance.
            ==================================================================================*/
            void reserve(size_t capacity);

            //=================================================================================
            // Iteration
            //=================================================================================

            // Iterates over keys
            auto begin() noexcept { return m_dense.begin(); }
            auto end() noexcept { return m_dense.end(); }
            auto begin() const noexcept { return m_dense.begin(); }
            auto end() const noexcept { return m_dense.end(); }

            // Access to underlying arrays
            const Array<size_t>& keys() const noexcept { return m_dense; }
            const Array<T>& values() const noexcept requires (!std::is_same_v<T, void>) { return m_data; }
            Array<T>& values() noexcept requires (!std::is_same_v<T, void>) { return m_data; }

            //=================================================================================
            // Range operations
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Inserts a range of keys.

            Parameters:
            begin - Iterator to the beginning of the range.
            end - Iterator to the end of the range.

            Where to call it:
            When bulk inserting keys.
            ==================================================================================*/
            template<typename Iterator>
            void insert_range(Iterator begin, Iterator end);

            /*=================================================================================
            Function Brief:
            Removes all keys that satisfy a predicate.

            Parameters:
            predicate - Function that returns true for keys to remove.

            Where to call it:
            When conditionally removing elements.
            ==================================================================================*/
            template<typename Predicate>
            void erase_if(Predicate predicate);

            /*=================================================================================
            Function Brief:
            Sorts the dense array using a custom comparator.

            Parameters:
            compare - Comparator function.

            Where to call it:
            When sorting keys by some criteria.
            ==================================================================================*/
            template<typename Compare>
            void sort(Compare compare);

        private:
            Array<size_t> m_sparse;  // Maps key to dense index
            Array<size_t> m_dense;   // Dense array of keys
            Array<T> m_data;         // Dense array of data (if T != void)
        };

        //=================================================================================
        // Specialization for void (key-only, no data)
        //=================================================================================
        template<>
        class MCSparseSet<void> {
        public:
            static constexpr size_t NULL_INDEX = std::numeric_limits<size_t>::max();

            MCSparseSet() = default;
            explicit MCSparseSet(size_t reserve_size);
            MCSparseSet(const MCSparseSet& other) = default;
            MCSparseSet(MCSparseSet&& other) noexcept = default;
            ~MCSparseSet() = default;

            MCSparseSet& operator=(const MCSparseSet& other) = default;
            MCSparseSet& operator=(MCSparseSet&& other) noexcept = default;

            void insert(size_t key);
            void erase(size_t key);
            void clear();

            bool contains(size_t key) const noexcept;
            size_t key_at(size_t index) const;

            size_t size() const noexcept { return m_dense.size(); }
            bool empty() const noexcept { return m_dense.empty(); }
            void reserve(size_t capacity);

            auto begin() noexcept { return m_dense.begin(); }
            auto end() noexcept { return m_dense.end(); }
            auto begin() const noexcept { return m_dense.begin(); }
            auto end() const noexcept { return m_dense.end(); }

            const Array<size_t>& keys() const noexcept { return m_dense; }

            template<typename Iterator>
            void insert_range(Iterator begin, Iterator end);

            template<typename Predicate>
            void erase_if(Predicate predicate);

            template<typename Compare>
            void sort(Compare compare);

        private:
            Array<size_t> m_sparse;
            Array<size_t> m_dense;
        };

        //=================================================================================
        // Template Implementation - MCSparseSet<T>
        //=================================================================================

        template<typename T>
        MCSparseSet<T>::MCSparseSet() = default;

        template<typename T>
        MCSparseSet<T>::MCSparseSet(size_t reserve_size) {
            if (reserve_size > 0) {
                m_sparse.resize(reserve_size, NULL_INDEX);
            }
        }

        template<typename T>
        MCSparseSet<T>::MCSparseSet(const MCSparseSet& other)
            : m_sparse(other.m_sparse)
            , m_dense(other.m_dense)
            , m_data(other.m_data)
        {
        }

        template<typename T>
        MCSparseSet<T>::MCSparseSet(MCSparseSet&& other) noexcept
            : m_sparse(std::move(other.m_sparse))
            , m_dense(std::move(other.m_dense))
            , m_data(std::move(other.m_data))
        {
        }

        template<typename T>
        MCSparseSet<T>& MCSparseSet<T>::operator=(const MCSparseSet& other) {
            if (this != &other) {
                m_sparse = other.m_sparse;
                m_dense = other.m_dense;
                m_data = other.m_data;
            }
            return *this;
        }

        template<typename T>
        MCSparseSet<T>& MCSparseSet<T>::operator=(MCSparseSet&& other) noexcept {
            if (this != &other) {
                m_sparse = std::move(other.m_sparse);
                m_dense = std::move(other.m_dense);
                m_data = std::move(other.m_data);
            }
            return *this;
        }

        template<typename T>
        void MCSparseSet<T>::insert(size_t key) {
            if (contains(key)) return;

            // Expand sparse array if needed
            if (key >= m_sparse.size()) {
                m_sparse.resize(key + 1, NULL_INDEX);
            }

            m_sparse[key] = m_dense.size();
            m_dense.push_back(key);

            if constexpr (!std::is_same_v<T, size_t>) {
                m_data.emplace_back();
            }
        }

        template<typename T>
        void MCSparseSet<T>::insert(size_t key, const T& value) {
            if (contains(key)) {
                // Update existing value
                if constexpr (!std::is_same_v<T, size_t>) {
                    m_data[m_sparse[key]] = value;
                }
                return;
            }

            if (key >= m_sparse.size()) {
                m_sparse.resize(key + 1, NULL_INDEX);
            }

            m_sparse[key] = m_dense.size();
            m_dense.push_back(key);

            if constexpr (!std::is_same_v<T, size_t>) {
                m_data.push_back(value);
            }
        }

        template<typename T>
        void MCSparseSet<T>::insert(size_t key, T&& value) {
            if (contains(key)) {
                if constexpr (!std::is_same_v<T, size_t>) {
                    m_data[m_sparse[key]] = std::move(value);
                }
                return;
            }

            if (key >= m_sparse.size()) {
                m_sparse.resize(key + 1, NULL_INDEX);
            }

            m_sparse[key] = m_dense.size();
            m_dense.push_back(key);

            if constexpr (!std::is_same_v<T, size_t>) {
                m_data.push_back(std::move(value));
            }
        }

        template<typename T>
        template<typename... Args>
        void MCSparseSet<T>::emplace(size_t key, Args&&... args) {
            if (contains(key)) {
                if constexpr (!std::is_same_v<T, size_t>) {
                    m_data[m_sparse[key]] = T(std::forward<Args>(args)...);
                }
                return;
            }

            if (key >= m_sparse.size()) {
                m_sparse.resize(key + 1, NULL_INDEX);
            }

            m_sparse[key] = m_dense.size();
            m_dense.push_back(key);

            if constexpr (!std::is_same_v<T, size_t>) {
                m_data.emplace_back(std::forward<Args>(args)...);
            }
        }

        template<typename T>
        void MCSparseSet<T>::erase(size_t key) {
            if (!contains(key)) return;

            // Swap-and-pop removal for O(1) deletion
            size_t index = m_sparse[key];
            size_t last_key = m_dense.back();

            // Swap with last element
            m_dense[index] = last_key;
            m_sparse[last_key] = index;

            if constexpr (!std::is_same_v<T, size_t>) {
                m_data[index] = std::move(m_data.back());
                m_data.pop_back();
            }

            m_dense.pop_back();
            m_sparse[key] = NULL_INDEX;
        }

        template<typename T>
        void MCSparseSet<T>::clear() {
            // Reset sparse array entries
            for (size_t i = 0; i < m_dense.size(); ++i) {
                if (m_dense[i] < m_sparse.size()) {
                    m_sparse[m_dense[i]] = NULL_INDEX;
                }
            }
            m_dense.clear();
            if constexpr (!std::is_same_v<T, size_t>) {
                m_data.clear();
            }
        }

        template<typename T>
        bool MCSparseSet<T>::contains(size_t key) const noexcept {
            return key < m_sparse.size() && m_sparse[key] != NULL_INDEX;
        }

        template<typename T>
        T* MCSparseSet<T>::get(size_t key) noexcept {
            if (!contains(key)) return nullptr;
            if constexpr (std::is_same_v<T, size_t>) {
                return nullptr;
            }
            else {
                return &m_data[m_sparse[key]];
            }
        }

        template<typename T>
        const T* MCSparseSet<T>::get(size_t key) const noexcept {
            if (!contains(key)) return nullptr;
            if constexpr (std::is_same_v<T, size_t>) {
                return nullptr;
            }
            else {
                return &m_data[m_sparse[key]];
            }
        }

        template<typename T>
        T& MCSparseSet<T>::at(size_t index) {
            if constexpr (std::is_same_v<T, size_t>) {
                // Return reference to dense array element as a hack
                return reinterpret_cast<T&>(m_dense[index]);
            }
            else {
                return m_data[index];
            }
        }

        template<typename T>
        const T& MCSparseSet<T>::at(size_t index) const {
            if constexpr (std::is_same_v<T, size_t>) {
                return reinterpret_cast<const T&>(m_dense[index]);
            }
            else {
                return m_data[index];
            }
        }

        template<typename T>
        size_t MCSparseSet<T>::key_at(size_t index) const {
            return m_dense[index];
        }

        template<typename T>
        void MCSparseSet<T>::reserve(size_t capacity) {
            if (capacity > m_sparse.size()) {
                m_sparse.resize(capacity, NULL_INDEX);
            }
            m_dense.reserve(capacity);
            if constexpr (!std::is_same_v<T, size_t>) {
                m_data.reserve(capacity);
            }
        }

        template<typename T>
        template<typename Iterator>
        void MCSparseSet<T>::insert_range(Iterator begin, Iterator end) {
            for (auto it = begin; it != end; ++it) {
                insert(*it);
            }
        }

        template<typename T>
        template<typename Predicate>
        void MCSparseSet<T>::erase_if(Predicate predicate) {
            for (size_t i = m_dense.size(); i-- > 0; ) {
                if (predicate(m_dense[i])) {
                    erase(m_dense[i]);
                }
            }
        }

        template<typename T>
        template<typename Compare>
        void MCSparseSet<T>::sort(Compare compare) {
            // Simple insertion sort for small sets, or can implement quicksort
            // For now, bubble sort for simplicity (can optimize later)
            for (size_t i = 0; i < m_dense.size(); ++i) {
                for (size_t j = i + 1; j < m_dense.size(); ++j) {
                    if (compare(m_dense[j], m_dense[i])) {
                        // Swap
                        size_t temp_key = m_dense[i];
                        m_dense[i] = m_dense[j];
                        m_dense[j] = temp_key;

                        if constexpr (!std::is_same_v<T, size_t>) {
                            T temp_data = std::move(m_data[i]);
                            m_data[i] = std::move(m_data[j]);
                            m_data[j] = std::move(temp_data);
                        }
                    }
                }
            }

            // Update sparse array
            for (size_t i = 0; i < m_dense.size(); ++i) {
                m_sparse[m_dense[i]] = i;
            }
        }

        //=================================================================================
        // MCSparseSet<void> Implementation
        //=================================================================================

        inline MCSparseSet<void>::MCSparseSet(size_t reserve_size) {
            if (reserve_size > 0) {
                m_sparse.resize(reserve_size, NULL_INDEX);
            }
        }

        inline void MCSparseSet<void>::insert(size_t key) {
            if (contains(key)) return;

            if (key >= m_sparse.size()) {
                m_sparse.resize(key + 1, NULL_INDEX);
            }

            m_sparse[key] = m_dense.size();
            m_dense.push_back(key);
        }

        inline void MCSparseSet<void>::erase(size_t key) {
            if (!contains(key)) return;

            size_t index = m_sparse[key];
            size_t last_key = m_dense.back();

            m_dense[index] = last_key;
            m_sparse[last_key] = index;

            m_dense.pop_back();
            m_sparse[key] = NULL_INDEX;
        }

        inline void MCSparseSet<void>::clear() {
            for (size_t i = 0; i < m_dense.size(); ++i) {
                if (m_dense[i] < m_sparse.size()) {
                    m_sparse[m_dense[i]] = NULL_INDEX;
                }
            }
            m_dense.clear();
        }

        inline bool MCSparseSet<void>::contains(size_t key) const noexcept {
            return key < m_sparse.size() && m_sparse[key] != NULL_INDEX;
        }

        inline size_t MCSparseSet<void>::key_at(size_t index) const {
            return m_dense[index];
        }

        inline void MCSparseSet<void>::reserve(size_t capacity) {
            if (capacity > m_sparse.size()) {
                m_sparse.resize(capacity, NULL_INDEX);
            }
            m_dense.reserve(capacity);
        }

        template<typename Iterator>
        void MCSparseSet<void>::insert_range(Iterator begin, Iterator end) {
            for (auto it = begin; it != end; ++it) {
                insert(*it);
            }
        }

        template<typename Predicate>
        void MCSparseSet<void>::erase_if(Predicate predicate) {
            for (size_t i = m_dense.size(); i-- > 0; ) {
                if (predicate(m_dense[i])) {
                    erase(m_dense[i]);
                }
            }
        }

        template<typename Compare>
        void MCSparseSet<void>::sort(Compare compare) {
            for (size_t i = 0; i < m_dense.size(); ++i) {
                for (size_t j = i + 1; j < m_dense.size(); ++j) {
                    if (compare(m_dense[j], m_dense[i])) {
                        size_t temp = m_dense[i];
                        m_dense[i] = m_dense[j];
                        m_dense[j] = temp;
                    }
                }
            }

            for (size_t i = 0; i < m_dense.size(); ++i) {
                m_sparse[m_dense[i]] = i;
            }
        }

    } // namespace Memory
} // namespace Engine
