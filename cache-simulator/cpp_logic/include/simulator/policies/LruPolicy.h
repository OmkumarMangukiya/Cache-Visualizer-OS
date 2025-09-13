#ifndef LRUPOLICY_H
#define LRUPOLICY_H
using namespace std;
#include "IReplacementPolicy.h"
#include <vector>
#include <map>



class LruPolicy : public IReplacementPolicy {
private:
    map<int, vector<int>> lru_counters;
    int global_counter;

public:
    LruPolicy() : global_counter(0) {}

    void onAccess(int set_index, int way, char access_type) override;
    void onInsertion(int set_index, int way) override;
    int findVictim(int set_index, int associativity) override;
    void reset(int set_index, int associativity) override;
    const char* getName() const override { return "LRU"; }

private:
    void ensureSetExists(int set_index, int associativity);
};

#endif
