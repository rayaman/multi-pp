#include <windows.h>
#include <iostream>
#include <any>
#include <variant>
#include "multi.h"
#include <thread>
#include <bitset>

#ifdef UNIX 

#endif
#ifdef WIN

#endif

int main()
{
    //multi::runner main(true);
    //auto test = main.newTLoop<multi::milliseconds>(1000,[](multi::mobj* m) {
    //    printf("Looping... %i\n",multi::isMainThread());
    //    return true;
    //});
    //main.loop(); // This is blocking and will keep looping until stopped! The exception to this is if you tell it to run in another thread!
    //// If another thread is pushed you must hold the main thread still!
    //while (true);
    unsigned int core = 2;
    unsigned int cores = std::thread::hardware_concurrency();
    auto mask = (static_cast<DWORD_PTR>(1) << core);
    std::bitset<32> val(mask);
    std::cout << val << std::endl;
    //SetThreadAffinityMask(GetCurrentThread(), 1);
}
