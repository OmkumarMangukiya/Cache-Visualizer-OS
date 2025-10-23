#include "simulator/policies/MruPolicy.h"
using namespace std;

MruPolicy::MruPolicy() {}

void MruPolicy::onAccess(int set_index, int way, char access_type) {
    // Ensure vectors are sized appropriately
    if (set_index >= accessTimes.size()) {
        accessTimes.resize(set_index + 1);
        currentTime.resize(set_index + 1, 0);
    }
    if (way >= accessTimes[set_index].size()) {
        accessTimes[set_index].resize(way + 1, 0);
    }

    // Update access time for the way
    currentTime[set_index]++;
    accessTimes[set_index][way] = currentTime[set_index];
}

void MruPolicy::onInsertion(int set_index, int way) {
    // Same as onAccess for MRU
    onAccess(set_index, way, 'W');
}

int MruPolicy::findVictim(int set_index, int associativity) {
    // Ensure vectors are sized appropriately
    if (set_index >= accessTimes.size()) {
        accessTimes.resize(set_index + 1);
        currentTime.resize(set_index + 1, 0);
    }
    if (associativity > accessTimes[set_index].size()) {
        accessTimes[set_index].resize(associativity, 0);
    }

    int victim = 0;
    unsigned int mostRecent = 0;

    // Find the most recently used way (highest access time)
    for (int way = 0; way < associativity; ++way) {
        if (accessTimes[set_index][way] > mostRecent) {
            mostRecent = accessTimes[set_index][way];
            victim = way;
        }
    }
    return victim;
}

void MruPolicy::reset(int set_index, int associativity) {
    if (set_index >= accessTimes.size()) {
        accessTimes.resize(set_index + 1);
        currentTime.resize(set_index + 1, 0);
    }
    accessTimes[set_index].assign(associativity, 0);
    currentTime[set_index] = 0;
}

const char* MruPolicy::getName() const {
    return "MRU";
}