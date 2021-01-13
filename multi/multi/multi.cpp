#include <iostream>
#include <any>
#include <variant>
#include "multi.h"
#include <thread>


int main()
{
    std::bitset<32> b;
    b = multi::GetAffinityMask();
    std::cout << b << " | " << multi::GetAffinityCore() << std::endl;
    multi::SetAffinityCore(10);
    b = multi::GetAffinityMask();
    std::cout << b << " | " << multi::GetAffinityCore() << std::endl;

    multi::runner main(true);
    auto test = main.newTLoop<multi::milliseconds>(1000,[](multi::mobj* m) {
        printf("Looping... %i\n",multi::isMainThread());
        return true;
    });
    main.loop(); // This is blocking and will keep looping until stopped! The exception to this is if you tell it to run in another thread!
    // If another thread is pushed you must hold the main thread still!
    while (true);
}
