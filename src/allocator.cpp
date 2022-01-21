#include "allocator.h"

#include "Util.h"

#include <string>

namespace edl {
namespace mem {

Allocator::Allocator() {

}

Allocator::~Allocator() {

}

void* Allocator::calloc(uint32_t size, uint32_t align) {
    void* ptr = malloc(size, align);
    std::memset(ptr, 0, size);
    return ptr;
}

void* DefaultAllocator::malloc(uint32_t size, uint32_t align) {
    return std::malloc(size);
}
void DefaultAllocator::free(void* data) {
    std::free(data);
}
void* DefaultAllocator::realloc(void* data, uint32_t size, uint32_t align) {
    return std::realloc(data, size);
}

}
}