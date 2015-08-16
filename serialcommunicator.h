#ifndef SERIALCOMMUNICATOR_H
#define SERIALCOMMUNICATOR_H

#include <libusb.h>
#include <pthread.h>
#include <stdint.h>
#include <string>
#include <deque>
#include <vector>

/// An ugly way to implement the Pthreads, but... so be it.
enum CallbackType
{
    SenderThreadCB,
    ReceiverThreadCB,

    LibUSBReceiveCB
};

template<CallbackType>
static void* __callbackExternalMethod(void* castedSCParameter);

/**
 * Class intended for an IO-stream of USB serial data.
 */
class SerialCommunicator
{
template<CallbackType>
friend void* __callbackExternalMethod(void*);

private:
    libusb_context*         _usbctx;
    libusb_device*          _usbdev;
    libusb_device_handle*   _usbhandle;

    uint16_t                _vendor;
    uint16_t                _product;
    int                     _sel_interface;
    uint8_t                 _recv_endpoint_addr;
    uint8_t                 _send_endpoint_addr;

    bool                    _device_ready;

    bool                    _communicating;
    pthread_mutex_t         _comm_bool_mutex;

    std::string             _lasterror;

    pthread_t               _senderth, _receiverth;

    std::deque<std::vector<unsigned char> >     _recvstack;
    pthread_mutex_t                             _recvstack_mutex;

    pthread_mutex_t         _recvlock_mutex;
    pthread_cond_t          _recvlock_cond;

    libusb_transfer         _recv_transfer_template;
    char*                   _recv_buffer_data;

    void                    senderThreadMethod();
    void                    receiverThreadMethod();

    bool                    checkIfItIsCommunicating();

protected:
    /**
     * Used to set an error message.
     */
    void            setError(std::string which);

    /**
     * Resets the error message, telling there was no problem at the last operation.
     */
    void            setNoProblem();

public:
    /**
     * Constructor. Does nothing.
     */
    SerialCommunicator();
    ~SerialCommunicator();

    /**
     * Inits the CDC Serial Communicator. Searches for an USB device with a given Vendor:Product ID and selects the first of them.
     * From the selected interface, uses the supplied Endpoints for Serial communication.
     * Returns false and throws an exception if something fails.
     * Returns false too if communication is running (you're expected to stop communications before init again)
     */
    bool            init(uint16_t vendor, uint16_t product, int interface, uint8_t recv_endpoint_addr, uint8_t send_endpoint_addr);

    /**
     * Opens two serial communication channels, one for sending and another for receiving.
     * These are implemented with two independent threads.
     * To stop them, use the method stopCommunication() any time.
     */
    bool            turnOnAutomaticReceiving();

    /**
     * Function to stop Serial communication. Makes you able to init this device again.
     */
    bool            turnOffAutomaticReceiving();

    /**
     * Function to send data to the device.
     */
    size_t          send(std::vector<unsigned char> data);
    size_t          send(unsigned char* data, size_t length);


    std::vector<unsigned char> recv();

    /**
     * Locks the thread it's called in, until the USB device receives some data. The first just puts the retrieved data into the stack, and the second
     * makes you able to retrieve it immediatly.
     */
    std::vector<unsigned char>  recvlock();
    virtual void                atReceiving(std::vector<unsigned char> data){}

    /**
     * Gets a description of the last error.
     */
    std::string     getLastError() const;

    /**
     * Prints a description of the last error to stderr.
     */
    void            printLastError() const;
};



#endif // SERIALCOMMUNICATOR_H
