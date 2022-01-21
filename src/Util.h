#pragma once

#include "vulkan/vulkan.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>

namespace edl {

inline uint32_t getMemoryType(const VkPhysicalDevice& physicalDevice, int filter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
	//TODO: Branching
	for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
		if (filter & (1 << i) && physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) return i;
	}

	fprintf(stderr, "Failed to find memory type!");
	return 0;
}

inline VkDeviceSize align(const VkDeviceSize& offset, const VkDeviceSize& alignment) {
	return offset + (alignment - offset % alignment) % alignment;
}

template<typename T>
inline T* getPointerOffset(void* data, uint32_t offset) {
	return reinterpret_cast<T*>(static_cast<char*>(data) + offset);
}

template<typename T>
inline const T* getPointerOffset(const void* data, uint32_t offset) {
	return reinterpret_cast<const T*>(static_cast<const char*>(data) + offset);
}

inline bool exists_test(const std::string& name) {
    if (FILE* file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    }
    else {
        return false;
    }
}

inline void printToFile(const std::string& out) {
    std::ofstream outfile;

    outfile.open("missing.txt", std::ios_base::app); // append instead of overwrite
    outfile << out << std::endl;
    outfile.close();
}

inline uint64_t hashString(const std::string& str) {
    std::hash<std::string> hasher;
    return hasher(str);
}

inline int32_t unsigned_to_signed(uint32_t u) {
    if (u <= INT_MAX)
        return static_cast<int>(u);

    if (u >= INT_MIN)
        return static_cast<int>(u - INT_MIN) + INT_MIN;

    return -1;
}

}