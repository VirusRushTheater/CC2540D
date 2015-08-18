#include <iostream>
#include <vector>
#include <unistd.h>

#include "serialcommunicator.h"
#include "cc2540communicator.h"

using namespace std;

CC2540Communicator  sc;

int main()
{
    try
    {
        sc.init(0x0451, 0x16AA, 1, 0x84, 0x04);
        sc.txInitCommand();
        sc.txDeviceDiscovery();
    }
    catch(std::string p)
    {
        sc.printLastError();
        return 1;
    }

    return 0;
}

