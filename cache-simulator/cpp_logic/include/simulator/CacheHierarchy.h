#ifndef HIERARCHICAL_CACHE_H
#define HIERARCHICAL_CACHE_H

#include "Cache.h"
#include <string>
#include <vector>
#include <memory>


enum CacheLevel {
    L1 = 1,
    L2 = 2,
    L3 = 3,
    MAIN_MEMORY = 4
};


struct LevelAccessResult {
    CacheLevel level;
    bool hit;
    std::string status;
    int access_time;
    unsigned int evicted_tag;
    bool had_eviction;

    LevelAccessResult(CacheLevel lvl = L1, bool h = false, std::string s = "MISS", int time = 1)
        : level(lvl), hit(h), status(s), access_time(time), evicted_tag(0), had_eviction(false) {}
};


struct HierarchyAccessResult {
    std::vector<LevelAccessResult> level_results;
    CacheLevel final_level;
    int total_access_time;
    bool overall_hit;
    std::string access_path;

    HierarchyAccessResult() : final_level(MAIN_MEMORY), total_access_time(0), overall_hit(false) {}
};


struct LevelConfig {
    int cache_size;
    int block_size;
    int associativity;
    int access_time;
    std::string name;
    WritePolicy write_policy;
    WriteMissPolicy write_miss_policy;

    LevelConfig(int size = 1024, int block = 64, int assoc = 1, int time = 1, std::string n = "L1",
               WritePolicy wp = WRITE_BACK, WriteMissPolicy wmp = WRITE_ALLOCATE)
        : cache_size(size), block_size(block), associativity(assoc), access_time(time), name(n),
          write_policy(wp), write_miss_policy(wmp) {}
};


class HierarchicalCache {
private:
    std::unique_ptr<SetAssociativeCache> l1_cache;
    std::unique_ptr<SetAssociativeCache> l2_cache;
    std::unique_ptr<SetAssociativeCache> l3_cache;

    LevelConfig l1_config;
    LevelConfig l2_config;
    LevelConfig l3_config;

    int main_memory_access_time;


    int total_requests;
    int l1_hits, l2_hits, l3_hits, main_memory_accesses;
    int total_access_time_accumulated;


    bool inclusive_policy;

public:
    HierarchicalCache(const LevelConfig& l1_cfg = LevelConfig(1024, 64, 2, 1, "L1", WRITE_BACK, WRITE_ALLOCATE),
                     const LevelConfig& l2_cfg = LevelConfig(8192, 64, 4, 10, "L2", WRITE_BACK, WRITE_ALLOCATE),
                     const LevelConfig& l3_cfg = LevelConfig(32768, 64, 8, 30, "L3", WRITE_BACK, WRITE_ALLOCATE),
                     int memory_time = 100,
                     bool inclusive = true);

    ~HierarchicalCache();


    HierarchyAccessResult accessMemory(unsigned int address);
    HierarchyAccessResult writeMemory(unsigned int address, int data);


private:
    LevelAccessResult accessLevel(SetAssociativeCache* cache, unsigned int address,
                                CacheLevel level, int access_time);


    void fillDataUp(unsigned int address, CacheLevel from_level);
    void invalidateBelow(unsigned int address, CacheLevel from_level);

public:

    void reconfigure(const LevelConfig& l1_cfg, const LevelConfig& l2_cfg,
                    const LevelConfig& l3_cfg, int memory_time = 100);

    void resetStatistics();
    void printStatistics() const;
    std::string getStatisticsJSON() const;


    std::string getL1StateJSON() const;
    std::string getL2StateJSON() const;
    std::string getL3StateJSON() const;


    LevelConfig getL1Config() const { return l1_config; }
    LevelConfig getL2Config() const { return l2_config; }
    LevelConfig getL3Config() const { return l3_config; }
    int getMainMemoryTime() const { return main_memory_access_time; }


    int getTotalRequests() const { return total_requests; }
    int getL1Hits() const { return l1_hits; }
    int getL2Hits() const { return l2_hits; }
    int getL3Hits() const { return l3_hits; }
    int getMainMemoryAccesses() const { return main_memory_accesses; }
    double getAverageAccessTime() const;


    double getL1HitRate() const;
    double getL2HitRate() const;
    double getL3HitRate() const;
    double getOverallHitRate() const;


    std::string getLastAccessDetails() const;
    void printCacheStates() const;
};

#endif
