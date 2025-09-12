#include "simulator/Cache.h"
#include <cstdlib>
#include <ctime>

SetAssociativeCache::SetAssociativeCache(int cache_size, int block_size, int associativity,
                                         ReplacementPolicy rp, WritePolicy wp, WriteMissPolicy wmp) {

    config.cache_size = cache_size;
    config.block_size = block_size;
    config.associativity = associativity;
    config.num_sets = cache_size / (block_size * associativity);
    config.replacement_policy = rp;
    config.write_policy = wp;
    config.write_miss_policy = wmp;


    config.offset_bits = log2(block_size);
    config.index_bits = log2(config.num_sets);
    config.address_bits = 32;
    config.tag_bits = config.address_bits - config.index_bits - config.offset_bits;


    cache_sets.reserve(config.num_sets);
    for (int i = 0; i < config.num_sets; i++) {
        cache_sets.emplace_back(associativity, block_size);
    }


    global_lru_counter = 1;
    global_fifo_timestamp = 1;
    total_accesses = 0;
    cache_hits = 0;
    cache_misses = 0;
    compulsory_misses = 0;
    conflict_misses = 0;
    writebacks = 0;
    dirty_evictions = 0;


    srand(time(nullptr));
}

bool SetAssociativeCache::accessMemory(unsigned int address) {
    total_accesses++;


    unsigned int tag = getTag(address);
    unsigned int set_index = getSetIndex(address);
    unsigned int offset = getOffset(address);


    last_access = LastAccess();
    last_access.set_index = set_index;


    CacheSet& set = cache_sets[set_index];


    int hit_line = set.findLine(tag);

    if (hit_line != -1) {

        cache_hits++;
        last_access.was_hit = true;
        last_access.line_index = hit_line;


        updateReplacementCounters(set, hit_line);

        std::cout << "CACHE HIT: Address 0x" << std::hex << address
                  << " (Tag: 0x" << tag << ", Set: " << std::dec << set_index
                  << ", Way: " << hit_line << ", Offset: " << offset << ")" << std::endl;

        return true;
    } else {

        cache_misses++;
        last_access.was_hit = false;


        int empty_line = set.findEmptyLine();

        if (empty_line != -1) {

            compulsory_misses++;
            last_access.was_compulsory_miss = true;
            last_access.line_index = empty_line;


            set.lines[empty_line].valid = true;
            set.lines[empty_line].tag = tag;
            set.lines[empty_line].dirty = false;
            initializeBlockCounters(set, empty_line);


            for (size_t i = 0; i < set.lines[empty_line].data.size(); i++) {
                set.lines[empty_line].data[i] = rand() % 1000;
            }

            std::cout << "COMPULSORY MISS: Address 0x" << std::hex << address
                      << " (Tag: 0x" << tag << ", Set: " << std::dec << set_index
                      << ", Way: " << empty_line << ", Offset: " << offset
                      << ") - Loading into empty line" << std::endl;
        } else {

            conflict_misses++;
            last_access.was_compulsory_miss = false;

            int evict_line = findEvictionLine(set);
            last_access.line_index = evict_line;
            last_access.had_eviction = true;
            last_access.evicted_line_index = evict_line;
            last_access.evicted_tag = set.lines[evict_line].tag;

            std::cout << "CONFLICT MISS: Address 0x" << std::hex << address
                      << " (Tag: 0x" << tag << ", Set: " << std::dec << set_index
                      << ", Way: " << evict_line << ", Offset: " << offset
                      << ") - Evicting tag 0x" << std::hex << set.lines[evict_line].tag
                      << std::dec << std::endl;


            if (config.write_policy == WRITE_BACK && set.lines[evict_line].dirty) {
                writebacks++;
                dirty_evictions++;
                last_access.was_dirty_eviction = true;
                std::cout << "WRITEBACK: Evicted dirty line written to memory" << std::endl;
            }


            set.lines[evict_line].valid = true;
            set.lines[evict_line].tag = tag;
            set.lines[evict_line].dirty = false;
            initializeBlockCounters(set, evict_line);


            for (size_t i = 0; i < set.lines[evict_line].data.size(); i++) {
                set.lines[evict_line].data[i] = rand() % 1000;
            }
        }

        return false;
    }
}

bool SetAssociativeCache::writeMemory(unsigned int address, int data) {
    total_accesses++;


    unsigned int tag = getTag(address);
    unsigned int set_index = getSetIndex(address);
    unsigned int offset = getOffset(address);


    last_access = LastAccess();
    last_access.set_index = set_index;
    last_access.was_write_operation = true;


    CacheSet& set = cache_sets[set_index];


    int hit_line = set.findLine(tag);

    if (hit_line != -1) {

        cache_hits++;
        last_access.was_hit = true;
        last_access.line_index = hit_line;


        if (offset < set.lines[hit_line].data.size()) {
            set.lines[hit_line].data[offset] = data;
        }


        if (config.write_policy == WRITE_THROUGH) {

            std::cout << "WRITE HIT (Write-Through): Address 0x" << std::hex << address
                      << " (Tag: 0x" << tag << ", Set: " << std::dec << set_index
                      << ", Way: " << hit_line << ") - Writing to cache and memory" << std::endl;
        } else {

            set.lines[hit_line].dirty = true;
            std::cout << "WRITE HIT (Write-Back): Address 0x" << std::hex << address
                      << " (Tag: 0x" << tag << ", Set: " << std::dec << set_index
                      << ", Way: " << hit_line << ") - Writing to cache, marking dirty" << std::endl;
        }


        updateReplacementCounters(set, hit_line);

        return true;
    } else {

        cache_misses++;
        last_access.was_hit = false;


        if (config.write_miss_policy == NO_WRITE_ALLOCATE) {

            std::cout << "WRITE MISS (No-Write-Allocate): Address 0x" << std::hex << address
                      << " (Tag: 0x" << tag << ", Set: " << std::dec << set_index
                      << ") - Writing directly to memory" << std::endl;
            return false;
        } else {

            std::cout << "WRITE MISS (Write-Allocate): Address 0x" << std::hex << address
                      << " (Tag: 0x" << tag << ", Set: " << std::dec << set_index
                      << ") - Loading block into cache" << std::endl;


            int empty_line = set.findEmptyLine();

            if (empty_line != -1) {

                compulsory_misses++;
                last_access.was_compulsory_miss = true;
                last_access.line_index = empty_line;


                set.lines[empty_line].valid = true;
                set.lines[empty_line].tag = tag;
                set.lines[empty_line].dirty = false;
                initializeBlockCounters(set, empty_line);


                for (size_t i = 0; i < set.lines[empty_line].data.size(); i++) {
                    set.lines[empty_line].data[i] = rand() % 1000;
                }


                if (offset < set.lines[empty_line].data.size()) {
                    set.lines[empty_line].data[offset] = data;
                }


                if (config.write_policy == WRITE_THROUGH) {
                    std::cout << "Write-Through: Also writing to memory" << std::endl;
                } else {
                    set.lines[empty_line].dirty = true;
                    std::cout << "Write-Back: Marking cache line dirty" << std::endl;
                }

            } else {

                conflict_misses++;
                last_access.was_compulsory_miss = false;

                int evict_line = findEvictionLine(set);
                last_access.line_index = evict_line;
                last_access.had_eviction = true;
                last_access.evicted_line_index = evict_line;
                last_access.evicted_tag = set.lines[evict_line].tag;


                if (config.write_policy == WRITE_BACK && set.lines[evict_line].dirty) {
                    writebacks++;
                    dirty_evictions++;
                    last_access.was_dirty_eviction = true;
                    std::cout << "WRITEBACK: Evicted dirty line written to memory" << std::endl;
                }


                set.lines[evict_line].valid = true;
                set.lines[evict_line].tag = tag;
                set.lines[evict_line].dirty = false;
                initializeBlockCounters(set, evict_line);


                for (size_t i = 0; i < set.lines[evict_line].data.size(); i++) {
                    set.lines[evict_line].data[i] = rand() % 1000;
                }


                if (offset < set.lines[evict_line].data.size()) {
                    set.lines[evict_line].data[offset] = data;
                }


                if (config.write_policy == WRITE_THROUGH) {
                    std::cout << "Write-Through: Also writing to memory" << std::endl;
                } else {
                    set.lines[evict_line].dirty = true;
                    std::cout << "Write-Back: Marking cache line dirty" << std::endl;
                }
            }

            return false;
        }
    }
}

unsigned int SetAssociativeCache::getTag(unsigned int address) {
    return address >> (config.index_bits + config.offset_bits);
}

unsigned int SetAssociativeCache::getSetIndex(unsigned int address) {
    unsigned int mask = (1 << config.index_bits) - 1;
    return (address >> config.offset_bits) & mask;
}

unsigned int SetAssociativeCache::getOffset(unsigned int address) {
    unsigned int mask = (1 << config.offset_bits) - 1;
    return address & mask;
}

void SetAssociativeCache::reset() {
    for (auto& set : cache_sets) {
        for (auto& line : set.lines) {
            line.valid = false;
            line.tag = 0;
            line.dirty = false;
            line.lru_counter = 0;
            line.fifo_timestamp = 0;
            std::fill(line.data.begin(), line.data.end(), 0);
        }
    }

    global_lru_counter = 1;
    global_fifo_timestamp = 1;
    total_accesses = 0;
    cache_hits = 0;
    cache_misses = 0;
    compulsory_misses = 0;
    conflict_misses = 0;
    writebacks = 0;
    dirty_evictions = 0;

    last_access = LastAccess();

    std::cout << "Cache reset successfully." << std::endl;
}

void SetAssociativeCache::displayCache() const {
    std::cout << "\n=== CACHE STATE ===" << std::endl;
    std::cout << "Cache Size: " << config.cache_size << " bytes" << std::endl;
    std::cout << "Block Size: " << config.block_size << " bytes" << std::endl;
    std::cout << "Associativity: " << config.associativity << "-way" << std::endl;
    std::cout << "Number of Sets: " << config.num_sets << std::endl;
    std::cout << "Address bits: " << config.address_bits
              << " (Tag: " << config.tag_bits
              << ", Index: " << config.index_bits
              << ", Offset: " << config.offset_bits << ")" << std::endl;

    std::cout << "\nStatistics:" << std::endl;
    std::cout << "Total Accesses: " << total_accesses << std::endl;
    std::cout << "Cache Hits: " << cache_hits << std::endl;
    std::cout << "Cache Misses: " << cache_misses << std::endl;
    std::cout << "  - Compulsory Misses: " << compulsory_misses << std::endl;
    std::cout << "  - Conflict Misses: " << conflict_misses << std::endl;
    std::cout << "Hit Rate: " << std::fixed << std::setprecision(2)
              << (getHitRate() * 100) << "%" << std::endl;
    std::cout << "Writebacks: " << writebacks << std::endl;
    std::cout << "Dirty Evictions: " << dirty_evictions << std::endl;

    displayCacheDetailed();
}

void SetAssociativeCache::displayCacheDetailed() const {
    std::cout << "\nCache Contents:" << std::endl;

    for (int set_idx = 0; set_idx < config.num_sets; set_idx++) {
        std::cout << "Set " << set_idx << ":" << std::endl;
        std::cout << "  Way | Valid | Dirty | Tag      | LRU | Data (first 4 words)" << std::endl;
        std::cout << "  ----|-------|-------|----------|-----|-------------------" << std::endl;

        const CacheSet& set = cache_sets[set_idx];
        for (int way = 0; way < config.associativity; way++) {
            const AssociativeCacheLine& line = set.lines[way];

            std::cout << "  " << std::setw(3) << way << " | ";
            std::cout << std::setw(5) << (line.valid ? "1" : "0") << " | ";
            std::cout << std::setw(5) << (line.dirty ? "1" : "0") << " | ";

            if (line.valid) {
                std::cout << "0x" << std::hex << std::setw(6) << std::setfill('0')
                          << line.tag << std::dec << " | ";
                std::cout << std::setw(3) << line.lru_counter << " | ";
                for (int j = 0; j < std::min(4, (int)line.data.size()); j++) {
                    std::cout << std::setw(4) << line.data[j] << " ";
                }
            } else {
                std::cout << "  ----   |  -- |  -- | ---- ---- ---- ----";
            }
            std::cout << std::endl;
            std::cout << std::setfill(' ');
        }
        std::cout << std::endl;
    }
}


int SetAssociativeCache::findEvictionLine(CacheSet& set) {
    switch(config.replacement_policy) {
        case LRU:
            return set.findLRULine();
        case FIFO:
            return set.findFIFOLine();
        case RANDOM:
            return set.findRandomLine();
        default:
            return set.findLRULine();
    }
}


void SetAssociativeCache::updateReplacementCounters(CacheSet& set, int line_index) {
    switch(config.replacement_policy) {
        case LRU:
            set.lines[line_index].updateLRU(global_lru_counter++);
            break;
        case FIFO:

            break;
        case RANDOM:

            break;
    }
}


void SetAssociativeCache::initializeBlockCounters(CacheSet& set, int line_index) {
    switch(config.replacement_policy) {
        case LRU:
            set.lines[line_index].updateLRU(global_lru_counter++);
            break;
        case FIFO:
            set.lines[line_index].updateFIFO(global_fifo_timestamp++);
            break;
        case RANDOM:

            break;
    }
}


std::vector<TraceEntry> SetAssociativeCache::loadTraceFile(const std::string& filename) {
    std::vector<TraceEntry> trace;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open trace file: " << filename << std::endl;
        return trace;
    }

    std::string line;
    while (std::getline(file, line)) {

        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string operation;
        std::string address_str;

        if (!(iss >> operation >> address_str)) {
            continue;
        }


        unsigned int address;
        try {
            if (address_str.substr(0, 2) == "0x" || address_str.substr(0, 2) == "0X") {
                address = std::stoul(address_str, nullptr, 16);
            } else {
                address = std::stoul(address_str, nullptr, 10);
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not parse address: " << address_str << std::endl;
            continue;
        }


        AccessType type = (operation == "R" || operation == "r" || operation == "READ") ? READ : WRITE;


        int data = 0;
        if (type == WRITE) {
            std::string data_str;
            if (iss >> data_str) {
                try {
                    data = std::stoi(data_str);
                } catch (const std::exception& e) {
                    data = 0;
                }
            }
        }

        trace.emplace_back(type, address, data);
    }

    file.close();
    std::cout << "Loaded " << trace.size() << " trace entries from " << filename << std::endl;
    return trace;
}


TraceResults SetAssociativeCache::processTraceFile(const std::string& filename) {
    std::vector<TraceEntry> trace = loadTraceFile(filename);
    return processTrace(trace);
}


TraceResults SetAssociativeCache::processTrace(const std::vector<TraceEntry>& trace) {

    reset();

    TraceResults results;

    for (const auto& entry : trace) {
        bool hit;
        if (entry.type == READ) {
            hit = accessMemory(entry.address);
            results.reads++;
        } else {
            hit = writeMemory(entry.address, entry.data);
            results.writes++;
        }

        if (hit) {
            results.hits++;
        } else {
            results.misses++;
        }
    }


    results.total_accesses = total_accesses;
    results.writebacks = writebacks;
    results.dirty_evictions = dirty_evictions;
    results.hit_rate = getHitRate();
    results.miss_rate = 1.0 - results.hit_rate;
    results.replacement_policy = getReplacementPolicyString();
    results.write_policy = getWritePolicyString();
    results.write_miss_policy = getWriteMissPolicyString();

    return results;
}
