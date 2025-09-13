#include "simulator/Cache.h"
#include "simulator/CacheHierarchy.h"
#include "simulator/policies/LruPolicy.h"
#include <string>
#include <sstream>
#include <cstring>
#include <memory>
using namespace std;

struct CacheSimulator {
    unique_ptr<SetAssociativeCache> cache;
    unique_ptr<IReplacementPolicy> policy;


    int cache_size;
    int block_size;
    int associativity;
    ReplacementPolicy policy_type;


    int total_accesses;
    int hits;
    int misses;
    int writebacks;

    CacheSimulator() : total_accesses(0), hits(0), misses(0), writebacks(0) {}
};


static char result_buffer[8192];  // Increased buffer size for cache state

extern "C" {
    __attribute__((visibility("default"))) CacheSimulator* create_simulator() {
        return new CacheSimulator();
    }
    __attribute__((visibility("default"))) int configure_cache(
        CacheSimulator* sim,
        int cache_size,
        int block_size,
        int associativity,
        int policy_type
    ) {
        if (!sim) return 0;
        try {
            sim->cache_size = cache_size;
            sim->block_size = block_size;
            sim->associativity = associativity;
            sim->policy_type = static_cast<ReplacementPolicy>(policy_type);


            sim->cache = make_unique<SetAssociativeCache>(
                cache_size, block_size, associativity, sim->policy_type
            );


            sim->total_accesses = 0;
            sim->hits = 0;
            sim->misses = 0;
            sim->writebacks = 0;

            return 1;
        } catch (...) {
            return 0;
        }
    }
    __attribute__((visibility("default"))) const char* process_access(
        CacheSimulator* sim,
        unsigned int address,
        char operation,
        int data_value
    ) {
        if (!sim || !sim->cache) {
            strcpy(result_buffer, "{\"error\": \"Simulator not configured\"}");
            return result_buffer;
        }

        try {
            sim->total_accesses++;


            bool is_hit = false;

            if (operation == 'R') {
                bool result = sim->cache->accessMemory(address);
                is_hit = result;
            } else if (operation == 'W') {
                bool result = sim->cache->writeMemory(address, data_value);
                is_hit = result;
            }


            if (is_hit) {
                sim->hits++;
            } else {
                sim->misses++;
            }


            int block_address = address / sim->block_size;
            int set_index = block_address % (sim->cache_size / (sim->block_size * sim->associativity));
            int tag = block_address / (sim->cache_size / (sim->block_size * sim->associativity));


            ostringstream json;
            json << "{"
                 << "\"address\": \"0x" << hex << address << "\","
                 << "\"operation\": \"" << operation << "\","
                 << "\"result\": \"" << (is_hit ? "HIT" : "MISS") << "\","
                 << "\"set_index\": " << dec << set_index << ","
                 << "\"tag\": \"0x" << hex << tag << "\","
                 << "\"total_accesses\": " << dec << sim->total_accesses << ","
                 << "\"hits\": " << sim->hits << ","
                 << "\"misses\": " << sim->misses << ","
                 << "\"hit_rate\": " << (sim->total_accesses > 0 ? (sim->hits * 100.0 / sim->total_accesses) : 0.0) << ","
                 << "\"writebacks\": " << sim->writebacks
                 << "}";

            string result_str = json.str();
            strncpy(result_buffer, result_str.c_str(), sizeof(result_buffer) - 1);
            result_buffer[sizeof(result_buffer) - 1] = '\0';

            return result_buffer;

        } catch (...) {
            strcpy(result_buffer, "{\"error\": \"Access processing failed\"}");
            return result_buffer;
        }
    }



    __attribute__((visibility("default"))) const char* get_statistics(CacheSimulator* sim) {
        if (!sim) {
            strcpy(result_buffer, "{\"error\": \"Invalid simulator\"}");
            return result_buffer;
        }

        ostringstream json;
        json << "{"
             << "\"total_accesses\": " << sim->total_accesses << ","
             << "\"hits\": " << sim->hits << ","
             << "\"misses\": " << sim->misses << ","
             << "\"hit_rate\": " << (sim->total_accesses > 0 ? (sim->hits * 100.0 / sim->total_accesses) : 0.0) << ","
             << "\"miss_rate\": " << (sim->total_accesses > 0 ? (sim->misses * 100.0 / sim->total_accesses) : 0.0) << ","
             << "\"writebacks\": " << sim->writebacks << ","
             << "\"cache_size\": " << sim->cache_size << ","
             << "\"block_size\": " << sim->block_size << ","
             << "\"associativity\": " << sim->associativity << ","
             << "\"policy\": " << static_cast<int>(sim->policy_type)
             << "}";

        string result_str = json.str();
        strncpy(result_buffer, result_str.c_str(), sizeof(result_buffer) - 1);
        result_buffer[sizeof(result_buffer) - 1] = '\0';

        return result_buffer;
    }



    __attribute__((visibility("default"))) void reset_simulator(CacheSimulator* sim) {
        if (sim && sim->cache) {
            sim->total_accesses = 0;
            sim->hits = 0;
            sim->misses = 0;
            sim->writebacks = 0;

        }
    }



    __attribute__((visibility("default"))) const char* process_trace_file(
        CacheSimulator* sim,
        const char* filename
    ) {
        if (!sim || !sim->cache || !filename) {
            strcpy(result_buffer, "{\"error\": \"Invalid parameters\"}");
            return result_buffer;
        }

        try {
            TraceResults results = sim->cache->processTraceFile(filename);


            sim->total_accesses = results.total_accesses;
            sim->hits = results.hits;
            sim->misses = results.misses;
            sim->writebacks = results.writebacks;


            ostringstream json;
            json << "{"
                 << "\"success\": true,"
                 << "\"total_accesses\": " << results.total_accesses << ","
                 << "\"reads\": " << results.reads << ","
                 << "\"writes\": " << results.writes << ","
                 << "\"hits\": " << results.hits << ","
                 << "\"misses\": " << results.misses << ","
                 << "\"hit_rate\": " << results.hit_rate << ","
                 << "\"writebacks\": " << results.writebacks << ","
                 << "\"dirty_evictions\": " << results.dirty_evictions
                 << "}";

            string result_str = json.str();
            strncpy(result_buffer, result_str.c_str(), sizeof(result_buffer) - 1);
            result_buffer[sizeof(result_buffer) - 1] = '\0';

            return result_buffer;

        } catch (...) {
            strcpy(result_buffer, "{\"error\": \"Trace file processing failed\"}");
            return result_buffer;
        }
    }



    __attribute__((visibility("default"))) const char* get_cache_state(CacheSimulator* sim) {
        if (!sim || !sim->cache) {
            strcpy(result_buffer, "{\"error\": \"Invalid simulator\"}");
            return result_buffer;
        }

        try {
            ostringstream json;
            json << "{\"sets\": {";

            const auto& cache_sets = sim->cache->getCacheSets();
            int num_sets = cache_sets.size();

            for (int set = 0; set < num_sets; set++) {
                if (set > 0) json << ",";
                json << "\"" << set << "\": {\"ways\": {";

                const auto& cache_set = cache_sets[set];
                for (int way = 0; way < sim->associativity; way++) {
                    if (way > 0) json << ",";
                    
                    const auto& line = cache_set.lines[way];
                    json << "\"" << way << "\": {"
                         << "\"valid\": " << (line.valid ? "true" : "false") << ","
                         << "\"tag\": ";
                    
                    if (line.valid) {
                        json << "\"0x" << hex << line.tag << dec << "\"";
                    } else {
                        json << "null";
                    }
                    
                    json << ","
                         << "\"dirty\": " << (line.dirty ? "true" : "false") << ","
                         << "\"lru_counter\": " << line.lru_counter << ","
                         << "\"data\": ";
                    
                    if (line.valid && line.data.size() > 0) {
                        json << "\"Data_" << hex << line.tag << dec << "\"";
                    } else {
                        json << "null";
                    }
                    
                    json << "}";
                }

                json << "}}";
            }

            json << "}}";

            string result_str = json.str();
            strncpy(result_buffer, result_str.c_str(), sizeof(result_buffer) - 1);
            result_buffer[sizeof(result_buffer) - 1] = '\0';

            return result_buffer;

        } catch (...) {
            strcpy(result_buffer, "{\"error\": \"Cache state retrieval failed\"}");
            return result_buffer;
        }
    }



    __attribute__((visibility("default"))) void destroy_simulator(CacheSimulator* sim) {
        delete sim;
    }
}
