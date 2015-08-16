#include "cc2540communicator.h"
#include "binaryparameter.h"
#include <iostream>

CC2540Communicator::CC2540Communicator()
{
}

size_t CC2540Communicator::txSendCommand(TxOpcode opcode, std::vector<unsigned char> dataparams)
{
    unsigned char datalength = dataparams.size();
    std::vector<unsigned char> datasend;
    datasend = datasend << BinarySender<1>(TX_TYPE_COMMAND) <<                          // Type: Command:   0x01
                           BinarySender<2>(static_cast<unsigned short>(opcode)) <<      // 2-byte opcode.
                           BinarySender<1>(datalength);                                 // Message length in bytes.
    datasend.insert(datasend.end(), dataparams.begin(), dataparams.end());              // Message contents

    return send(datasend);
}

int CC2540Communicator::txInitCommand()
{
    /* BTool mimic:
    [1] : <Tx> - 04:52:52.802
    -Type       : 0x01 (Command)
    -Opcode     : 0xFE00 (GAP_DeviceInit)
    -Data Length    : 0x26 (38) byte(s)
     ProfileRole    : 0x04 (Peripheral)
     MaxScanRsps    : 0x05 (5)
     IRK        : 00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00
     CSRK       : 00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00
     SignCounter    : 0x00000001 (1)
     */


    #ifdef CC2540_DEBUGMODE
    std::cout << "Sent init packet." << std::endl;
    #endif

    size_t sendret;

    sendret = txSendCommand(GAP_DeviceInit, std::vector<unsigned char>() <<
                BinarySender<1>(0x08) <<        // Profile role:    0x08 (Central)
                BinarySender<1>(0x05) <<        // Max Scan Rsps:   0x05
                BinarySender<16>(0x00) <<       // IRK: 16 zeroes
                BinarySender<16>(0x00) <<       // CSRK: 16 zeroes
                BinarySender<4>(0x00000001)     // SignCounter: 0x00 00 00 01
    );

    // 0 bytes transferred
    if(sendret == 0)
    {
        setError("Could not transfer the Initialization packet.");
        return Tx_TxUnsuccessful;
    }

    // Waits for acknowledgement and retrieving information (2 Rx Packets)
    return rxPacket();
}

int CC2540Communicator::rxPacket()
{
    return rxPacket(NULL);
}

int CC2540Communicator::rxPacket(std::vector<unsigned char>* rxdata)
{
    /* BTool mimic:
    [2] : <Rx> - 04:52:52.878
    -Type       : 0x04 (Event)
    -EventCode  : 0xFF (HCI_LE_ExtEvent)
    -Data Length    : 0x06 (6) bytes(s)
     Event      : 0x067F (GAP_HCI_ExtentionCommandStatus)
     Status     : 0x00 (Success) -> This is going to be our return value.
     [What follows depends on the Event]
     */

    std::vector<unsigned char> recvpacket = recv();
    unsigned char   evtype, evcode, datalen, retval;
    unsigned short  eventno;
    RxEvent         eventlabel;

    if(recvpacket.size() < 6)
    {
        setError("Did not receive a message big enough to be successfully interpreted.");
        return Tx_RxTooShort;
    }

    recvpacket >> BinaryGrabber<1>(0, &evtype) >>
                  BinaryGrabber<1>(1, &evcode) >>
                  BinaryGrabber<1>(2, &datalen) >>
                  BinaryGrabber<2>(3, &eventno) >>
                  BinaryGrabber<1>(5, &retval);

    eventlabel = static_cast<RxEvent>(eventno);

    if(evtype != RX_TYPE_EVENT || evcode != RX_HCI_LE_EXTEVENT)
    {
        setError("Received a malformed packet.");
        return Tx_RxMalformed;
    }

    // There was an issue on receiving the package. It'll return an error code.
    if(retval != 0)
    {
        setError("Issue receiving the package.");
        return retval;
    }

    if(rxdata != NULL)
    {
        rxdata->clear();
        rxdata->reserve(recvpacket.size());
        rxdata->insert(rxdata->end(), recvpacket.begin(), recvpacket.end());
    }

    switch(eventlabel)
    {
        // Acknowledgement. Other receiving packet should follow.
        case GAP_HCI_ExtentionCommandStatus:
            #ifdef CC2540_DEBUGMODE
            std::cout << "GAP_HCI_ExtentionCommandStatus (Acknowledgement). Other receiving packet should follow." << std::endl;
            #endif
            return rxPacket(rxdata);
        break;

        // Init device done. We now know the hardware IDs of the USB dongle.
        case GAP_DeviceInitDone:
            #ifdef CC2540_DEBUGMODE
            std::cout << "GAP_DeviceInitDone. Hardware IDs are now known." << std::endl;
            #endif
            rxInterpretDeviceInit(recvpacket);
            return Tx_Success;
        break;
        default:
            #ifdef CC2540_DEBUGMODE
            std::cout << "Unknown event." << std::endl;
            #endif
            return Tx_Success;
        break;
    }

    return Tx_Success;
}

void CC2540Communicator::rxInterpretDeviceInit(std::vector<unsigned char> data)
{
    /* BTool mimic
    [14] : <Rx> - 08:30:57.356
    -Type       : 0x04 (Event)
    -EventCode  : 0xFF (HCI_LE_ExtEvent)
    -Data Length    : 0x2C (44) bytes(s)
     Event      : 0x0600 (GAP_DeviceInitDone)
     Status     : 0x00 (Success)
     DevAddr        : 00:18:30:EA:9C:1B
     DataPktLen : 0x001B (27)
     NumDataPkts    : 0x04 (4)
     IRK        : E2:7D:76:4B:9B:24:8D:EA:F8:A6:E3:82:16:C8:C5:0C
     CSRK       : EE:68:E6:8E:F6:48:43:04:C4:F0:5C:D3:D1:39:A9:9F
    Dump(Rx):
    04 FF 2C 00 06 00 1B 9C EA 30 18 00 1B 00 04 E2
    7D 76 4B 9B 24 8D EA F8 A6 E3 82 16 C8 C5 0C EE
    68 E6 8E F6 48 43 04 C4 F0 5C D3 D1 39 A9 9F
     */

    data >> BinaryGrabber<6>(6, _device_mac_reversed) >>
            BinaryGrabber<16>(15, _device_irk) >>
            BinaryGrabber<16>(31, _device_csrk);
}
