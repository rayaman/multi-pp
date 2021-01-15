#include <iostream>
#include "multi.h"
#include <stdio.h>
#include <assert.h>
#include <bitset>
#include <stdexcept>

// Part of the tests
void do_tests() {
    using namespace std;
    bitset<32> old = multi::GetAffinityMask();
    bitset<32> b;
    auto pcc = multi::GetPhysicalCoreCount();
    assert (pcc == 12);
    std::cout << "PhysicalCoreCount: " << pcc << endl;
    std::cout << "LogicalCoreCount: " << multi::GetLogicalCoreCount() << endl;

    auto success = multi::SetAffinityCore(1);
    assert(success);
    std::cout << "Affinity has been set!" << endl;

    b = multi::GetAffinityMask();
    std::cout << b << " Expected: 0000...0010" << endl;

    std::cout << "Affinity Core is set to: " << multi::GetAffinityCore() << " Expected: 1" << endl;

    success = multi::SetAffinityMask(bitset<32>("1001001").to_ullong());

    b = multi::GetAffinityMask();
    std::cout << b << " Expected: 0000...1001001" << endl;

    std::cout << "Affinity Core is set to: " << multi::GetAffinityCore() << " Expected: -1" << endl;

    multi::SetAffinityMask(old.to_ulong());
    std::cout << "Affinity Mask Restored: " << old << endl;
    // Time to test thread handles

    multi::tHandle *hand = new multi::tHandle;
    int* count = new int(0);
    std::thread* test = new std::thread([](multi::tHandle ** hand,int* c) {
        //_set_abort_behavior(0, _WRITE_ABORT_MSG);
        bitset<32> mask;
        bitset<32> comp = bitset<32>("10");
        std::cout << "Setting Handle!" << endl;
        (*hand)->setHandle(multi::GetThreadHandle(true));
        std::cout << "Handle set! " << (*(*hand)).handle << endl;
        std::cout << "Inside Another thread!" << endl;
        while (true) {
            this_thread::sleep_for(250ms);
            std::cout << "Current Mask: " << (mask = multi::GetAffinityMask()) << endl;
            std::cout << "Expected Mask: " << comp << endl;
            if (mask == comp)
                break;
        }
        *c = -1;
    },&hand,count);
    while (!hand) {} // Wait until this handle is set
    this_thread::sleep_for(100ms);
    std::cout << "We have a handle to the thread! " << hand->handle << " | " << test->native_handle() << endl;
    multi::SetAffinityCore(1,hand);
    while (*count!=-1) {}
    delete count;
    delete hand;
    std::cout << "All Tests Finished!" << endl;
    this_thread::sleep_for(1000ms);
}
int main()
{
    
    do_tests();
    std::cout << "hm,m" << std::endl;
    exit(0);
    //std::cout << multi::GetPhysicalCoreCount() << std::endl;
    //std::bitset<32> b;
    //b = multi::GetAffinityMask();
    //std::cout << b << " | " << multi::GetAffinityCore() << std::endl;
    //multi::SetAffinityCore(10);
    //b = multi::GetAffinityMask();
    //std::cout << b << " | " << multi::GetAffinityCore() << std::endl;

    //multi::runner main(true);
    //auto test = main.newTLoop<multi::milliseconds>(1000,[](multi::mobj* m) {
    //    printf("Looping... %i\n",multi::isMainThread());
    //    return true;
    //});
    //main.loop(); // This is blocking and will keep looping until stopped! The exception to this is if you tell it to run in another thread!
    //// If another thread is pushed you must hold the main thread still!
    //while (true);


}
