#ifndef CC2540COMMUNICATOR_H
#define CC2540COMMUNICATOR_H

#include "serialcommunicator.h"
#include <vector>

enum TxOpcode
{
    GAP_DeviceInit      = 0xFE00
};

class CC2540Communicator : public SerialCommunicator
{
public:
    CC2540Communicator();

    size_t sendCommand(TxOpcode opcode, std::vector<unsigned char> dataparams);

    size_t txInitCommand();
};

#endif // CC2540COMMUNICATOR_H
