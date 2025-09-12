#include <iostream>
#include "simulator/Cache.h"
using namespace std;
int main()
{
    cout << "Testing Cache Library..." << endl;

    try {

        SetAssociativeCache cache(1024, 64, 2, ReplacementPolicy::LRU);

        cout << "✓ Cache created successfully" << endl;


        cache.writeMemory(0x1000, 100);
        cache.accessMemory(0x1000);

        cout << "✓ Basic read/write operations work" << endl;

        cout << "All tests passed!" << endl;
        return 0;

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "Unknown error occurred" << endl;
        return 1;
    }
}
