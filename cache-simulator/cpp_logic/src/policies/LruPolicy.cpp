#include "simulator/policies/LruPolicy.h"
#include <algorithm>
#include <climits>

void LruPolicy::onAccess(int set_index, int way, char ) {
    ensureSetExists(set_index, lru_counters[set_index].size());
    lru_counters[set_index][way] = ++global_counter;
}

void LruPolicy::onInsertion(int set_index, int way) {
    ensureSetExists(set_index, lru_counters[set_index].size());
    lru_counters[set_index][way] = ++global_counter;
}

int LruPolicy::findVictim(int set_index, int associativity) {
    ensureSetExists(set_index, associativity);

    int victim_way = 0;
    int min_counter = lru_counters[set_index][0];

    for (int way = 1; way < associativity; way++) {
        if (lru_counters[set_index][way] < min_counter) {
            min_counter = lru_counters[set_index][way];
            victim_way = way;
        }
    }

    return victim_way;
}

void LruPolicy::reset(int set_index, int associativity) {
    lru_counters[set_index] = std::vector<int>(associativity, 0);
}

void LruPolicy::ensureSetExists(int set_index, int associativity) {
    if (lru_counters.find(set_index) == lru_counters.end()) {
        lru_counters[set_index] = std::vector<int>(associativity, 0);
    }
}
