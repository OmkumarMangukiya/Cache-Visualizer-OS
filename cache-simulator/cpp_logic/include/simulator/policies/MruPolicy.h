#pragma once
#include "IReplacementPolicy.h"
#include <vector>
using namespace std;

class MruPolicy : public IReplacementPolicy {
public:
    MruPolicy();
    virtual ~MruPolicy() = default;
    
    void onAccess(int set_index, int way, char access_type) override;
    void onInsertion(int set_index, int way) override;
    int findVictim(int set_index, int associativity) override;
    void reset(int set_index, int associativity) override;
    const char* getName() const override;

private:
    vector<vector<unsigned int>> accessTimes;  // [set_index][way]
    vector<unsigned int> currentTime;          // [set_index]
};