#include <iostream>
#include "multi.h"
#include <stdio.h>
#include <bitset>

// Part of the tests
void do_tests() {
    using namespace std;
    cout << "If the tests exit without seeing \"All Tests Finished!\" Something went wrong!" << endl;
    bitset<32> old = multi::GetAffinityMask();
    bitset<32> b;
    auto pcc = multi::GetPhysicalCoreCount();
    cout << "\nPhysicalCoreCount: " << pcc << endl;
    cout << "\nLogicalCoreCount: " << multi::GetLogicalCoreCount() << endl;

    auto success = multi::SetAffinityCore(1);

    cout << "\nAffinity has been set!" << endl;

    b = multi::GetAffinityMask();
    cout << b << "\nExpected: 0000...0010" << endl;

    cout << "\nAffinity Core is set to: " << multi::GetAffinityCore() << "\nExpected: 1\n" << endl;

    success = multi::SetAffinityMask(bitset<32>("1001001").to_ullong());

    b = multi::GetAffinityMask();
    cout << b << "\nExpected: 0000...1001001" << endl;

    cout << "\nAffinity Core is set to: " << multi::GetAffinityCore() << "\nExpected: -1" << endl;

    multi::SetAffinityMask(old.to_ulong());
    cout << "Affinity Mask Restored: " << old << "\n" << endl;
    // Time to test thread handles

    multi::tHandle *hand = new multi::tHandle;
    int* count = new int(0);
    thread* test = new thread([](multi::tHandle ** hand,int* c) {
        //_set_abort_behavior(0, _WRITE_ABORT_MSG);
        bitset<32> mask;
        bitset<32> comp = bitset<32>("10");
        cout << "Setting Handle!" << endl;
        (*hand)->setHandle(multi::GetThreadHandle(true));
        cout << "Handle set! " << (*(*hand)).handle << endl;
        cout << "Inside Another thread!" << endl;
        while (true) {
            this_thread::sleep_for(250ms);
            cout << "Current Mask: " << (mask = multi::GetAffinityMask()) << endl;
            cout << "Expected Mask: " << comp << endl;
            if (mask == comp) {
                *c = -1;
                break;
            }
        }
    },&hand,count);
    while (!hand) {} // Wait until this handle is set
    this_thread::sleep_for(100ms);
    cout << "We have a handle to the thread! " << hand->handle << " | " << test->native_handle() << endl;
    multi::SetAffinityCore(1,hand);
    while ((*count)!=-1) {
        this_thread::sleep_for(100ms);
    }
    cout << "All Tests Finished!" << endl;
}
int main()
{
    do_tests();

    /*multi::tStatus test{ multi::STATUS_ERRORED };
    std::cout << "SizeOf: " << sizeof(test) << std::endl;
    std::cout << "Status: " << (int)test.active << std::endl;*/
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
