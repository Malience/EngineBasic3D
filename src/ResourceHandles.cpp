#include "ResourceHandles.h"

#include "Util.h"

#include <filesystem>

namespace edl {
namespace res {

//TODO: Might cause issues
void MeshBuilder::addBatch(Batch batch) {
    batches.push_back(batch);
}
Mesh MeshBuilder::build(mem::Allocator& allocator) {
    Mesh mesh = {};

    mesh.batchCount = batches.size();
    size_t batchSize = mesh.batchCount * sizeof(Batch);
    mesh.batches = static_cast<Batch*>(allocator.malloc(batchSize));
    memcpy(mesh.batches, batches.data(), batchSize);

    return mesh;
}

void MeshBuilder::clear() {
    batches.clear();
}

File loadFile(const char* path, mem::Allocator& allocator) {
    std::filesystem::directory_entry entry(path);
    if(!entry.exists()) {
        return { 0, 0 };
    }
    File file = {};
    file.size = entry.file_size();
    file.data = allocator.malloc(file.size);

    FILE* f = fopen(path, "rb");

    size_t bytes = 0;
    size_t bytesRead = 0;
    do {
        bytes = fread((char*)file.data + bytesRead, 1, 2048, f);
        bytesRead += bytes;
    } while (bytes > 0);

    if (ferror(f)) {
        printf("Error!\n");
    }

    fclose(f);

    return file;
}

}
}