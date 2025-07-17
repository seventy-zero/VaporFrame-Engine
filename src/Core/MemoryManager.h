#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>
#include <string>
#include <optional>

namespace VaporFrame {
namespace Core {

// Forward declarations
class MemoryPool;
class MemoryTracker;
class AllocatorBase;

/**
 * @brief Memory allocation statistics
 */
struct MemoryStats {
    std::size_t totalAllocated = 0;
    std::size_t totalFreed = 0;
    std::size_t peakUsage = 0;
    std::size_t currentUsage = 0;
    std::size_t allocationCount = 0;
    std::size_t deallocationCount = 0;
    std::size_t fragmentation = 0;
    
    void reset() {
        totalAllocated = 0;
        totalFreed = 0;
        peakUsage = 0;
        currentUsage = 0;
        allocationCount = 0;
        deallocationCount = 0;
        fragmentation = 0;
    }
};

/**
 * @brief Memory allocation information for tracking
 */
struct AllocationInfo {
    void* ptr = nullptr;
    std::size_t size = 0;
    std::size_t alignment = 0;
    std::string tag;
    std::string file;
    int line = 0;
    std::chrono::steady_clock::time_point timestamp;
    bool isArray = false;
    
    AllocationInfo() = default;
    AllocationInfo(void* p, std::size_t s, std::size_t align, const std::string& t, 
                   const std::string& f, int l, bool arr = false)
        : ptr(p), size(s), alignment(align), tag(t), file(f), line(l), 
          timestamp(std::chrono::steady_clock::now()), isArray(arr) {}
};

/**
 * @brief Memory pool configuration
 */
struct MemoryPoolConfig {
    std::size_t initialSize = 1024 * 1024; // 1MB default
    std::size_t maxSize = 100 * 1024 * 1024; // 100MB default
    std::size_t blockSize = 4096; // 4KB default
    std::size_t alignment = 16;
    bool enableTracking = true;
    std::string name = "DefaultPool";
    
    MemoryPoolConfig() = default;
    MemoryPoolConfig(std::size_t init, std::size_t max, std::size_t block, 
                     std::size_t align = 16, bool track = true, const std::string& n = "DefaultPool")
        : initialSize(init), maxSize(max), blockSize(block), alignment(align), 
          enableTracking(track), name(n) {}
};

/**
 * @brief Base allocator interface
 */
class AllocatorBase {
public:
    virtual ~AllocatorBase() = default;
    
    virtual void* allocate(std::size_t size, std::size_t alignment = 8) = 0;
    virtual void deallocate(void* ptr) = 0;
    virtual void* reallocate(void* ptr, std::size_t newSize) = 0;
    virtual std::size_t getSize(void* ptr) const = 0;
    virtual bool owns(void* ptr) const = 0;
    virtual MemoryStats getStats() const = 0;
    virtual void reset() = 0;
    
    // Optional: Get allocator name for debugging
    virtual const std::string& getName() const = 0;
};

/**
 * @brief Memory pool for efficient small allocations
 */
class MemoryPool : public AllocatorBase {
public:
    explicit MemoryPool(const MemoryPoolConfig& config = MemoryPoolConfig{});
    ~MemoryPool();
    
    // AllocatorBase interface
    void* allocate(std::size_t size, std::size_t alignment = 8) override;
    void deallocate(void* ptr) override;
    void* reallocate(void* ptr, std::size_t newSize) override;
    std::size_t getSize(void* ptr) const override;
    bool owns(void* ptr) const override;
    MemoryStats getStats() const override;
    void reset() override;
    const std::string& getName() const override { return config.name; }
    
    // Pool-specific methods
    bool expand(std::size_t additionalSize);
    void defragment();
    std::size_t getFragmentation() const;
    
private:
    struct Block {
        void* data = nullptr;
        std::size_t size = 0;
        bool used = false;
        Block* next = nullptr;
        Block* prev = nullptr;
    };
    
    MemoryPoolConfig config;
    std::vector<void*> pools;
    std::vector<Block*> freeBlocks;
    std::unordered_map<void*, Block*> blockMap;
    MemoryStats stats;
    mutable std::mutex mutex;
    
    Block* findFreeBlock(std::size_t size, std::size_t alignment);
    void splitBlock(Block* block, std::size_t size);
    void mergeAdjacentBlocks(Block* block);
    void* allocateFromSystem(std::size_t size);
    void deallocateFromSystem(void* ptr);
};

/**
 * @brief Stack allocator for temporary allocations
 */
class StackAllocator : public AllocatorBase {
public:
    explicit StackAllocator(std::size_t size);
    ~StackAllocator();
    
    // AllocatorBase interface
    void* allocate(std::size_t size, std::size_t alignment = 8) override;
    void deallocate(void* ptr) override;
    void* reallocate(void* ptr, std::size_t newSize) override;
    std::size_t getSize(void* ptr) const override;
    bool owns(void* ptr) const override;
    MemoryStats getStats() const override;
    void reset() override;
    const std::string& getName() const override { return "StackAllocator"; }
    
    // Stack-specific methods
    void* getMarker() const;
    void freeToMarker(void* marker);
    std::size_t getCurrentOffset() const;
    
private:
    void* memory = nullptr;
    std::size_t totalSize = 0;
    std::size_t currentOffset = 0;
    MemoryStats stats;
    mutable std::mutex mutex;
};

/**
 * @brief Memory tracker for debugging and profiling
 */
class MemoryTracker {
public:
    static MemoryTracker& getInstance();
    
    void trackAllocation(void* ptr, std::size_t size, std::size_t alignment, 
                        const std::string& tag, const std::string& file, int line, bool isArray = false);
    void trackDeallocation(void* ptr);
    void trackReallocation(void* oldPtr, void* newPtr, std::size_t newSize);
    
    MemoryStats getGlobalStats() const;
    std::vector<AllocationInfo> getActiveAllocations() const;
    std::vector<AllocationInfo> getLeakedAllocations() const;
    
    void enableTracking(bool enable) { trackingEnabled = enable; }
    bool isTrackingEnabled() const { return trackingEnabled; }
    
    void reset();
    void dumpStats() const;
    void dumpLeaks() const;
    
private:
    MemoryTracker() = default;
    ~MemoryTracker() = default;
    MemoryTracker(const MemoryTracker&) = delete;
    MemoryTracker& operator=(const MemoryTracker&) = delete;
    
    std::unordered_map<void*, AllocationInfo> allocations;
    MemoryStats globalStats;
    mutable std::mutex mutex;
    std::atomic<bool> trackingEnabled{true};
};

/**
 * @brief Main memory manager singleton
 */
class MemoryManager {
public:
    static MemoryManager& getInstance();
    
    // Initialization and cleanup
    void initialize(const MemoryPoolConfig& defaultConfig = MemoryPoolConfig{});
    void shutdown();
    
    // Global allocation methods
    void* allocate(std::size_t size, std::size_t alignment = 8, 
                  const std::string& tag = "", const std::string& file = "", int line = 0);
    void deallocate(void* ptr);
    void* reallocate(void* ptr, std::size_t newSize);
    
    // Pool management
    MemoryPool* createPool(const MemoryPoolConfig& config);
    void destroyPool(MemoryPool* pool);
    MemoryPool* getDefaultPool() const { return defaultPool.get(); }
    
    // Stack allocator management
    StackAllocator* createStackAllocator(std::size_t size);
    void destroyStackAllocator(StackAllocator* allocator);
    
    // Statistics and debugging
    MemoryStats getGlobalStats() const;
    MemoryTracker& getTracker() { return tracker; }
    
    // Memory utilities
    std::size_t getAlignmentPadding(std::size_t size, std::size_t alignment);
    bool isPowerOfTwo(std::size_t value);
    std::size_t nextPowerOfTwo(std::size_t value);
    
private:
    MemoryManager() = default;
    ~MemoryManager() = default;
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    
    std::unique_ptr<MemoryPool> defaultPool;
    std::vector<std::unique_ptr<MemoryPool>> pools;
    std::vector<std::unique_ptr<StackAllocator>> stackAllocators;
    MemoryTracker& tracker = MemoryTracker::getInstance();
    bool initialized = false;
    mutable std::mutex mutex;
};

// Global convenience functions
inline void* Allocate(std::size_t size, std::size_t alignment = 8, 
                     const std::string& tag = "", const std::string& file = "", int line = 0) {
    return MemoryManager::getInstance().allocate(size, alignment, tag, file, line);
}

inline void Deallocate(void* ptr) {
    MemoryManager::getInstance().deallocate(ptr);
}

inline void* Reallocate(void* ptr, std::size_t newSize) {
    return MemoryManager::getInstance().reallocate(ptr, newSize);
}

// Macro for automatic file/line tracking
#define VF_ALLOCATE(size, alignment, tag) \
    VaporFrame::Core::Allocate(size, alignment, tag, __FILE__, __LINE__)

#define VF_DEALLOCATE(ptr) \
    VaporFrame::Core::Deallocate(ptr)

#define VF_REALLOCATE(ptr, newSize) \
    VaporFrame::Core::Reallocate(ptr, newSize)

} // namespace Core
} // namespace VaporFrame 