#pragma once

#include "allocator.h"

#include <utility>

namespace edl {
namespace mem {

template<typename T>
class DynamicArray {
public:
    DynamicArray() {}
    DynamicArray(Allocator& allocator, uint32_t count, uint32_t align = DEFAULT_ALIGNMENT) {
        calloc(allocator, count, align);
    }

    void malloc(Allocator& allocator, uint32_t count, uint32_t align = DEFAULT_ALIGNMENT) {
        this->count = count;
        memory = allocator.malloc<T>(count, align);
    }

    void calloc(Allocator& allocator, uint32_t count, uint32_t align = DEFAULT_ALIGNMENT) {
        this->count = count;
        memory = allocator.calloc<T>(count, align);
    }

    T& operator[](uint32_t index) { return memory[index]; }
    const T& operator[](uint32_t index) const { return memory[index]; }

    T* data() { return memory; }
    const T* data() const { return memory; }

    const uint32_t size() { return count; }

    void realloc(Allocator& allocator, uint32_t count, uint32_t align = DEFAULT_ALIGNMENT) {
        memory = allocator.realloc(memory, count, align);
        this->count = count;
    }

    void clear() {
        for (uint32_t i = 0; i < count; i++) {
            memory[i].~T();
        }
    }

    void free(Allocator& allocator) {
        clear();
        allocator.free(memory);
    }

private:
    uint32_t count = 0;
    T* memory = nullptr;
};

}
}