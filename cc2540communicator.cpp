#include "cc2540communicator.h"
#include "binaryparameter.h"

CC2540Communicator::CC2540Communicator()
{
}

size_t CC2540Communicator::sendCommand(TxOpcode opcode, std::vector<unsigned char> dataparams)
{
    unsigned char datalength = dataparams.size();
    std::vector<unsigned char> datasend;
    datasend = datasend << BinarySender<1>(0x01) <<
                           BinarySender<2>(static_cast<unsigned short>(opcode)) <<
                           BinarySender<1>(datalength);
    datasend.insert(datasend.end(), dataparams.begin(), dataparams.end());

    return send(datasend);
}

size_t CC2540Communicator::txInitCommand()
{
    return sendCommand(GAP_DeviceInit, std::vector<unsigned char>() <<
                BinarySender<1>(0x08) <<    // Profile role:    0x08 (Central)
                BinarySender<1>(0x05) <<    // Max Scan Rsps:   0x05
                BinarySender<16>(0x00) <<   // IRK: 16 zeroes
                BinarySender<16>(0x00) <<   // CSRK: 16 zeroes
                BinarySender<4>(0x00000001)   // SignCounter: 0x00 00 00 01
    );
}
