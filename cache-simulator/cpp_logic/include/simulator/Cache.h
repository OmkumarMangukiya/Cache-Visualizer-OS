#ifndef SET_ASSOCIATIVE_CACHE_H
#define SET_ASSOCIATIVE_CACHE_H
using namespace std;
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <climits>
#include <random>
#include <fstream>
#include <sstream>


enum AccessType {
    READ = 0,
    WRITE = 1
};


struct TraceEntry {
    AccessType type;
    unsigned int address;
    int data;

    TraceEntry(AccessType t = READ, unsigned int addr = 0, int d = 0)
        : type(t), address(addr), data(d) {}
};


struct TraceResults {
    int total_accesses;
    int reads;
    int writes;
    int hits;
    int misses;
    int writebacks;
    int dirty_evictions;
    double hit_rate;
    double miss_rate;
    string replacement_policy;
    string write_policy;
    string write_miss_policy;

    TraceResults() : total_accesses(0), reads(0), writes(0), hits(0), misses(0),
                    writebacks(0), dirty_evictions(0), hit_rate(0.0), miss_rate(0.0) {}
};


enum ReplacementPolicy {
    LRU = 0,
    FIFO = 1,
    RANDOM = 2,
    MRU = 3
};


enum WritePolicy {
    WRITE_THROUGH = 0,
    WRITE_BACK = 1
};


enum WriteMissPolicy {
    WRITE_ALLOCATE = 0,
    NO_WRITE_ALLOCATE = 1
};


struct AssociativeCacheConfig {
    int cache_size;
    int block_size;
    int associativity;
    int num_sets;
    int address_bits;
    int offset_bits;
    int index_bits;
    int tag_bits;
    ReplacementPolicy replacement_policy;
    WritePolicy write_policy;
    WriteMissPolicy write_miss_policy;
};


struct AssociativeCacheLine {
    bool valid;
    bool dirty;
    unsigned int tag;
    unsigned int lru_counter;
    unsigned int fifo_timestamp;
    vector<int> data;

    AssociativeCacheLine() : valid(false), dirty(false), tag(0), lru_counter(0), fifo_timestamp(0) {}

    void updateLRU(unsigned int counter) {
        lru_counter = counter;
    }

    void updateFIFO(unsigned int timestamp) {
        fifo_timestamp = timestamp;
    }

    void markDirty() {
        dirty = true;
    }

    void clearDirty() {
        dirty = false;
    }
};


struct CacheSet {
    vector<AssociativeCacheLine> lines;

    CacheSet(int associativity, int block_size) {
        lines.resize(associativity);
        for (auto& line : lines) {
            line.data.resize(block_size / 4);
        }
    }


    int findLine(unsigned int tag) {
        for (size_t i = 0; i < lines.size(); i++) {
            if (lines[i].valid && lines[i].tag == tag) {
                return i;
            }
        }
        return -1;
    }


    int findEmptyLine() {
        for (size_t i = 0; i < lines.size(); i++) {
            if (!lines[i].valid) {
                return i;
            }
        }
        return -1;
    }


    int findLRULine() {
        int lru_index = -1;
        unsigned int min_counter = UINT_MAX;

        // Only consider valid lines for LRU replacement
        for (size_t i = 0; i < lines.size(); i++) {
            if (lines[i].valid && lines[i].lru_counter < min_counter) {
                min_counter = lines[i].lru_counter;
                lru_index = i;
            }
        }
        
        // If no valid lines found (shouldn't happen), return 0
        return (lru_index != -1) ? lru_index : 0;
    }


    int findFIFOLine() {
        int fifo_index = -1;
        unsigned int min_timestamp = UINT_MAX;

        // Only consider valid lines for FIFO replacement
        for (size_t i = 0; i < lines.size(); i++) {
            if (lines[i].valid && lines[i].fifo_timestamp < min_timestamp) {
                min_timestamp = lines[i].fifo_timestamp;
                fifo_index = i;
            }
        }
        
        // If no valid lines found (shouldn't happen), return 0
        return (fifo_index != -1) ? fifo_index : 0;
    }


    int findRandomLine() {
        static random_device rd;
        static mt19937 gen(rd());
        uniform_int_distribution<> dis(0, lines.size() - 1);
        return dis(gen);
    }

    int findMRULine() {
        int mru_index = -1;
        unsigned int max_counter = 0;

        // Find the line with the highest (most recent) counter value
        for (size_t i = 0; i < lines.size(); i++) {
            if (lines[i].valid && lines[i].lru_counter > max_counter) {
                max_counter = lines[i].lru_counter;
                mru_index = i;
            }
        }
        
        // If no valid lines found (shouldn't happen), return 0
        return (mru_index != -1) ? mru_index : 0;
    }
};


class SetAssociativeCache {
private:
    vector<CacheSet> cache_sets;
    AssociativeCacheConfig config;
    unsigned int global_lru_counter;
    unsigned int global_fifo_timestamp;


    int total_accesses;
    int cache_hits;
    int cache_misses;
    int compulsory_misses;
    int conflict_misses;
    int writebacks;
    int dirty_evictions;


    struct LastAccess {
        int set_index;
        int line_index;
        bool was_hit;
        bool was_compulsory_miss;
        bool was_write_operation;
        bool was_dirty_eviction;
        int evicted_line_index;
        unsigned int evicted_tag;
        bool had_eviction;

        LastAccess() : set_index(-1), line_index(-1), was_hit(false),
                      was_compulsory_miss(false), was_write_operation(false),
                      was_dirty_eviction(false), evicted_line_index(-1),
                      evicted_tag(0), had_eviction(false) {}
    } last_access;

public:
    SetAssociativeCache(int cache_size = 1024, int block_size = 64, int associativity = 1,
                       ReplacementPolicy rp = LRU, WritePolicy wp = WRITE_THROUGH,
                       WriteMissPolicy wmp = WRITE_ALLOCATE);


    bool accessMemory(unsigned int address);
    bool writeMemory(unsigned int address, int data = 0);


    void setReplacementPolicy(ReplacementPolicy rp) { config.replacement_policy = rp; }
    void setWritePolicy(WritePolicy wp) { config.write_policy = wp; }
    void setWriteMissPolicy(WriteMissPolicy wmp) { config.write_miss_policy = wmp; }
    ReplacementPolicy getReplacementPolicy() const { return config.replacement_policy; }
    WritePolicy getWritePolicy() const { return config.write_policy; }
    WriteMissPolicy getWriteMissPolicy() const { return config.write_miss_policy; }


    unsigned int getTag(unsigned int address);
    unsigned int getSetIndex(unsigned int address);
    unsigned int getOffset(unsigned int address);


    const vector<CacheSet>& getCacheSets() const { return cache_sets; }
    const AssociativeCacheConfig& getConfig() const { return config; }
    const LastAccess& getLastAccess() const { return last_access; }


    int getTotalAccesses() const { return total_accesses; }
    int getCacheHits() const { return cache_hits; }
    int getCacheMisses() const { return cache_misses; }
    int getCompulsoryMisses() const { return compulsory_misses; }
    int getConflictMisses() const { return conflict_misses; }
    int getWritebacks() const { return writebacks; }
    int getDirtyEvictions() const { return dirty_evictions; }
    double getHitRate() const {
        return total_accesses > 0 ? (double)cache_hits / total_accesses : 0.0;
    }


    string getReplacementPolicyString() const {
        switch(config.replacement_policy) {
            case LRU: return "LRU";
            case FIFO: return "FIFO";
            case RANDOM: return "Random";
            default: return "Unknown";
        }
    }
    string getWritePolicyString() const {
        return (config.write_policy == WRITE_THROUGH) ? "Write-Through" : "Write-Back";
    }
    string getWriteMissPolicyString() const {
        return (config.write_miss_policy == WRITE_ALLOCATE) ? "Write-Allocate" : "No-Write-Allocate";
    }


    vector<TraceEntry> loadTraceFile(const string& filename);
    TraceResults processTraceFile(const string& filename);
    TraceResults processTrace(const vector<TraceEntry>& trace);


    void reset();


    void displayCache() const;
    void displayCacheDetailed() const;

private:

    int findEvictionLine(CacheSet& set);
    void updateReplacementCounters(CacheSet& set, int line_index);
    void initializeBlockCounters(CacheSet& set, int line_index);
};

#endif
