#ifndef IREPLACEMENTPOLICY_H
#define IREPLACEMENTPOLICY_H



class IReplacementPolicy {
public:
    virtual ~IReplacementPolicy() = default;



    virtual void onAccess(int set_index, int way, char access_type) = 0;



    virtual void onInsertion(int set_index, int way) = 0;



    virtual int findVictim(int set_index, int associativity) = 0;



    virtual void reset(int set_index, int associativity) = 0;



    virtual const char* getName() const = 0;
};

#endif
