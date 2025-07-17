#include "MemoryManager.h"
#include "Logger.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>

using namespace VaporFrame::Core;

void testBasicAllocation() {
    std::cout << "=== Testing Basic Allocation ===" << std::endl;
    
    // Initialize memory manager
    MemoryManager::getInstance().initialize();
    
    // Test basic allocation
    void* ptr1 = VF_ALLOCATE(1024, 16, "test1");
    void* ptr2 = VF_ALLOCATE(2048, 32, "test2");
    void* ptr3 = VF_ALLOCATE(512, 8, "test3");
    
    std::cout << "Allocated: " << ptr1 << ", " << ptr2 << ", " << ptr3 << std::endl;
    
    // Test deallocation
    VF_DEALLOCATE(ptr1);
    VF_DEALLOCATE(ptr2);
    VF_DEALLOCATE(ptr3);
    
    std::cout << "Deallocated all pointers" << std::endl;
    
    // Dump stats
    MemoryManager::getInstance().getTracker().dumpStats();
}

void testMemoryPool() {
    std::cout << "\n=== Testing Memory Pool ===" << std::endl;
    
    // Create a custom memory pool
    MemoryPoolConfig config(1024 * 1024, 10 * 1024 * 1024, 4096, 16, true, "TestPool");
    MemoryPool* pool = MemoryManager::getInstance().createPool(config);
    
    // Allocate from pool
    void* ptr1 = pool->allocate(1024, 16);
    void* ptr2 = pool->allocate(2048, 32);
    void* ptr3 = pool->allocate(512, 8);
    
    std::cout << "Pool allocated: " << ptr1 << ", " << ptr2 << ", " << ptr3 << std::endl;
    
    // Check if pool owns the pointers
    std::cout << "Pool owns ptr1: " << (pool->owns(ptr1) ? "yes" : "no") << std::endl;
    std::cout << "Pool owns ptr2: " << (pool->owns(ptr2) ? "yes" : "no") << std::endl;
    std::cout << "Pool owns ptr3: " << (pool->owns(ptr3) ? "yes" : "no") << std::endl;
    
    // Get pool stats
    MemoryStats stats = pool->getStats();
    std::cout << "Pool stats - Current usage: " << stats.currentUsage << " bytes" << std::endl;
    std::cout << "Pool stats - Allocation count: " << stats.allocationCount << std::endl;
    
    // Deallocate
    pool->deallocate(ptr1);
    pool->deallocate(ptr2);
    pool->deallocate(ptr3);
    
    std::cout << "Pool deallocated all pointers" << std::endl;
    
    // Destroy pool
    MemoryManager::getInstance().destroyPool(pool);
}

void testStackAllocator() {
    std::cout << "\n=== Testing Stack Allocator ===" << std::endl;
    
    // Create stack allocator
    StackAllocator* stack = MemoryManager::getInstance().createStackAllocator(1024 * 1024);
    
    // Get initial marker
    void* marker1 = stack->getMarker();
    std::cout << "Initial marker: " << marker1 << std::endl;
    
    // Allocate some memory
    void* ptr1 = stack->allocate(1024, 16);
    void* ptr2 = stack->allocate(2048, 32);
    void* ptr3 = stack->allocate(512, 8);
    
    std::cout << "Stack allocated: " << ptr1 << ", " << ptr2 << ", " << ptr3 << std::endl;
    std::cout << "Current offset: " << stack->getCurrentOffset() << std::endl;
    
    // Get marker after allocations
    void* marker2 = stack->getMarker();
    std::cout << "Marker after allocations: " << marker2 << std::endl;
    
    // Free to first marker
    stack->freeToMarker(marker1);
    std::cout << "Freed to initial marker" << std::endl;
    std::cout << "Current offset after free: " << stack->getCurrentOffset() << std::endl;
    
    // Destroy stack allocator
    MemoryManager::getInstance().destroyStackAllocator(stack);
}

void testReallocation() {
    std::cout << "\n=== Testing Reallocation ===" << std::endl;
    
    // Test reallocation
    void* ptr = VF_ALLOCATE(1024, 16, "realloc_test");
    std::cout << "Initial allocation: " << ptr << std::endl;
    
    void* newPtr = VF_REALLOCATE(ptr, 2048);
    std::cout << "After reallocation: " << newPtr << std::endl;
    
    VF_DEALLOCATE(newPtr);
    std::cout << "Deallocated reallocated pointer" << std::endl;
}

void testPerformance() {
    std::cout << "\n=== Testing Performance ===" << std::endl;
    
    const int numAllocations = 10000;
    std::vector<void*> pointers;
    pointers.reserve(numAllocations);
    
    // Test system allocator
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numAllocations; ++i) {
        pointers.push_back(std::malloc(64 + (i % 1000)));
    }
    for (void* ptr : pointers) {
        std::free(ptr);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto systemTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    pointers.clear();
    
    // Test our memory manager
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numAllocations; ++i) {
        pointers.push_back(VF_ALLOCATE(64 + (i % 1000), 16, "perf_test"));
    }
    for (void* ptr : pointers) {
        VF_DEALLOCATE(ptr);
    }
    end = std::chrono::high_resolution_clock::now();
    auto managerTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "System allocator time: " << systemTime.count() << " microseconds" << std::endl;
    std::cout << "Memory manager time: " << managerTime.count() << " microseconds" << std::endl;
    std::cout << "Speedup: " << (double)systemTime.count() / managerTime.count() << "x" << std::endl;
}

void testMemoryTracking() {
    std::cout << "\n=== Testing Memory Tracking ===" << std::endl;
    
    // Enable tracking
    MemoryTracker::getInstance().enableTracking(true);
    
    // Make some allocations with tags
    void* ptr1 = VF_ALLOCATE(1024, 16, "tracked_allocation_1");
    void* ptr2 = VF_ALLOCATE(2048, 32, "tracked_allocation_2");
    void* ptr3 = VF_ALLOCATE(512, 8, "tracked_allocation_3");
    
    // Get active allocations
    auto activeAllocs = MemoryTracker::getInstance().getActiveAllocations();
    std::cout << "Active allocations: " << activeAllocs.size() << std::endl;
    
    for (const auto& alloc : activeAllocs) {
        std::cout << "  " << alloc.ptr << " (" << alloc.size << " bytes) - " << alloc.tag << std::endl;
    }
    
    // Deallocate some
    VF_DEALLOCATE(ptr1);
    VF_DEALLOCATE(ptr2);
    
    // Get remaining active allocations
    activeAllocs = MemoryTracker::getInstance().getActiveAllocations();
    std::cout << "Remaining active allocations: " << activeAllocs.size() << std::endl;
    
    // Deallocate the last one
    VF_DEALLOCATE(ptr3);
    
    // Dump final stats
    MemoryTracker::getInstance().dumpStats();
}

int main() {
    std::cout << "VaporFrame Memory Management System Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testBasicAllocation();
        testMemoryPool();
        testStackAllocator();
        testReallocation();
        testPerformance();
        testMemoryTracking();
        
        // Shutdown memory manager
        MemoryManager::getInstance().shutdown();
        
        std::cout << "\nAll tests completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 