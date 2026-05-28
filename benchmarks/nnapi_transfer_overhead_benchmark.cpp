#include "../core/inference/nnapi_backend.hpp"
#include <iostream>
#include <cassert>

using namespace isha;

void testTransferOverhead() {
    NnapiBackend backend;
    
    // Allocate staging buffers
    bool dma_alloc = backend.allocateStagingBuffer(1024);
    assert(dma_alloc);

    // Execute transfer
    char dummy_data[1024] = {0};
    bool success_zero_copy = backend.executeTransfer(dummy_data, 1024, true);
    assert(success_zero_copy);

    bool success_staging_copy = backend.executeTransfer(dummy_data, 1024, false);
    assert(success_staging_copy);

    std::cout << "testTransferOverhead passed." << std::endl;
}

int main() {
    testTransferOverhead();
    std::cout << "All NNAPI Transfer Overhead Benchmarks passed." << std::endl;
    return 0;
}
