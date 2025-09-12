#include "simulator/CacheHierarchy.h"
#include <sstream>
#include <iomanip>
#include <memory>

HierarchicalCache::HierarchicalCache(const LevelConfig& l1_cfg, const LevelConfig& l2_cfg,
                                   const LevelConfig& l3_cfg, int memory_time, bool inclusive)
    : l1_config(l1_cfg), l2_config(l2_cfg), l3_config(l3_cfg),
      main_memory_access_time(memory_time), inclusive_policy(inclusive),
      total_requests(0), l1_hits(0), l2_hits(0), l3_hits(0), main_memory_accesses(0),
      total_access_time_accumulated(0) {


    l1_cache.reset(new SetAssociativeCache(l1_cfg.cache_size, l1_cfg.block_size, l1_cfg.associativity,
                                          l1_cfg.write_policy, l1_cfg.write_miss_policy));
    l2_cache.reset(new SetAssociativeCache(l2_cfg.cache_size, l2_cfg.block_size, l2_cfg.associativity,
                                          l2_cfg.write_policy, l2_cfg.write_miss_policy));
    l3_cache.reset(new SetAssociativeCache(l3_cfg.cache_size, l3_cfg.block_size, l3_cfg.associativity,
                                          l3_cfg.write_policy, l3_cfg.write_miss_policy));

    std::cout << "Hierarchical Cache System Initialized:" << std::endl;
    std::cout << "L1: " << l1_cfg.cache_size << "B, " << l1_cfg.associativity << "-way, " << l1_cfg.access_time << " cycles, "
              << (l1_cfg.write_policy == WRITE_THROUGH ? "Write-Through" : "Write-Back") << std::endl;
    std::cout << "L2: " << l2_cfg.cache_size << "B, " << l2_cfg.associativity << "-way, " << l2_cfg.access_time << " cycles, "
              << (l2_cfg.write_policy == WRITE_THROUGH ? "Write-Through" : "Write-Back") << std::endl;
    std::cout << "L3: " << l3_cfg.cache_size << "B, " << l3_cfg.associativity << "-way, " << l3_cfg.access_time << " cycles, "
              << (l3_cfg.write_policy == WRITE_THROUGH ? "Write-Through" : "Write-Back") << std::endl;
    std::cout << "Main Memory: " << memory_time << " cycles" << std::endl;
    std::cout << "Inclusion Policy: " << (inclusive ? "Inclusive" : "Exclusive") << std::endl << std::endl;
}

HierarchicalCache::~HierarchicalCache() = default;

HierarchyAccessResult HierarchicalCache::accessMemory(unsigned int address) {
    HierarchyAccessResult result;
    total_requests++;

    std::ostringstream path;
    int accumulated_time = 0;


    LevelAccessResult l1_result = accessLevel(l1_cache.get(), address, L1, l1_config.access_time);
    result.level_results.push_back(l1_result);
    accumulated_time += l1_result.access_time;
    path << "L1:" << l1_result.status;

    if (l1_result.hit) {

        l1_hits++;
        result.final_level = L1;
        result.overall_hit = true;
        result.total_access_time = accumulated_time;
        result.access_path = path.str();
        total_access_time_accumulated += accumulated_time;
        return result;
    }


    LevelAccessResult l2_result = accessLevel(l2_cache.get(), address, L2, l2_config.access_time);
    result.level_results.push_back(l2_result);
    accumulated_time += l2_result.access_time;
    path << " -> L2:" << l2_result.status;

    if (l2_result.hit) {

        l2_hits++;
        result.final_level = L2;
        result.overall_hit = true;


        fillDataUp(address, L2);


        LevelAccessResult l1_fill(L1, false, "FILL", 1);
        result.level_results.push_back(l1_fill);
        accumulated_time += 1;
        path << " -> L1:FILL";

        result.total_access_time = accumulated_time;
        result.access_path = path.str();
        total_access_time_accumulated += accumulated_time;
        return result;
    }


    LevelAccessResult l3_result = accessLevel(l3_cache.get(), address, L3, l3_config.access_time);
    result.level_results.push_back(l3_result);
    accumulated_time += l3_result.access_time;
    path << " -> L3:" << l3_result.status;

    if (l3_result.hit) {

        l3_hits++;
        result.final_level = L3;
        result.overall_hit = true;


        fillDataUp(address, L3);


        LevelAccessResult l2_fill(L2, false, "FILL", 2);
        LevelAccessResult l1_fill(L1, false, "FILL", 1);
        result.level_results.push_back(l2_fill);
        result.level_results.push_back(l1_fill);
        accumulated_time += 3;
        path << " -> L2:FILL -> L1:FILL";

        result.total_access_time = accumulated_time;
        result.access_path = path.str();
        total_access_time_accumulated += accumulated_time;
        return result;
    }


    main_memory_accesses++;
    result.final_level = MAIN_MEMORY;
    result.overall_hit = false;


    LevelAccessResult memory_result(MAIN_MEMORY, true, "HIT", main_memory_access_time);
    result.level_results.push_back(memory_result);
    accumulated_time += main_memory_access_time;
    path << " -> MEM:HIT";


    fillDataUp(address, MAIN_MEMORY);


    LevelAccessResult l3_fill(L3, false, "FILL", 3);
    LevelAccessResult l2_fill(L2, false, "FILL", 2);
    LevelAccessResult l1_fill(L1, false, "FILL", 1);
    result.level_results.push_back(l3_fill);
    result.level_results.push_back(l2_fill);
    result.level_results.push_back(l1_fill);
    accumulated_time += 6;
    path << " -> L3:FILL -> L2:FILL -> L1:FILL";

    result.total_access_time = accumulated_time;
    result.access_path = path.str();
    total_access_time_accumulated += accumulated_time;

    return result;
}

LevelAccessResult HierarchicalCache::accessLevel(SetAssociativeCache* cache, unsigned int address,
                                                CacheLevel level, int access_time) {
    LevelAccessResult result(level, false, "MISS", access_time);


    bool hit = cache->accessMemory(address);

    if (hit) {
        result.hit = true;
        result.status = "HIT";
    } else {
        result.hit = false;
        result.status = "MISS";



        result.had_eviction = false;
        result.evicted_tag = 0;
    }

    return result;
}

void HierarchicalCache::fillDataUp(unsigned int address, CacheLevel from_level) {
    switch (from_level) {
        case MAIN_MEMORY:

            l3_cache->accessMemory(address);
            [[fallthrough]];
        case L3:

            l2_cache->accessMemory(address);
            [[fallthrough]];
        case L2:

            l1_cache->accessMemory(address);
            break;
        case L1:

            break;
    }
}

void HierarchicalCache::invalidateBelow(unsigned int address, CacheLevel from_level) {


}

void HierarchicalCache::reconfigure(const LevelConfig& l1_cfg, const LevelConfig& l2_cfg,
                                  const LevelConfig& l3_cfg, int memory_time) {
    l1_config = l1_cfg;
    l2_config = l2_cfg;
    l3_config = l3_cfg;
    main_memory_access_time = memory_time;


    l1_cache.reset(new SetAssociativeCache(l1_cfg.cache_size, l1_cfg.block_size, l1_cfg.associativity));
    l2_cache.reset(new SetAssociativeCache(l2_cfg.cache_size, l2_cfg.block_size, l2_cfg.associativity));
    l3_cache.reset(new SetAssociativeCache(l3_cfg.cache_size, l3_cfg.block_size, l3_cfg.associativity));

    resetStatistics();
}

void HierarchicalCache::resetStatistics() {
    total_requests = 0;
    l1_hits = l2_hits = l3_hits = main_memory_accesses = 0;
    total_access_time_accumulated = 0;

    l1_cache->reset();
    l2_cache->reset();
    l3_cache->reset();
}

void HierarchicalCache::printStatistics() const {
    std::cout << "\n=== HIERARCHICAL CACHE STATISTICS ===" << std::endl;
    std::cout << "Total Requests: " << total_requests << std::endl;
    std::cout << "Average Access Time: " << std::fixed << std::setprecision(2)
              << getAverageAccessTime() << " cycles" << std::endl << std::endl;

    std::cout << "L1 Cache:" << std::endl;
    std::cout << "  Hits: " << l1_hits << " (" << std::fixed << std::setprecision(1)
              << getL1HitRate() << "%)" << std::endl;
    std::cout << "  Access Time: " << l1_config.access_time << " cycles" << std::endl << std::endl;

    std::cout << "L2 Cache:" << std::endl;
    std::cout << "  Hits: " << l2_hits << " (" << std::fixed << std::setprecision(1)
              << getL2HitRate() << "%)" << std::endl;
    std::cout << "  Access Time: " << l2_config.access_time << " cycles" << std::endl << std::endl;

    std::cout << "L3 Cache:" << std::endl;
    std::cout << "  Hits: " << l3_hits << " (" << std::fixed << std::setprecision(1)
              << getL3HitRate() << "%)" << std::endl;
    std::cout << "  Access Time: " << l3_config.access_time << " cycles" << std::endl << std::endl;

    std::cout << "Main Memory:" << std::endl;
    std::cout << "  Accesses: " << main_memory_accesses << std::endl;
    std::cout << "  Access Time: " << main_memory_access_time << " cycles" << std::endl << std::endl;

    std::cout << "Overall Hit Rate: " << std::fixed << std::setprecision(1)
              << getOverallHitRate() << "%" << std::endl;
}

std::string HierarchicalCache::getStatisticsJSON() const {
    std::ostringstream json;
    json << "{";
    json << "\"total_requests\":" << total_requests << ",";
    json << "\"l1_hits\":" << l1_hits << ",";
    json << "\"l2_hits\":" << l2_hits << ",";
    json << "\"l3_hits\":" << l3_hits << ",";
    json << "\"main_memory_accesses\":" << main_memory_accesses << ",";
    json << "\"l1_hit_rate\":" << std::fixed << std::setprecision(2) << getL1HitRate() << ",";
    json << "\"l2_hit_rate\":" << std::fixed << std::setprecision(2) << getL2HitRate() << ",";
    json << "\"l3_hit_rate\":" << std::fixed << std::setprecision(2) << getL3HitRate() << ",";
    json << "\"overall_hit_rate\":" << std::fixed << std::setprecision(2) << getOverallHitRate() << ",";
    json << "\"average_access_time\":" << std::fixed << std::setprecision(2) << getAverageAccessTime();
    json << "}";
    return json.str();
}

std::string HierarchicalCache::getL1StateJSON() const {

    return "{\"state\":\"available\",\"note\":\"L1 cache state\"}";
}

std::string HierarchicalCache::getL2StateJSON() const {

    return "{\"state\":\"available\",\"note\":\"L2 cache state\"}";
}

std::string HierarchicalCache::getL3StateJSON() const {

    return "{\"state\":\"available\",\"note\":\"L3 cache state\"}";
}

double HierarchicalCache::getAverageAccessTime() const {
    if (total_requests == 0) return 0.0;
    return static_cast<double>(total_access_time_accumulated) / total_requests;
}

double HierarchicalCache::getL1HitRate() const {
    if (total_requests == 0) return 0.0;
    return (static_cast<double>(l1_hits) / total_requests) * 100.0;
}

double HierarchicalCache::getL2HitRate() const {
    int l1_misses = total_requests - l1_hits;
    if (l1_misses == 0) return 0.0;
    return (static_cast<double>(l2_hits) / l1_misses) * 100.0;
}

double HierarchicalCache::getL3HitRate() const {
    int l2_misses = (total_requests - l1_hits) - l2_hits;
    if (l2_misses == 0) return 0.0;
    return (static_cast<double>(l3_hits) / l2_misses) * 100.0;
}

double HierarchicalCache::getOverallHitRate() const {
    if (total_requests == 0) return 0.0;
    int total_hits = l1_hits + l2_hits + l3_hits;
    return (static_cast<double>(total_hits) / total_requests) * 100.0;
}

std::string HierarchicalCache::getLastAccessDetails() const {


    return "Last access details available in individual cache states";
}

void HierarchicalCache::printCacheStates() const {
    std::cout << "\n=== L1 CACHE STATE ===" << std::endl;
    l1_cache->displayCache();

    std::cout << "\n=== L2 CACHE STATE ===" << std::endl;
    l2_cache->displayCache();

    std::cout << "\n=== L3 CACHE STATE ===" << std::endl;
    l3_cache->displayCache();
}

HierarchyAccessResult HierarchicalCache::writeMemory(unsigned int address, int data) {
    HierarchyAccessResult result;
    total_requests++;

    std::ostringstream path;
    path << "WRITE 0x" << std::hex << address << std::dec << " (data=" << data << "): ";

    int cumulative_time = 0;


    bool l1_hit = l1_cache->writeMemory(address, data);
    cumulative_time += l1_config.access_time;

    LevelAccessResult l1_result(L1, l1_hit, l1_hit ? "HIT" : "MISS", l1_config.access_time);
    result.level_results.push_back(l1_result);
    path << "L1-" << (l1_hit ? "HIT" : "MISS");

    if (l1_hit) {
        l1_hits++;
        result.final_level = L1;
        result.overall_hit = true;
        path << " (COMPLETE)";
    } else {

        bool l2_hit = l2_cache->writeMemory(address, data);
        cumulative_time += l2_config.access_time;

        LevelAccessResult l2_result(L2, l2_hit, l2_hit ? "HIT" : "MISS", l2_config.access_time);
        result.level_results.push_back(l2_result);
        path << " -> L2-" << (l2_hit ? "HIT" : "MISS");

        if (l2_hit) {
            l2_hits++;
            result.final_level = L2;
            result.overall_hit = true;


            if (l1_config.write_miss_policy == WRITE_ALLOCATE) {
                l1_cache->accessMemory(address);
                path << " (FILL-L1)";
            }
        } else {

            bool l3_hit = l3_cache->writeMemory(address, data);
            cumulative_time += l3_config.access_time;

            LevelAccessResult l3_result(L3, l3_hit, l3_hit ? "HIT" : "MISS", l3_config.access_time);
            result.level_results.push_back(l3_result);
            path << " -> L3-" << (l3_hit ? "HIT" : "MISS");

            if (l3_hit) {
                l3_hits++;
                result.final_level = L3;
                result.overall_hit = true;


                if (l2_config.write_miss_policy == WRITE_ALLOCATE) {
                    l2_cache->accessMemory(address);
                    path << " (FILL-L2)";
                }
                if (l1_config.write_miss_policy == WRITE_ALLOCATE) {
                    l1_cache->accessMemory(address);
                    path << " (FILL-L1)";
                }
            } else {

                main_memory_accesses++;
                cumulative_time += main_memory_access_time;

                LevelAccessResult mem_result(MAIN_MEMORY, true, "MEMORY", main_memory_access_time);
                result.level_results.push_back(mem_result);
                result.final_level = MAIN_MEMORY;
                result.overall_hit = false;
                path << " -> MEMORY";


                if (l3_config.write_miss_policy == WRITE_ALLOCATE) {
                    l3_cache->accessMemory(address);
                    path << " (FILL-L3)";
                }
                if (l2_config.write_miss_policy == WRITE_ALLOCATE) {
                    l2_cache->accessMemory(address);
                    path << " (FILL-L2)";
                }
                if (l1_config.write_miss_policy == WRITE_ALLOCATE) {
                    l1_cache->accessMemory(address);
                    path << " (FILL-L1)";
                }
            }
        }
    }

    result.total_access_time = cumulative_time;
    result.access_path = path.str();
    total_access_time_accumulated += cumulative_time;

    std::cout << result.access_path << " [" << cumulative_time << " cycles]" << std::endl;

    return result;
}
