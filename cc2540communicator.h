#ifndef CC2540COMMUNICATOR_H
#define CC2540COMMUNICATOR_H

#include "serialcommunicator.h"
#include <vector>

#define TX_TYPE_COMMAND     0x01
#define RX_TYPE_EVENT       0x04
#define RX_HCI_LE_EXTEVENT  0xFF

#define CC2540_DEBUGMODE

enum TxOpcode
{
    GAP_DeviceInit                      = 0xFE00,
    GAP_MakeDiscoverable                = 0xFE06,
    GAP_UpdateAdvertisingData           = 0xFE07,
    GAP_GetParam                        = 0xFE31
};

enum RxEvent
{
    GAP_HCI_ExtentionCommandStatus      = 0x067F,
    GAP_DeviceInitDone                  = 0x0600
};

class CC2540Communicator : public SerialCommunicator
{
private:
    unsigned char   _device_irk[16];
    unsigned char   _device_csrk[16];

    unsigned char   _device_mac_reversed[6];

    void            rxInterpretDeviceInit(std::vector<unsigned char> data);

public:
    CC2540Communicator();

    size_t          sendCommand(TxOpcode opcode, std::vector<unsigned char> dataparams);

    unsigned char   rxPacket();

    size_t          txInitCommand();
};

#endif // CC2540COMMUNICATOR_H
