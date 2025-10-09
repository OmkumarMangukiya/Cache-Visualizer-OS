#include "simulator/policies/FifoPolicy.h"
#include <algorithm>
#include <climits>
using namespace std;

void FifoPolicy::onAccess(int set_index, int way, char) {
    // In FIFO, we do not update timestamps on access
    ensureSetExists(set_index, fifo_timestamps[set_index].size());
}

void FifoPolicy::onInsertion(int set_index, int way) {
    ensureSetExists(set_index, fifo_timestamps[set_index].size());
    fifo_timestamps[set_index][way] = ++global_counter;
}

int FifoPolicy::findVictim(int set_index, int associativity) {
    ensureSetExists(set_index, associativity);

    int victim_way = 0;
    int min_timestamp = fifo_timestamps[set_index][0];

    for (int way = 1; way < associativity; way++) {
        if (fifo_timestamps[set_index][way] < min_timestamp) {
            min_timestamp = fifo_timestamps[set_index][way];
            victim_way = way;
        }
    }

    return victim_way;
}

void FifoPolicy::reset(int set_index, int associativity) {
    fifo_timestamps[set_index] = vector<int>(associativity, 0);
}

void FifoPolicy::ensureSetExists(int set_index, int associativity) {
    if (fifo_timestamps.find(set_index) == fifo_timestamps.end()) {
        fifo_timestamps[set_index] = vector<int>(associativity, 0);
    }
}