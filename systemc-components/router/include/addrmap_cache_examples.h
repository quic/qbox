/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _ADDRMAPCACHES_H
#define _ADDRMAPCACHES_H

#include <router.h>

namespace gs {

template <typename Key, typename Value>
class LRUCache : public AddrMapCacheBase<Key, Value>
{
    const size_t Capacity = 0x10000; // 65,536 entries for address-based caching
    using ListIt = typename std::list<std::pair<Key, Value>>::iterator;
    std::list<std::pair<Key, Value>> m_list; ///< LRU-ordered list of entries
    std::unordered_map<Key, ListIt> m_map;   ///< Hash map for O(1) key lookup

    // Statistics
    uint64_t m_hits = 0;
    uint64_t m_misses = 0;
#if THREAD_SAFE == true
    mutable std::shared_mutex m_mutex;
#endif

public:
    /**
     * @brief Retrieve value from cache and update LRU order.
     * @param key Address to look up
     * @param value Output parameter for the cached value
     * @return true if found in cache, false otherwise
     */
    bool get(const Key& key, Value& value) override
    {
#if THREAD_SAFE == true
        std::unique_lock<std::shared_mutex> l(m_mutex);
#endif
        auto it = m_map.find(key);
        if (it == m_map.end()) {
            ++m_misses;
            return false;
        }

        // Move to front (most recently used)
        m_list.splice(m_list.begin(), m_list, it->second);
        value = it->second->second;
        ++m_hits;
        return true;
    }

    /**
     * @brief Insert or update value in cache with LRU eviction.
     * @param key Address to cache
     * @param value Target info to cache
     */
    void put(const Key& key, const Value& value, [[maybe_unused]] uint64_t size) override
    {
#if THREAD_SAFE == true
        std::unique_lock<std::shared_mutex> l(m_mutex);
#endif
        // Evict least recently used entry if at capacity
        if (m_list.size() >= Capacity) {
            auto last = m_list.back();
            m_map.erase(last.first);
            m_list.pop_back();
        }

        // Insert new entry at front
        m_list.emplace_front(key, value);
        m_map[key] = m_list.begin();
    }

    /// @brief Clear all cached entries
    void clear() override
    {
        m_list.clear();
        m_map.clear();
    }

    // Statistics interface implementation
    uint64_t get_hits() const override { return m_hits; }
    uint64_t get_misses() const override { return m_misses; }
    void reset_stats() override
    {
        m_hits = 0;
        m_misses = 0;
    }
};

/**
 * @brief RegionFIFOCache - A simple FIFO cache for region-based address lookups.
 *
 * This cache uses First-In-First-Out (FIFO) eviction policy, removing the oldest
 * entry by insertion time (not by access pattern). Each entry covers an entire
 * memory region, allowing range-based lookups.
 *
 * Note: This is NOT an LRU cache - frequently accessed regions may still be evicted
 * if they were added early. For true LRU behavior, use RegionLRUCache instead.
 *
 * Each router instance has its own cache (instance-level storage).
 */
template <typename Key, typename Value>
class RegionFIFOCache : public AddrMapCacheBase<Key, Value>
{
    const size_t Capacity = 8; // Small capacity since each entry covers entire region
    using ListIt = typename std::list<std::pair<Key, Value>>::iterator;
    struct Entry {
        Key start;
        uint64_t end;
        Value value;
    };
    std::vector<Entry> m_cache;

    // Statistics
    uint64_t m_hits = 0;
    uint64_t m_misses = 0;
#if THREAD_SAFE == true
    mutable std::mutex m_mutex;
#endif

public:
    bool get(const Key& key, Value& value) override
    {
#if THREAD_SAFE == true
        std::lock_guard<std::mutex> l(m_mutex);
#endif
        for (const auto& entry : m_cache) {
            if (key >= entry.start && key < entry.end) {
                value = entry.value;
                ++m_hits;
                return true;
            }
        }
        ++m_misses;
        return false;
    }

    void put(const Key& key, const Value& value, [[maybe_unused]] uint64_t size) override
    {
#if THREAD_SAFE == true
        std::lock_guard<std::mutex> l(m_mutex);
#endif
        (void)key;  // Region cache uses canonical region start from the Value
        (void)size; // Region cache uses the target's region size
        if (!value) {
            // Do not cache negative lookups for region caches
            return;
        }
        if (m_cache.size() >= Capacity) {
            m_cache.erase(m_cache.begin());
        }
        // Store canonical region bounds to ensure future lookups within the region hit
        m_cache.emplace_back(Entry{ value->address, value->address + value->size, value });
    }

    void clear() override { m_cache.clear(); }

    // Statistics interface implementation
    uint64_t get_hits() const override { return m_hits; }
    uint64_t get_misses() const override { return m_misses; }
    void reset_stats() override
    {
        m_hits = 0;
        m_misses = 0;
    }
};

/**
 * @brief RegionLRUCache - A true LRU cache for region-based address lookups.
 *
 * This cache implements proper Least Recently Used (LRU) eviction policy using a
 * std::list-based approach. When a region is accessed, it's moved to the back of the
 * list (most recently used position). When eviction is needed, the front entry
 * (least recently used) is removed.
 *
 * Key features:
 * - True LRU eviction based on access patterns (not insertion time)
 * - List-based storage enabling O(1) splice for LRU
 * - Moves accessed entries to back (most recently used)
 * - Evicts from front (least recently used)
 * - O(n) traversal and O(1) move via splice (acceptable for small cache sizes)
 * - Instance-level storage (each router has its own cache)
 *
 * Use this cache when workloads exhibit temporal locality in region access patterns.
 *
 * Note: SystemC uses cooperative multitasking (single OS thread), so thread-local
 * storage is not needed. Each router instance maintains its own cache.
 */
template <typename Key, typename Value>
class RegionLRUCache : public AddrMapCacheBase<Key, Value>
{
    const size_t Capacity = 8; // Small capacity since each entry covers entire region

    struct CacheEntry {
        Key start;    // Region start address (inclusive)
        uint64_t end; // Region end address (exclusive)
        Value target; // Target info pointer

        CacheEntry(Key s, uint64_t e, Value t): start(s), end(e), target(t) {}
    };

    // Instance storage (each router has its own cache)
    std::list<CacheEntry> m_list;
    uint64_t m_hits = 0;
    uint64_t m_misses = 0;
#if THREAD_SAFE == true
    mutable std::mutex m_mutex;
#endif

public:
    /**
     * @brief Retrieve value from cache and update LRU order.
     *
     * Searches through the vector from back to front (most recently used first)
     * for better cache hit performance. If found, moves the entry to the back
     * (most recently used) and returns true.
     *
     * @param key Address to look up
     * @param value Output parameter for the cached target info
     * @return true if found in cache, false otherwise
     */
    bool get(const Key& key, Value& value) override
    {
#if THREAD_SAFE == true
        std::lock_guard<std::mutex> l(m_mutex);
#endif
        // Traverse from MRU to LRU using a reverse walk over the list.
        // When found, splice the node to the back (MRU) in O(1).
        for (auto it = m_list.end(); it != m_list.begin();) {
            --it;
            const CacheEntry& entry = *it;
            if (key >= entry.start && key < entry.end) {
                value = entry.target;
                if (std::next(it) != m_list.end()) {
                    m_list.splice(m_list.end(), m_list, it);
                }
                ++m_hits;
                return true;
            }
        }
        ++m_misses;
        return false;
    }

    /**
     * @brief Insert or update region in cache with LRU eviction.
     *
     * If a region with the same start address already exists, updates it and
     * moves to back. If new and at capacity, evicts the least recently used
     * entry (front of vector) before inserting at back.
     *
     * @param key Region start address
     * @param value Target info to cache
     * @param size Region size
     */
    void put(const Key& key, const Value& value, uint64_t size) override
    {
#if THREAD_SAFE == true
        std::lock_guard<std::mutex> l(m_mutex);
#endif
        (void)key;  // Region cache ignores probe address
        (void)size; // Region cache uses the target's region size
        if (!value) {
            // Do not cache negative lookups for region caches
            return;
        }

        // Evict least recently used entry (front) if at capacity
        if (m_list.size() >= Capacity) {
            m_list.pop_front();
        }

        // Insert new entry at back (most recently used) using canonical region bounds
        m_list.emplace_back(value->address, value->address + value->size, value);
    }

    /// @brief Clear all cached entries
    void clear() override { m_list.clear(); }

    // Statistics interface implementation
    uint64_t get_hits() const override { return m_hits; }
    uint64_t get_misses() const override { return m_misses; }
    void reset_stats() override
    {
        m_hits = 0;
        m_misses = 0;
    }
};
} // namespace gs
#endif // _ADDRMAPCACHES_H