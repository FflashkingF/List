#pragma once
#include <iostream>
#include <memory>

template <typename T, size_t N>
class StackAllocator;

template <size_t N>
class StackStorage {
  public:
    size_t shift = 0;
    char arr[N];
    StackStorage(const StackStorage&) = delete;
    StackStorage() = default;
    StackStorage& operator=(const StackStorage&) = delete;
};

template <typename T, size_t N>
class StackAllocator {
  public:
    StackStorage<N>* stack;

    using value_type = T;

    StackAllocator(StackStorage<N>& pool)
        : stack(&pool) {}

    template <typename U>
    StackAllocator(const StackAllocator<U, N>& other)
        : stack(other.stack) {}

    ~StackAllocator() {}

    template <typename U>
    StackAllocator& operator=(const StackAllocator<U, N>& other) {
        stack = other->stack;
        return *this;
    }

    T* allocate(size_t n) {
        size_t al = alignof(T);
        stack->shift = (stack->shift + al - 1) / al * al;
        T* ans = reinterpret_cast<T*>(stack->arr + stack->shift);
        stack->shift += n * sizeof(T);
        return ans;
    }

    void deallocate(T* /*unused*/, size_t /*unused*/) {}

    template <typename U>
    bool operator==(const StackAllocator<U, N>& other) {
        return stack == other.stack;
    };

    template <typename U>
    bool operator!=(const StackAllocator<U, N>& other) {
        return stack != other.stack;
    };

    template <typename U>
    struct rebind {
        using other = StackAllocator<U, N>;
    };
};