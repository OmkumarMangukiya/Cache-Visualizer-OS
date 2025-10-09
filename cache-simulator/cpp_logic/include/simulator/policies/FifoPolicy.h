#ifndef FIFOPOLICY_H
#define FIFOPOLICY_H
using namespace std;
#include "IReplacementPolicy.h"
#include <vector>
#include <map>

class FifoPolicy : public IReplacementPolicy {
private:
    map<int, vector<int>> fifo_timestamps;
    int global_counter;

public:
    FifoPolicy() : global_counter(0) {}

    void onAccess(int set_index, int way, char access_type) override;
    void onInsertion(int set_index, int way) override;
    int findVictim(int set_index, int associativity) override;
    void reset(int set_index, int associativity) override;
    const char* getName() const override { return "FIFO"; }

private:
    void ensureSetExists(int set_index, int associativity);
};

#endif