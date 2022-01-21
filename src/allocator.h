#pragma once

#include <stdint.h>

namespace edl {
namespace mem {

const uint32_t DEFAULT_ALIGNMENT = 0x4;

class Allocator {
public:
    Allocator();
    virtual ~Allocator();

    virtual void* malloc(uint32_t size, uint32_t align = DEFAULT_ALIGNMENT) = 0;
    void* calloc(uint32_t size, uint32_t align = DEFAULT_ALIGNMENT);

    virtual void free(void* data) = 0;
    virtual void* realloc(void* data, uint32_t size, uint32_t align = DEFAULT_ALIGNMENT) = 0;

    template<typename T>
    inline T* malloc(uint32_t count, uint32_t align = DEFAULT_ALIGNMENT) {
        return static_cast<T*>(malloc(count * sizeof(T), align));
    }

    template<typename T>
    inline T* calloc(uint32_t count, uint32_t align = DEFAULT_ALIGNMENT) {
        return static_cast<T*>(calloc(count * sizeof(T), align));
    }

};

class DefaultAllocator : public Allocator {
public:
    virtual void* malloc(uint32_t size, uint32_t align = DEFAULT_ALIGNMENT);
    virtual void free(void* data);
    virtual void* realloc(void* data, uint32_t size, uint32_t align = DEFAULT_ALIGNMENT);
};

}
}