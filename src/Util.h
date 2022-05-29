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

}