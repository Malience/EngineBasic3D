#include "ResourceHandles.h"

#include "Util.h"

#include <filesystem>

namespace edl {
namespace res {

//TODO: Might cause issues
void MeshBuilder::addBatch(Batch batch) {
    batches.push_back(batch);
}
Mesh MeshBuilder::build(Allocator& allocator) {
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

}
}