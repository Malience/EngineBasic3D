#pragma once

#include "descriptor_manager.h"
#include "glm/glm.hpp"
#include "vulkan/vulkan.h"
#include "Allocator.h"
#include "image_table.h"

#include <string>
#include <unordered_map>

namespace edl {
namespace res {

class Pipeline;
class ResourceSystem;


struct VertexBuffer {
    uint32_t positionOffset;
    uint32_t normalOffset;
    uint32_t texCoord0Offset;
    uint32_t texCoord1Offset;

    uint32_t indexOffset;
};

struct IndexBuffer {
    //std::string name;
    //VkIndexType type;
    uint32_t size;
    void* data;
};


struct Image {
    ImageHandle image;
    //VkDescriptorSet set = 0; //PLEASE!!!!!!
    //UBOHandle ubo_handle;
    int32_t materialIndex;
    uint32_t size;
    void* data;
};

/*
class Descriptor {
    DescriptorHandle handle;
};

class ModelData {
public:
    std::string mesh;
    std::vector<std::string> textures;
};
*/

typedef uint64_t ResourceID;
typedef uint64_t ResourceType;
typedef uint64_t ResourceSubType;

enum class ResourceStatus : uint8_t {
    UNLOADED,
    LOADED,
    FINISHED,

    MAX_RESOURCE_STATUS
};

struct Resource {
    ResourceID id;
    ResourceStatus status;

    ResourceType type;
    ResourceSubType subtype;

    uint64_t nameHash;
    char* name;

    uint64_t filenameHash;
    char* path;

    uint32_t dependenciesCount;
    ResourceID* dependencies;

    size_t size;
    void* data;
};

struct File {
    size_t size;
    void* data;
};

struct Batch {
    uint32_t indexStart;
    uint32_t indexCount;
    ResourceID material;
};

struct Mesh {
    uint32_t positionOffset;
    uint32_t normalOffset;
    uint32_t texCoord0Offset;
    uint32_t texCoord1Offset;
    uint32_t indexOffset;

    uint32_t batchCount;
    Batch* batches;
};

//TODO: Maybe set up a toolchain style system? Like you compile a toolchain similar to a VkDevice and toss that around to things?
class MeshBuilder {
public:
    void addBatch(Batch batch);

    Mesh build(mem::Allocator& allocator);
    void clear();

private:
    std::vector<Batch> batches;
};

File loadFile(const char* path, mem::Allocator& allocator);

inline void allocateResourceData(Resource& res, size_t size, mem::Allocator& allocator) {
    res.size = size;
    res.data = allocator.malloc(size);
}

template<typename T>
inline T& getResourceData(Resource& res) {
    return *static_cast<T*>(res.data);
}

typedef uint64_t KeyID;

template<typename T>
class Keychain {
public:
    KeyID add(const std::string& id, T key) {
        std::hash<std::string> hasher;
        KeyID hash = hasher(id);
        chain.insert({ hash, key });
        return hash;
    }

    KeyID add(const char* id, T key) {
        std::hash<std::string> hasher;
        KeyID hash = hasher(id);
        chain.insert({ hash, key });
        return hash;
    }

    KeyID add(KeyID id, T key) {
        chain.insert({ id, key });
        return id;
    }

    T& get(const std::string& id) {
        std::hash<std::string> hasher;
        KeyID hash = hasher(id);
        return chain.at(hash);
    }

    T& get(const char* id) {
        std::hash<std::string> hasher;
        KeyID hash = hasher(id);
        return chain.at(hash);
    }

    T& get(KeyID id) {
        return chain.at(id);
    }

protected:
    std::unordered_map<KeyID, T> chain;
};

class Toolchain : public Keychain<void*> {
public:
    template<typename T>
    T& getTool(const std::string& id) {
        std::hash<std::string> hasher;
        KeyID hash = hasher(id);
        return *static_cast<T*>(chain.at(hash));
    }

    template<typename T>
    T& getTool(const char* id) {
        std::hash<std::string> hasher;
        KeyID hash = hasher(id);
        return *static_cast<T*>(chain.at(hash));
    }

    template<typename T>
    T& getTool(KeyID id) {
        return *static_cast<T*>(chain.at(id));
    }
};

}
}
