#include "simulator/Cache.h"
#include <cstdlib>
#include <ctime>
using namespace std;
SetAssociativeCache::SetAssociativeCache(int cache_size, int block_size, int associativity,
                                         ReplacementPolicy rp, WritePolicy wp, WriteMissPolicy wmp)
{

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

        cout << "CACHE HIT: Address 0x" << hex << address
                  << " (Tag: 0x" << tag << ", Set: " << dec << set_index
                  << ", Way: " << hit_line << ", Offset: " << offset << ")" << endl;

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

            cout << "COMPULSORY MISS: Address 0x" << hex << address
                      << " (Tag: 0x" << tag << ", Set: " << dec << set_index
                      << ", Way: " << empty_line << ", Offset: " << offset
                      << ") - Loading into empty line" << endl;
        } else {

            conflict_misses++;
            last_access.was_compulsory_miss = false;

            int evict_line = findEvictionLine(set);
            last_access.line_index = evict_line;
            last_access.had_eviction = true;
            last_access.evicted_line_index = evict_line;
            last_access.evicted_tag = set.lines[evict_line].tag;

            cout << "CONFLICT MISS: Address 0x" << hex << address
                      << " (Tag: 0x" << tag << ", Set: " << dec << set_index
                      << ", Way: " << evict_line << ", Offset: " << offset
                      << ") - Evicting tag 0x" << hex << set.lines[evict_line].tag
                      << dec << endl;


            if (config.write_policy == WRITE_BACK && set.lines[evict_line].dirty) {
                writebacks++;
                dirty_evictions++;
                last_access.was_dirty_eviction = true;
                cout << "WRITEBACK: Evicted dirty line written to memory" << endl;
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

            cout << "WRITE HIT (Write-Through): Address 0x" << hex << address
                      << " (Tag: 0x" << tag << ", Set: " << dec << set_index
                      << ", Way: " << hit_line << ") - Writing to cache and memory" << endl;
        } else {

            set.lines[hit_line].dirty = true;
            cout << "WRITE HIT (Write-Back): Address 0x" << hex << address
                      << " (Tag: 0x" << tag << ", Set: " << dec << set_index
                      << ", Way: " << hit_line << ") - Writing to cache, marking dirty" << endl;
        }


        updateReplacementCounters(set, hit_line);

        return true;
    } else {

        cache_misses++;
        last_access.was_hit = false;


        if (config.write_miss_policy == NO_WRITE_ALLOCATE) {

            cout << "WRITE MISS (No-Write-Allocate): Address 0x" << hex << address
                      << " (Tag: 0x" << tag << ", Set: " << dec << set_index
                      << ") - Writing directly to memory" << endl;
            return false;
        } else {

            cout << "WRITE MISS (Write-Allocate): Address 0x" << hex << address
                      << " (Tag: 0x" << tag << ", Set: " << dec << set_index
                      << ") - Loading block into cache" << endl;


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
                    cout << "Write-Through: Also writing to memory" << endl;
                } else {
                    set.lines[empty_line].dirty = true;
                    cout << "Write-Back: Marking cache line dirty" << endl;
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
                    cout << "WRITEBACK: Evicted dirty line written to memory" << endl;
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
                    cout << "Write-Through: Also writing to memory" << endl;
                } else {
                    set.lines[evict_line].dirty = true;
                    cout << "Write-Back: Marking cache line dirty" << endl;
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
            fill(line.data.begin(), line.data.end(), 0);
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

    cout << "Cache reset successfully." << endl;
}

void SetAssociativeCache::displayCache() const {
    cout << "\n=== CACHE STATE ===" << endl;
    cout << "Cache Size: " << config.cache_size << " bytes" << endl;
    cout << "Block Size: " << config.block_size << " bytes" << endl;
    cout << "Associativity: " << config.associativity << "-way" << endl;
    cout << "Number of Sets: " << config.num_sets << endl;
    cout << "Address bits: " << config.address_bits
              << " (Tag: " << config.tag_bits
              << ", Index: " << config.index_bits
              << ", Offset: " << config.offset_bits << ")" << endl;

    cout << "\nStatistics:" << endl;
    cout << "Total Accesses: " << total_accesses << endl;
    cout << "Cache Hits: " << cache_hits << endl;
    cout << "Cache Misses: " << cache_misses << endl;
    cout << "  - Compulsory Misses: " << compulsory_misses << endl;
    cout << "  - Conflict Misses: " << conflict_misses << endl;
    cout << "Hit Rate: " << fixed << setprecision(2)
              << (getHitRate() * 100) << "%" << endl;
    cout << "Writebacks: " << writebacks << endl;
    cout << "Dirty Evictions: " << dirty_evictions << endl;

    displayCacheDetailed();
}

void SetAssociativeCache::displayCacheDetailed() const {
    cout << "\nCache Contents:" << endl;

    for (int set_idx = 0; set_idx < config.num_sets; set_idx++) {
        cout << "Set " << set_idx << ":" << endl;
        cout << "  Way | Valid | Dirty | Tag      | LRU | Data (first 4 words)" << endl;
        cout << "  ----|-------|-------|----------|-----|-------------------" << endl;

        const CacheSet& set = cache_sets[set_idx];
        for (int way = 0; way < config.associativity; way++) {
            const AssociativeCacheLine& line = set.lines[way];

            cout << "  " << setw(3) << way << " | ";
            cout << setw(5) << (line.valid ? "1" : "0") << " | ";
            cout << setw(5) << (line.dirty ? "1" : "0") << " | ";

            if (line.valid) {
                cout << "0x" << hex << setw(6) << setfill('0')
                          << line.tag << dec << " | ";
                cout << setw(3) << line.lru_counter << " | ";
                for (int j = 0; j < min(4, (int)line.data.size()); j++) {
                    cout << setw(4) << line.data[j] << " ";
                }
            } else {
                cout << "  ----   |  -- |  -- | ---- ---- ---- ----";
            }
            cout << endl;
            cout << setfill(' ');
        }
        cout << endl;
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


vector<TraceEntry> SetAssociativeCache::loadTraceFile(const string& filename) {
    vector<TraceEntry> trace;
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Error: Could not open trace file: " << filename << endl;
        return trace;
    }

    string line;
    while (getline(file, line)) {

        if (line.empty() || line[0] == '#') {
            continue;
        }

        istringstream iss(line);
        string operation;
        string address_str;

        if (!(iss >> operation >> address_str)) {
            continue;
        }


        unsigned int address;
        try {
            if (address_str.substr(0, 2) == "0x" || address_str.substr(0, 2) == "0X") {
                address = stoul(address_str, nullptr, 16);
            } else {
                address = stoul(address_str, nullptr, 10);
            }
        } catch (const exception& e) {
            cerr << "Warning: Could not parse address: " << address_str << endl;
            continue;
        }


        AccessType type = (operation == "R" || operation == "r" || operation == "READ") ? READ : WRITE;


        int data = 0;
        if (type == WRITE) {
            string data_str;
            if (iss >> data_str) {
                try {
                    data = stoi(data_str);
                } catch (const exception& e) {
                    data = 0;
                }
            }
        }

        trace.emplace_back(type, address, data);
    }

    file.close();
    cout << "Loaded " << trace.size() << " trace entries from " << filename << endl;
    return trace;
}


TraceResults SetAssociativeCache::processTraceFile(const string& filename) {
    vector<TraceEntry> trace = loadTraceFile(filename);
    return processTrace(trace);
}


TraceResults SetAssociativeCache::processTrace(const vector<TraceEntry>& trace) {

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
