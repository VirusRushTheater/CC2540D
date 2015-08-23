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

        std::vector<MacAddress> discovereddev;
        discovereddev = sc.getDiscoveredDevices();
        for(int i = 0; i < discovereddev.size(); i++)
        {
            std::cout << "MAC " << i << ": " << discovereddev[i].toString() << std::endl;
        }

        if(discovereddev.size() > 0)
        {
            //discovereddev[0].addr[5] = 0xFF;
            LinkInfo k = sc.txEstablishLink(discovereddev[0]);
            std::cout << k.dev_address.toString() << std::endl;
            sc.txTerminateLinkRequest(k);
        }
    }
    catch(std::string p)
    {
        sc.printLastError();
        return 1;
    }

    return 0;
}

