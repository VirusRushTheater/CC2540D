#ifndef CC2540COMMUNICATOR_H
#define CC2540COMMUNICATOR_H

#include "serialcommunicator.h"

#include <vector>
#include <cstdio>

#define TX_TYPE_COMMAND     0x01
#define RX_TYPE_EVENT       0x04
#define RX_HCI_LE_EXTEVENT  0xFF

#define CC2540_DEBUGMODE

/**
 * Error codes returned by rx_ and tx_ functions. Negative values are internal errors, and Positive values are errors given by the device.
 */
enum TxErrors
{
    Tx_Success              = 0,
    Tx_TxUnsuccessful       = -1,
    Tx_RxMalformed          = -2,
    Tx_RxTooShort           = -3
};

/**
 * Transmission Opcodes.
 */
enum TxOpcode
{
    GAP_DeviceInit                      = 0xFE00,
    GAP_ConfigureDeviceAddress          = 0xFE03,
    GAP_DeviceDiscoveryRequest          = 0xFE04,
    GAP_MakeDiscoverable                = 0xFE06,
    GAP_UpdateAdvertisingData           = 0xFE07,
    GAP_EndDiscoverable                 = 0xFE08,
    GAP_EstablishLinkRequest            = 0xFE09,
    GAP_TerminateLinkRequest            = 0xFE0A,
    GAP_GetParam                        = 0xFE31
};

/**
 * Received event codes.
 */
enum RxEvent
{
    GAP_DeviceInitDone                  = 0x0600,
    GAP_DeviceDiscoveryDone             = 0x0601,
    GAP_EstablishLink                   = 0x0605,
    GAP_LinkParamUpdate                 = 0x0607,
    GAP_DeviceInformation               = 0x060D,
    GAP_HCI_ExtentionCommandStatus      = 0x067F
};

struct MacAddress
{
    unsigned char addr[6];
};

class CC2540Communicator : public SerialCommunicator
{
private:
    unsigned char                   _device_irk[16];
    unsigned char                   _device_csrk[16];

    unsigned char                   _device_mac_reversed[6];

    std::vector<MacAddress>         _discovered_devices;

    void            rxInterpretDeviceInit(std::vector<unsigned char> data);
    void            rxInterpretDeviceInformation(std::vector<unsigned char> data);

public:
    CC2540Communicator();

    /**
     * Low level function to send a packet. Its more intended for internal use, or if there is a function not programmed in this class yet.
     */
    size_t          txSendCommand(TxOpcode opcode, std::vector<unsigned char> dataparams);

    /**
     * Low level function to receive a packet. If supplied, packet data will be stored in the std::vector argument variable.
     */
    int             rxPacket();
    int             rxPacket(std::vector<unsigned char>* rxdata);

    /**
     * High level function to signal the CC2540 to start operating.
     */
    int             txInitCommand();

    /**
     * Searches discoverable BLE devices for a while and stores their addresses internally. To view them, use getDiscoveredDevices().
     */
    std::vector<MacAddress> txDeviceDiscovery();

    /**
     * Establishes a communication Link with a remote device.
     */
    int             txEstablishLink(MacAddress remoteDevice);
    int             txTerminateLink();

    /**
     * Gets the addresses of the BLE devices discovered previously with txDeviceDiscovery().
     */
    std::vector<MacAddress> getDiscoveredDevices() const;
};

#endif // CC2540COMMUNICATOR_H
