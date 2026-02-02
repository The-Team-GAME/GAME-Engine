//-----------------------------------------------------------------------------------
// file:    Array.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Custom dynamic array container that uses the Memory Manager for all
//          allocations. Replaces std::vector in the ECS. Does NOT use any STL
//          containers internally - all memory is managed through raw pointers
//          and the Memory Manager.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../MemoryManager.h"
#include <utility>      // For std::move, std::forward
#include <new>          // For placement new
#include <initializer_list>
#include <type_traits>

namespace Engine {
    namespace Memory {

        //=================================================================================
        // Array<T> - Custom dynamic array (replacement for std::vector)
        //
        // Features:
        // - Uses Memory Manager for all allocations (no direct new/delete)
        // - Supports move semantics and perfect forwarding
        // - Compatible with range-based for loops
        // - No STL dependencies for storage
        //
        // Memory Layout:
        // [T0][T1][T2]...[Tn-1][...unused capacity...]
        //=================================================================================
        template<typename T>
        class Array {
        public:
            // Type aliases for STL compatibility
            using value_type = T;
            using size_type = size_t;
            using difference_type = ptrdiff_t;
            using reference = T&;
            using const_reference = const T&;
            using pointer = T*;
            using const_pointer = const T*;
            using iterator = T*;
            using const_iterator = const T*;

            //=================================================================================
            // Constructors / Destructor
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Default constructor - creates an empty array.

            Parameters:
            None.

            Where to call it:
            When declaring an array that will be filled later.
            ==================================================================================*/
            Array();

            /*=================================================================================
            Function Brief:
            Constructs an array with initial capacity reserved.

            Parameters:
            initial_capacity - Number of elements to reserve space for.

            Where to call it:
            When you know the approximate size needed in advance.
            ==================================================================================*/
            explicit Array(size_t initial_capacity);

            /*=================================================================================
            Function Brief:
            Constructs an array with count default-constructed elements.

            Parameters:
            count - Number of elements to create.
            value - Value to copy into each element.

            Where to call it:
            When creating a pre-filled array.
            ==================================================================================*/
            Array(size_t count, const T& value);

            /*=================================================================================
            Function Brief:
            Constructs an array from an initializer list.

            Parameters:
            init - Initializer list of elements.

            Where to call it:
            When creating an array with known initial values.
            ==================================================================================*/
            Array(std::initializer_list<T> init);

            /*=================================================================================
            Function Brief:
            Copy constructor - deep copies all elements.

            Parameters:
            other - Array to copy from.

            Where to call it:
            When a copy of an array is needed.
            ==================================================================================*/
            Array(const Array& other);

            /*=================================================================================
            Function Brief:
            Move constructor - transfers ownership of data.

            Parameters:
            other - Array to move from.

            Where to call it:
            When transferring array ownership.
            ==================================================================================*/
            Array(Array&& other) noexcept;

            /*=================================================================================
            Function Brief:
            Destructor - destroys all elements and frees memory.

            Parameters:
            None.

            Where to call it:
            Automatically called when array goes out of scope.
            ==================================================================================*/
            ~Array();

            //=================================================================================
            // Assignment Operators
            //=================================================================================

            Array& operator=(const Array& other);
            Array& operator=(Array&& other) noexcept;
            Array& operator=(std::initializer_list<T> init);

            //=================================================================================
            // Element Access
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Returns a reference to the element at the specified index.
            No bounds checking in release mode.

            Parameters:
            index - Index of the element.

            Returns:
            Reference to the element.

            Where to call it:
            When accessing elements by index.
            ==================================================================================*/
            T& operator[](size_t index);
            const T& operator[](size_t index) const;

            /*=================================================================================
            Function Brief:
            Returns a reference to the element at the specified index with bounds checking.

            Parameters:
            index - Index of the element.

            Returns:
            Reference to the element.

            Where to call it:
            When bounds checking is needed.
            ==================================================================================*/
            T& at(size_t index);
            const T& at(size_t index) const;

            /*=================================================================================
            Function Brief:
            Returns a reference to the first element.

            Parameters:
            None.

            Returns:
            Reference to the first element.

            Where to call it:
            When accessing the first element.
            ==================================================================================*/
            T& front();
            const T& front() const;

            /*=================================================================================
            Function Brief:
            Returns a reference to the last element.

            Parameters:
            None.

            Returns:
            Reference to the last element.

            Where to call it:
            When accessing the last element.
            ==================================================================================*/
            T& back();
            const T& back() const;

            /*=================================================================================
            Function Brief:
            Returns a pointer to the underlying data array.

            Parameters:
            None.

            Returns:
            Pointer to the data.

            Where to call it:
            When raw pointer access is needed.
            ==================================================================================*/
            T* data() noexcept;
            const T* data() const noexcept;

            //=================================================================================
            // Iterators
            //=================================================================================

            iterator begin() noexcept { return m_data; }
            const_iterator begin() const noexcept { return m_data; }
            const_iterator cbegin() const noexcept { return m_data; }

            iterator end() noexcept { return m_data + m_size; }
            const_iterator end() const noexcept { return m_data + m_size; }
            const_iterator cend() const noexcept { return m_data + m_size; }

            //=================================================================================
            // Capacity
            //=================================================================================

            bool empty() const noexcept { return m_size == 0; }
            size_t size() const noexcept { return m_size; }
            size_t capacity() const noexcept { return m_capacity; }

            /*=================================================================================
            Function Brief:
            Reserves capacity for at least the specified number of elements.

            Parameters:
            new_capacity - Minimum capacity to reserve.

            Where to call it:
            When you know the required size in advance to avoid reallocations.
            ==================================================================================*/
            void reserve(size_t new_capacity);

            /*=================================================================================
            Function Brief:
            Reduces capacity to match the current size.

            Parameters:
            None.

            Where to call it:
            After removing many elements to free unused memory.
            ==================================================================================*/
            void shrink_to_fit();

            //=================================================================================
            // Modifiers
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Removes all elements from the array. Capacity is not changed.

            Parameters:
            None.

            Where to call it:
            When resetting the array while keeping capacity.
            ==================================================================================*/
            void clear();

            /*=================================================================================
            Function Brief:
            Adds an element to the end of the array.

            Parameters:
            value - Element to add (copied).

            Where to call it:
            When appending elements.
            ==================================================================================*/
            void push_back(const T& value);

            /*=================================================================================
            Function Brief:
            Adds an element to the end of the array (move version).

            Parameters:
            value - Element to add (moved).

            Where to call it:
            When appending elements with move semantics.
            ==================================================================================*/
            void push_back(T&& value);

            /*=================================================================================
            Function Brief:
            Constructs an element in place at the end of the array.

            Parameters:
            args - Arguments forwarded to T's constructor.

            Returns:
            Reference to the newly created element.

            Where to call it:
            When constructing elements directly in the array.
            ==================================================================================*/
            template<typename... Args>
            T& emplace_back(Args&&... args);

            /*=================================================================================
            Function Brief:
            Removes the last element from the array.

            Parameters:
            None.

            Where to call it:
            When removing the last element.
            ==================================================================================*/
            void pop_back();

            /*=================================================================================
            Function Brief:
            Resizes the array to contain the specified number of elements.
            If count > size, new elements are default-constructed.
            If count < size, elements are destroyed.

            Parameters:
            count - New size.

            Where to call it:
            When changing the array size.
            ==================================================================================*/
            void resize(size_t count);

            /*=================================================================================
            Function Brief:
            Resizes the array to contain the specified number of elements.
            New elements are initialized with the given value.

            Parameters:
            count - New size.
            value - Value for new elements.

            Where to call it:
            When changing the array size with a specific value.
            ==================================================================================*/
            void resize(size_t count, const T& value);

            /*=================================================================================
            Function Brief:
            Swaps the contents with another array.

            Parameters:
            other - Array to swap with.

            Where to call it:
            When swapping array contents efficiently.
            ==================================================================================*/
            void swap(Array& other) noexcept;

            /*=================================================================================
            Function Brief:
            Erases the element at the specified index using swap-and-pop.
            Order is NOT preserved, but this is O(1).

            Parameters:
            index - Index of element to erase.

            Where to call it:
            When order doesn't matter and fast removal is needed.
            ==================================================================================*/
            void erase_swap(size_t index);

            /*=================================================================================
            Function Brief:
            Erases the element at the specified index, preserving order.
            This is O(n) due to element shifting.

            Parameters:
            index - Index of element to erase.

            Where to call it:
            When order must be preserved.
            ==================================================================================*/
            void erase(size_t index);

            /*=================================================================================
            Function Brief:
            Erases the element at the specified iterator position, preserving order.
            Returns an iterator to the element following the erased element.

            Parameters:
            pos - Iterator to the element to erase.

            Returns:
            Iterator to the element following the erased element.

            Where to call it:
            For STL-compatible iterator-based erasure.
            ==================================================================================*/
            iterator erase(iterator pos);

            /*=================================================================================
            Function Brief:
            Erases a range of elements [first, last), preserving order.
            Returns an iterator to the element following the last erased element.

            Parameters:
            first - Iterator to the first element to erase.
            last - Iterator to one past the last element to erase.

            Returns:
            Iterator to the element following the last erased element.

            Where to call it:
            For STL-compatible range erasure (e.g., with std::remove_if).
            ==================================================================================*/
            iterator erase(iterator first, iterator last);

            /*=================================================================================
            Function Brief:
            Inserts an element at the specified index.

            Parameters:
            index - Index to insert at.
            value - Element to insert.

            Where to call it:
            When inserting at a specific position.
            ==================================================================================*/
            void insert(size_t index, const T& value);
            void insert(size_t index, T&& value);

        private:
            T* m_data;          // Pointer to data array
            size_t m_size;      // Number of elements
            size_t m_capacity;  // Allocated capacity

            /*=================================================================================
            Function Brief:
            Grows the capacity by a factor (typically 1.5x or 2x).

            Parameters:
            min_capacity - Minimum required capacity.

            Where to call it:
            Internally when capacity is exhausted.
            ==================================================================================*/
            void grow(size_t min_capacity);

            /*=================================================================================
            Function Brief:
            Reallocates the internal buffer to new capacity.

            Parameters:
            new_capacity - New capacity.

            Where to call it:
            Internally when resizing.
            ==================================================================================*/
            void reallocate(size_t new_capacity);
        };

        //=================================================================================
        // Template Implementation
        //=================================================================================

        template<typename T>
        Array<T>::Array()
            : m_data(nullptr)
            , m_size(0)
            , m_capacity(0)
        {
        }

        template<typename T>
        Array<T>::Array(size_t initial_capacity)
            : m_data(nullptr)
            , m_size(0)
            , m_capacity(0)
        {
            reserve(initial_capacity);
        }

        template<typename T>
        Array<T>::Array(size_t count, const T& value)
            : m_data(nullptr)
            , m_size(0)
            , m_capacity(0)
        {
            resize(count, value);
        }

        template<typename T>
        Array<T>::Array(std::initializer_list<T> init)
            : m_data(nullptr)
            , m_size(0)
            , m_capacity(0)
        {
            reserve(init.size());
            for (const auto& item : init) {
                push_back(item);
            }
        }

        template<typename T>
        Array<T>::Array(const Array& other)
            : m_data(nullptr)
            , m_size(0)
            , m_capacity(0)
        {
            reserve(other.m_size);
            for (size_t i = 0; i < other.m_size; ++i) {
                new (&m_data[i]) T(other.m_data[i]);
            }
            m_size = other.m_size;
        }

        template<typename T>
        Array<T>::Array(Array&& other) noexcept
            : m_data(other.m_data)
            , m_size(other.m_size)
            , m_capacity(other.m_capacity)
        {
            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }

        template<typename T>
        Array<T>::~Array() {
            clear();
            if (m_data && m_capacity > 0) {
                // Check if MemoryManager is still valid (static destruction order safety)
                if (MemoryManager::is_instance_valid()) {
                    MemoryManager::instance().deallocate(m_data, m_capacity * sizeof(T));
                }
                m_data = nullptr;
            }
            m_capacity = 0;
        }

        template<typename T>
        Array<T>& Array<T>::operator=(const Array& other) {
            if (this != &other) {
                clear();
                reserve(other.m_size);
                for (size_t i = 0; i < other.m_size; ++i) {
                    new (&m_data[i]) T(other.m_data[i]);
                }
                m_size = other.m_size;
            }
            return *this;
        }

        template<typename T>
        Array<T>& Array<T>::operator=(Array&& other) noexcept {
            if (this != &other) {
                clear();
                if (m_data && m_capacity > 0) {
                    if (MemoryManager::is_instance_valid()) {
                        MemoryManager::instance().deallocate(m_data, m_capacity * sizeof(T));
                    }
                }

                m_data = other.m_data;
                m_size = other.m_size;
                m_capacity = other.m_capacity;

                other.m_data = nullptr;
                other.m_size = 0;
                other.m_capacity = 0;
            }
            return *this;
        }

        template<typename T>
        Array<T>& Array<T>::operator=(std::initializer_list<T> init) {
            clear();
            reserve(init.size());
            for (const auto& item : init) {
                push_back(item);
            }
            return *this;
        }

        template<typename T>
        T& Array<T>::operator[](size_t index) {
            return m_data[index];
        }

        template<typename T>
        const T& Array<T>::operator[](size_t index) const {
            return m_data[index];
        }

        template<typename T>
        T& Array<T>::at(size_t index) {
#ifdef _DEBUG
            if (index >= m_size) {
                // In debug mode, we could throw or assert
                // For now, just clamp to last element
                index = m_size > 0 ? m_size - 1 : 0;
            }
#endif
            return m_data[index];
        }

        template<typename T>
        const T& Array<T>::at(size_t index) const {
#ifdef _DEBUG
            if (index >= m_size) {
                index = m_size > 0 ? m_size - 1 : 0;
            }
#endif
            return m_data[index];
        }

        template<typename T>
        T& Array<T>::front() {
            return m_data[0];
        }

        template<typename T>
        const T& Array<T>::front() const {
            return m_data[0];
        }

        template<typename T>
        T& Array<T>::back() {
            return m_data[m_size - 1];
        }

        template<typename T>
        const T& Array<T>::back() const {
            return m_data[m_size - 1];
        }

        template<typename T>
        T* Array<T>::data() noexcept {
            return m_data;
        }

        template<typename T>
        const T* Array<T>::data() const noexcept {
            return m_data;
        }

        template<typename T>
        void Array<T>::reserve(size_t new_capacity) {
            if (new_capacity > m_capacity) {
                reallocate(new_capacity);
            }
        }

        template<typename T>
        void Array<T>::shrink_to_fit() {
            if (m_size < m_capacity) {
                reallocate(m_size);
            }
        }

        template<typename T>
        void Array<T>::clear() {
            // Guard against null m_data (e.g., after move or during destruction)
            // Also check if MemoryManager is destroyed - if so, m_data may point to freed memory
            if (!m_data || !MemoryManager::is_instance_valid()) {
                m_size = 0;
                return;
            }
            // Call destructors for all elements
            for (size_t i = 0; i < m_size; ++i) {
                m_data[i].~T();
            }
            m_size = 0;
        }

        template<typename T>
        void Array<T>::push_back(const T& value) {
            if (m_size >= m_capacity) {
                grow(m_size + 1);
            }
            new (&m_data[m_size]) T(value);
            ++m_size;
        }

        template<typename T>
        void Array<T>::push_back(T&& value) {
            if (m_size >= m_capacity) {
                grow(m_size + 1);
            }
            new (&m_data[m_size]) T(std::move(value));
            ++m_size;
        }

        template<typename T>
        template<typename... Args>
        T& Array<T>::emplace_back(Args&&... args) {
            if (m_size >= m_capacity) {
                grow(m_size + 1);
            }
            new (&m_data[m_size]) T(std::forward<Args>(args)...);
            return m_data[m_size++];
        }

        template<typename T>
        void Array<T>::pop_back() {
            if (m_size > 0) {
                --m_size;
                m_data[m_size].~T();
            }
        }

        template<typename T>
        void Array<T>::resize(size_t count) {
            if (count > m_size) {
                reserve(count);
                // Default construct new elements
                for (size_t i = m_size; i < count; ++i) {
                    new (&m_data[i]) T();
                }
            }
            else if (count < m_size) {
                // Destruct excess elements
                for (size_t i = count; i < m_size; ++i) {
                    m_data[i].~T();
                }
            }
            m_size = count;
        }

        template<typename T>
        void Array<T>::resize(size_t count, const T& value) {
            if (count > m_size) {
                reserve(count);
                // Copy construct new elements
                for (size_t i = m_size; i < count; ++i) {
                    new (&m_data[i]) T(value);
                }
            }
            else if (count < m_size) {
                // Destruct excess elements
                for (size_t i = count; i < m_size; ++i) {
                    m_data[i].~T();
                }
            }
            m_size = count;
        }

        template<typename T>
        void Array<T>::swap(Array& other) noexcept {
            T* temp_data = m_data;
            size_t temp_size = m_size;
            size_t temp_capacity = m_capacity;

            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;

            other.m_data = temp_data;
            other.m_size = temp_size;
            other.m_capacity = temp_capacity;
        }

        template<typename T>
        void Array<T>::erase_swap(size_t index) {
            if (index >= m_size) return;

            // Swap with last element and pop
            if (index != m_size - 1) {
                m_data[index] = std::move(m_data[m_size - 1]);
            }
            pop_back();
        }

        template<typename T>
        void Array<T>::erase(size_t index) {
            if (index >= m_size) return;

            // Shift all elements down
            for (size_t i = index; i < m_size - 1; ++i) {
                m_data[i] = std::move(m_data[i + 1]);
            }
            pop_back();
        }

        template<typename T>
        typename Array<T>::iterator Array<T>::erase(iterator pos) {
            if (pos < begin() || pos >= end()) return end();

            size_t index = pos - begin();
            erase(index);
            return begin() + index;
        }

        template<typename T>
        typename Array<T>::iterator Array<T>::erase(iterator first, iterator last) {
            if (first >= last || first < begin() || last > end()) return last;

            size_t first_index = first - begin();
            size_t count = last - first;

            // Move elements after the range to fill the gap
            for (size_t i = first_index; i + count < m_size; ++i) {
                m_data[i] = std::move(m_data[i + count]);
            }

            // Destroy the elements at the end
            for (size_t i = 0; i < count; ++i) {
                m_data[m_size - 1 - i].~T();
            }

            m_size -= count;
            return begin() + first_index;
        }

        template<typename T>
        void Array<T>::insert(size_t index, const T& value) {
            if (index > m_size) index = m_size;

            if (m_size >= m_capacity) {
                grow(m_size + 1);
            }

            // Shift elements up
            for (size_t i = m_size; i > index; --i) {
                new (&m_data[i]) T(std::move(m_data[i - 1]));
                m_data[i - 1].~T();
            }

            new (&m_data[index]) T(value);
            ++m_size;
        }

        template<typename T>
        void Array<T>::insert(size_t index, T&& value) {
            if (index > m_size) index = m_size;

            if (m_size >= m_capacity) {
                grow(m_size + 1);
            }

            // Shift elements up
            for (size_t i = m_size; i > index; --i) {
                new (&m_data[i]) T(std::move(m_data[i - 1]));
                m_data[i - 1].~T();
            }

            new (&m_data[index]) T(std::move(value));
            ++m_size;
        }

        template<typename T>
        void Array<T>::grow(size_t min_capacity) {
            // Growth factor of 1.5x or minimum required
            size_t new_capacity = m_capacity == 0 ? 4 : m_capacity + m_capacity / 2;
            if (new_capacity < min_capacity) {
                new_capacity = min_capacity;
            }
            reallocate(new_capacity);
        }

        template<typename T>
        void Array<T>::reallocate(size_t new_capacity) {
            if (new_capacity == 0) {
                clear();
                if (m_data) {
                    MemoryManager::instance().deallocate(m_data, m_capacity * sizeof(T));
                    m_data = nullptr;
                }
                m_capacity = 0;
                return;
            }

            // Allocate new memory
            T* new_data = static_cast<T*>(MemoryManager::instance().allocate(new_capacity * sizeof(T)));

            if (new_data && m_data && m_size > 0) {
                // Move construct elements into new memory
                size_t elements_to_move = m_size < new_capacity ? m_size : new_capacity;
                for (size_t i = 0; i < elements_to_move; ++i) {
                    new (&new_data[i]) T(std::move(m_data[i]));
                    m_data[i].~T();
                }

                // Destruct any remaining elements
                for (size_t i = elements_to_move; i < m_size; ++i) {
                    m_data[i].~T();
                }
            }

            // Free old memory
            if (m_data) {
                MemoryManager::instance().deallocate(m_data, m_capacity * sizeof(T));
            }

            m_data = new_data;
            m_capacity = new_capacity;
            if (m_size > new_capacity) {
                m_size = new_capacity;
            }
        }

    } // namespace Memory
} // namespace Engine
