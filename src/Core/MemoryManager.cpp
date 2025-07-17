#include "MemoryManager.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdint>

#define NOMINMAX
#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace VaporFrame {
namespace Core {

// MemoryPool Implementation
MemoryPool::MemoryPool(const MemoryPoolConfig& config) : config(config) {
    // Allocate initial pool
    void* initialPool = allocateFromSystem(config.initialSize);
    if (initialPool) {
        pools.push_back(initialPool);
        
        // Create initial free block
        Block* initialBlock = new Block();
        initialBlock->data = initialPool;
        initialBlock->size = config.initialSize;
        initialBlock->used = false;
        initialBlock->next = nullptr;
        initialBlock->prev = nullptr;
        
        freeBlocks.push_back(initialBlock);
        blockMap[initialPool] = initialBlock;
    }
}

MemoryPool::~MemoryPool() {
    std::lock_guard<std::mutex> lock(mutex);
    
    // Free all pools
    for (void* pool : pools) {
        deallocateFromSystem(pool);
    }
    
    // Free all blocks
    for (Block* block : freeBlocks) {
        delete block;
    }
    
    pools.clear();
    freeBlocks.clear();
    blockMap.clear();
}

void* MemoryPool::allocate(std::size_t size, std::size_t alignment) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (size == 0) return nullptr;
    
    // Find suitable free block
    Block* block = findFreeBlock(size, alignment);
    if (!block) {
        // Try to expand pool
        if (!expand(std::max(size, config.blockSize))) {
            return nullptr;
        }
        block = findFreeBlock(size, alignment);
        if (!block) return nullptr;
    }
    
    // Calculate aligned address
    std::size_t alignedOffset = (reinterpret_cast<std::uintptr_t>(static_cast<char*>(block->data)) + alignment - 1) & ~(alignment - 1);
    std::size_t padding = alignedOffset - reinterpret_cast<std::uintptr_t>(static_cast<char*>(block->data));
    
    if (padding + size > block->size) {
        return nullptr; // Block too small after alignment
    }
    
    // Split block if necessary
    if (padding + size < block->size) {
        splitBlock(block, padding + size);
    }
    
    // Mark block as used
    block->used = true;
    
    // Update statistics
    stats.totalAllocated += size;
    stats.currentUsage += size;
    stats.allocationCount++;
    stats.peakUsage = std::max(stats.peakUsage, stats.currentUsage);
    
    void* result = reinterpret_cast<void*>(alignedOffset);
    
    // Track allocation if enabled
    if (config.enableTracking) {
        MemoryTracker::getInstance().trackAllocation(result, size, alignment, config.name, "", 0);
    }
    
    return result;
}

void MemoryPool::deallocate(void* ptr) {
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // Find the block containing this pointer
    Block* block = nullptr;
    for (auto& pair : blockMap) {
        if (ptr >= pair.first && ptr < static_cast<char*>(pair.first) + pair.second->size) {
            block = pair.second;
            break;
        }
    }
    
    if (!block || !block->used) return;
    
    // Update statistics
    std::size_t size = block->size;
    stats.totalFreed += size;
    stats.currentUsage -= size;
    stats.deallocationCount++;
    
    // Mark block as free
    block->used = false;
    
    // Merge with adjacent free blocks
    mergeAdjacentBlocks(block);
    
    // Track deallocation if enabled
    if (config.enableTracking) {
        MemoryTracker::getInstance().trackDeallocation(ptr);
    }
}

void* MemoryPool::reallocate(void* ptr, std::size_t newSize) {
    if (!ptr) return allocate(newSize);
    if (newSize == 0) {
        deallocate(ptr);
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // Find current block
    Block* block = nullptr;
    for (auto& pair : blockMap) {
        if (ptr >= pair.first && ptr < static_cast<char*>(pair.first) + pair.second->size) {
            block = pair.second;
            break;
        }
    }
    
    if (!block) return nullptr;
    
    // If new size fits in current block, just return the same pointer
    if (newSize <= block->size) {
        return ptr;
    }
    
    // Otherwise, allocate new block and copy data
    void* newPtr = allocate(newSize);
    if (newPtr) {
        std::memcpy(newPtr, ptr, block->size);
        deallocate(ptr);
    }
    
    return newPtr;
}

std::size_t MemoryPool::getSize(void* ptr) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& pair : blockMap) {
        if (ptr >= pair.first && ptr < static_cast<char*>(pair.first) + pair.second->size) {
            return pair.second->size;
        }
    }
    return 0;
}

bool MemoryPool::owns(void* ptr) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& pair : blockMap) {
        if (ptr >= pair.first && ptr < static_cast<char*>(pair.first) + pair.second->size) {
            return true;
        }
    }
    return false;
}

MemoryStats MemoryPool::getStats() const {
    std::lock_guard<std::mutex> lock(mutex);
    return stats;
}

void MemoryPool::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    
    // Free all pools except the first one
    for (size_t i = 1; i < pools.size(); ++i) {
        deallocateFromSystem(pools[i]);
    }
    pools.resize(1);
    
    // Reset all blocks to free
    for (auto& pair : blockMap) {
        pair.second->used = false;
    }
    
    // Rebuild free blocks list
    freeBlocks.clear();
    for (auto& pair : blockMap) {
        freeBlocks.push_back(pair.second);
    }
    
    stats.reset();
}

bool MemoryPool::expand(std::size_t additionalSize) {
    if (pools.empty()) return false;
    
    std::size_t totalSize = 0;
    for (void* pool : pools) {
        totalSize += config.initialSize;
    }
    
    if (totalSize + additionalSize > config.maxSize) {
        return false;
    }
    
    void* newPool = allocateFromSystem(additionalSize);
    if (!newPool) return false;
    
    pools.push_back(newPool);
    
    // Create new free block
    Block* newBlock = new Block();
    newBlock->data = newPool;
    newBlock->size = additionalSize;
    newBlock->used = false;
    newBlock->next = nullptr;
    newBlock->prev = nullptr;
    
    freeBlocks.push_back(newBlock);
    blockMap[newPool] = newBlock;
    
    return true;
}

void MemoryPool::defragment() {
    std::lock_guard<std::mutex> lock(mutex);
    
    // Simple defragmentation: merge adjacent free blocks
    for (auto& pair : blockMap) {
        if (!pair.second->used) {
            mergeAdjacentBlocks(pair.second);
        }
    }
}

std::size_t MemoryPool::getFragmentation() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::size_t totalFree = 0;
    std::size_t largestFree = 0;
    
    for (auto& pair : blockMap) {
        if (!pair.second->used) {
            totalFree += pair.second->size;
            largestFree = std::max(largestFree, pair.second->size);
        }
    }
    
    if (totalFree == 0) return 0;
    return ((totalFree - largestFree) * 100) / totalFree;
}

MemoryPool::Block* MemoryPool::findFreeBlock(std::size_t size, std::size_t alignment) {
    for (Block* block : freeBlocks) {
        if (!block->used) {
            std::size_t alignedOffset = (reinterpret_cast<std::uintptr_t>(static_cast<char*>(block->data)) + alignment - 1) & ~(alignment - 1);
            std::size_t padding = alignedOffset - reinterpret_cast<std::uintptr_t>(static_cast<char*>(block->data));
            
            if (padding + size <= block->size) {
                return block;
            }
        }
    }
    return nullptr;
}

void MemoryPool::splitBlock(Block* block, std::size_t usedSize) {
    if (usedSize >= block->size) return;
    
    std::size_t remainingSize = block->size - usedSize;
    if (remainingSize < config.blockSize) return; // Too small to split
    
    // Create new block for remaining space
    Block* newBlock = new Block();
    newBlock->data = static_cast<char*>(block->data) + usedSize;
    newBlock->size = remainingSize;
    newBlock->used = false;
    newBlock->next = block->next;
    newBlock->prev = block;
    
    if (block->next) block->next->prev = newBlock;
    block->next = newBlock;
    
    // Update block size
    block->size = usedSize;
    
    // Add to free blocks list
    freeBlocks.push_back(newBlock);
    blockMap[newBlock->data] = newBlock;
}

void MemoryPool::mergeAdjacentBlocks(Block* block) {
    // Merge with next block
    if (block->next && !block->next->used) {
        block->size += block->next->size;
        Block* nextBlock = block->next;
        block->next = nextBlock->next;
        if (nextBlock->next) nextBlock->next->prev = block;
        
        // Remove from free blocks and block map
        auto it = std::find(freeBlocks.begin(), freeBlocks.end(), nextBlock);
        if (it != freeBlocks.end()) freeBlocks.erase(it);
        blockMap.erase(nextBlock->data);
        delete nextBlock;
    }
    
    // Merge with previous block
    if (block->prev && !block->prev->used) {
        block->prev->size += block->size;
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
        
        // Remove from free blocks and block map
        auto it = std::find(freeBlocks.begin(), freeBlocks.end(), block);
        if (it != freeBlocks.end()) freeBlocks.erase(it);
        blockMap.erase(block->data);
        delete block;
    }
}

void* MemoryPool::allocateFromSystem(std::size_t size) {
#ifdef _WIN32
    return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
}

void MemoryPool::deallocateFromSystem(void* ptr) {
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, 0);
#endif
}

// StackAllocator Implementation
StackAllocator::StackAllocator(std::size_t size) : totalSize(size) {
    memory = std::malloc(size);
    if (!memory) {
        throw std::bad_alloc();
    }
    currentOffset = 0;
}

StackAllocator::~StackAllocator() {
    if (memory) {
        std::free(memory);
    }
}

void* StackAllocator::allocate(std::size_t size, std::size_t alignment) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (size == 0) return nullptr;
    
    // Calculate aligned address
    std::size_t alignedOffset = (reinterpret_cast<std::uintptr_t>(memory) + currentOffset + alignment - 1) & ~(alignment - 1);
    std::size_t padding = alignedOffset - (reinterpret_cast<std::uintptr_t>(memory) + currentOffset);
    
    if (currentOffset + padding + size > totalSize) {
        return nullptr; // Not enough space
    }
    
    currentOffset += padding + size;
    
    // Update statistics
    stats.totalAllocated += size;
    stats.currentUsage += size;
    stats.allocationCount++;
    stats.peakUsage = std::max(stats.peakUsage, stats.currentUsage);
    
    return reinterpret_cast<void*>(alignedOffset);
}

void StackAllocator::deallocate(void* ptr) {
    // Stack allocator doesn't support individual deallocation
    (void)ptr; // Suppress unused parameter warning
}

void* StackAllocator::reallocate(void* ptr, std::size_t newSize) {
    // Stack allocator doesn't support reallocation
    (void)ptr;
    (void)newSize;
    return nullptr;
}

std::size_t StackAllocator::getSize(void* ptr) const {
    // Stack allocator doesn't track individual allocation sizes
    (void)ptr;
    return 0;
}

bool StackAllocator::owns(void* ptr) const {
    return ptr >= memory && ptr < static_cast<char*>(memory) + totalSize;
}

MemoryStats StackAllocator::getStats() const {
    std::lock_guard<std::mutex> lock(mutex);
    return stats;
}

void StackAllocator::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    currentOffset = 0;
    stats.reset();
}

void* StackAllocator::getMarker() const {
    std::lock_guard<std::mutex> lock(mutex);
    return static_cast<char*>(memory) + currentOffset;
}

void StackAllocator::freeToMarker(void* marker) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (marker < memory || marker > static_cast<char*>(memory) + totalSize) {
        return; // Invalid marker
    }
    
    std::size_t newOffset = static_cast<char*>(marker) - static_cast<char*>(memory);
    if (newOffset <= currentOffset) {
        currentOffset = newOffset;
    }
}

std::size_t StackAllocator::getCurrentOffset() const {
    std::lock_guard<std::mutex> lock(mutex);
    return currentOffset;
}

// MemoryTracker Implementation
MemoryTracker& MemoryTracker::getInstance() {
    static MemoryTracker instance;
    return instance;
}

void MemoryTracker::trackAllocation(void* ptr, std::size_t size, std::size_t alignment, 
                                   const std::string& tag, const std::string& file, int line, bool isArray) {
    if (!trackingEnabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    AllocationInfo info(ptr, size, alignment, tag, file, line, isArray);
    allocations[ptr] = info;
    
    globalStats.totalAllocated += size;
    globalStats.currentUsage += size;
    globalStats.allocationCount++;
    globalStats.peakUsage = std::max(globalStats.peakUsage, globalStats.currentUsage);
}

void MemoryTracker::trackDeallocation(void* ptr) {
    if (!trackingEnabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = allocations.find(ptr);
    if (it != allocations.end()) {
        globalStats.totalFreed += it->second.size;
        globalStats.currentUsage -= it->second.size;
        globalStats.deallocationCount++;
        allocations.erase(it);
    }
}

void MemoryTracker::trackReallocation(void* oldPtr, void* newPtr, std::size_t newSize) {
    if (!trackingEnabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // Remove old allocation
    auto it = allocations.find(oldPtr);
    if (it != allocations.end()) {
        globalStats.totalFreed += it->second.size;
        globalStats.currentUsage -= it->second.size;
        allocations.erase(it);
    }
    
    // Add new allocation
    AllocationInfo info(newPtr, newSize, 8, "realloc", "", 0);
    allocations[newPtr] = info;
    
    globalStats.totalAllocated += newSize;
    globalStats.currentUsage += newSize;
    globalStats.allocationCount++;
    globalStats.peakUsage = std::max(globalStats.peakUsage, globalStats.currentUsage);
}

MemoryStats MemoryTracker::getGlobalStats() const {
    std::lock_guard<std::mutex> lock(mutex);
    return globalStats;
}

std::vector<AllocationInfo> MemoryTracker::getActiveAllocations() const {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<AllocationInfo> result;
    result.reserve(allocations.size());
    
    for (const auto& pair : allocations) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<AllocationInfo> MemoryTracker::getLeakedAllocations() const {
    return getActiveAllocations(); // For now, all active allocations are considered leaks
}

void MemoryTracker::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    allocations.clear();
    globalStats.reset();
}

void MemoryTracker::dumpStats() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::cout << "\n=== Memory Statistics ===" << std::endl;
    std::cout << "Total Allocated: " << globalStats.totalAllocated << " bytes" << std::endl;
    std::cout << "Total Freed: " << globalStats.totalFreed << " bytes" << std::endl;
    std::cout << "Current Usage: " << globalStats.currentUsage << " bytes" << std::endl;
    std::cout << "Peak Usage: " << globalStats.peakUsage << " bytes" << std::endl;
    std::cout << "Allocation Count: " << globalStats.allocationCount << std::endl;
    std::cout << "Deallocation Count: " << globalStats.deallocationCount << std::endl;
    std::cout << "Active Allocations: " << allocations.size() << std::endl;
    std::cout << "========================\n" << std::endl;
}

void MemoryTracker::dumpLeaks() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (allocations.empty()) {
        std::cout << "No memory leaks detected!" << std::endl;
        return;
    }
    
    std::cout << "\n=== Memory Leaks Detected ===" << std::endl;
    std::cout << "Total leaks: " << allocations.size() << std::endl;
    
    for (const auto& pair : allocations) {
        const AllocationInfo& info = pair.second;
        std::cout << "Leak: " << info.ptr << " (" << info.size << " bytes)";
        if (!info.tag.empty()) {
            std::cout << " - " << info.tag;
        }
        if (!info.file.empty()) {
            std::cout << " at " << info.file << ":" << info.line;
        }
        std::cout << std::endl;
    }
    std::cout << "============================\n" << std::endl;
}

// MemoryManager Implementation
MemoryManager& MemoryManager::getInstance() {
    static MemoryManager instance;
    return instance;
}

void MemoryManager::initialize(const MemoryPoolConfig& defaultConfig) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (initialized) return;
    
    defaultPool = std::make_unique<MemoryPool>(defaultConfig);
    initialized = true;
}

void MemoryManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!initialized) return;
    
    // Dump final statistics
    tracker.dumpStats();
    tracker.dumpLeaks();
    
    // Clean up all allocators
    stackAllocators.clear();
    pools.clear();
    defaultPool.reset();
    
    initialized = false;
}

void* MemoryManager::allocate(std::size_t size, std::size_t alignment, 
                             const std::string& tag, const std::string& file, int line) {
    if (!initialized) {
        // Fallback to system allocator
#ifdef _WIN32
        return _aligned_malloc(size, alignment);
#else
        return std::aligned_alloc(alignment, size);
#endif
    }
    
    // Try default pool first
    if (defaultPool) {
        void* ptr = defaultPool->allocate(size, alignment);
        if (ptr) {
            if (tracker.isTrackingEnabled()) {
                tracker.trackAllocation(ptr, size, alignment, tag, file, line);
            }
            return ptr;
        }
    }
    
    // Fallback to system allocator
#ifdef _WIN32
    void* ptr = _aligned_malloc(size, alignment);
#else
    void* ptr = std::aligned_alloc(alignment, size);
#endif
    if (ptr && tracker.isTrackingEnabled()) {
        tracker.trackAllocation(ptr, size, alignment, tag, file, line);
    }
    return ptr;
}

void MemoryManager::deallocate(void* ptr) {
    if (!ptr) return;
    
    if (!initialized) {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        std::free(ptr);
#endif
        return;
    }
    
    // Try to deallocate from default pool
    if (defaultPool && defaultPool->owns(ptr)) {
        defaultPool->deallocate(ptr);
        return;
    }
    
    // Try other pools
    for (auto& pool : pools) {
        if (pool->owns(ptr)) {
            pool->deallocate(ptr);
            return;
        }
    }
    
    // Try stack allocators
    for (auto& stack : stackAllocators) {
        if (stack->owns(ptr)) {
            // Stack allocators don't support individual deallocation
            return;
        }
    }
    
    // Fallback to system deallocator
#ifdef _WIN32
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

void* MemoryManager::reallocate(void* ptr, std::size_t newSize) {
    if (!ptr) return allocate(newSize);
    if (newSize == 0) {
        deallocate(ptr);
        return nullptr;
    }
    
    if (!initialized) {
        return std::realloc(ptr, newSize);
    }
    
    // Try to reallocate from default pool
    if (defaultPool && defaultPool->owns(ptr)) {
        void* newPtr = defaultPool->reallocate(ptr, newSize);
        if (newPtr && tracker.isTrackingEnabled()) {
            tracker.trackReallocation(ptr, newPtr, newSize);
        }
        return newPtr;
    }
    
    // Try other pools
    for (auto& pool : pools) {
        if (pool->owns(ptr)) {
            void* newPtr = pool->reallocate(ptr, newSize);
            if (newPtr && tracker.isTrackingEnabled()) {
                tracker.trackReallocation(ptr, newPtr, newSize);
            }
            return newPtr;
        }
    }
    
    // Fallback to system reallocator
    void* newPtr = std::realloc(ptr, newSize);
    if (newPtr && tracker.isTrackingEnabled()) {
        tracker.trackReallocation(ptr, newPtr, newSize);
    }
    return newPtr;
}

MemoryPool* MemoryManager::createPool(const MemoryPoolConfig& config) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto pool = std::make_unique<MemoryPool>(config);
    MemoryPool* poolPtr = pool.get();
    pools.push_back(std::move(pool));
    
    return poolPtr;
}

void MemoryManager::destroyPool(MemoryPool* pool) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = std::find_if(pools.begin(), pools.end(),
                          [pool](const std::unique_ptr<MemoryPool>& p) { return p.get() == pool; });
    if (it != pools.end()) {
        pools.erase(it);
    }
}

StackAllocator* MemoryManager::createStackAllocator(std::size_t size) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto stack = std::make_unique<StackAllocator>(size);
    StackAllocator* stackPtr = stack.get();
    stackAllocators.push_back(std::move(stack));
    
    return stackPtr;
}

void MemoryManager::destroyStackAllocator(StackAllocator* allocator) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = std::find_if(stackAllocators.begin(), stackAllocators.end(),
                          [allocator](const std::unique_ptr<StackAllocator>& s) { return s.get() == allocator; });
    if (it != stackAllocators.end()) {
        stackAllocators.erase(it);
    }
}

MemoryStats MemoryManager::getGlobalStats() const {
    return tracker.getGlobalStats();
}

std::size_t MemoryManager::getAlignmentPadding(std::size_t size, std::size_t alignment) {
    std::size_t remainder = size % alignment;
    return remainder == 0 ? 0 : alignment - remainder;
}

bool MemoryManager::isPowerOfTwo(std::size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

std::size_t MemoryManager::nextPowerOfTwo(std::size_t value) {
    if (value == 0) return 1;
    
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    value++;
    
    return value;
}

} // namespace Core
} // namespace VaporFrame 