#include "serialcommunicator.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <csignal>

#define     LIBUSB_DEBUG_OUTPUT

#define     RECV_BUFFER_SIZE        260

SerialCommunicator::SerialCommunicator() :
    _usbctx         (NULL),
    _usbdev         (NULL),
    _usbhandle      (NULL),

    _device_ready   (false),
    _communicating  (false)
{
    libusb_init(&_usbctx);
    #ifdef LIBUSB_DEBUG_OUTPUT
        libusb_set_debug(_usbctx, LIBUSB_LOG_LEVEL_DEBUG);
    #else
        libusb_set_debug(_usbctx, LIBUSB_LOG_LEVEL_WARNING);
    #endif

    _comm_bool_mutex    = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    _recvstack_mutex    = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    _recvlock_mutex     = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    _recvlock_cond      = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

    //_recv_buffer_data   = new unsigned char[RECV_BUFFER_SIZE];

    setNoProblem();
}

SerialCommunicator::~SerialCommunicator()
{
    stopCommunication();
    if(_usbhandle)
        libusb_close(_usbhandle);
    libusb_exit(_usbctx);
}

bool SerialCommunicator::init(uint16_t vendor, uint16_t product, int interface, uint8_t recv_endpoint_addr, uint8_t send_endpoint_addr)
{
    libusb_device *currentdev       = NULL;
    libusb_device **devlist         = NULL;
    libusb_device_descriptor devdesc;
    bool found                      = false;

    _vendor                 = vendor;
    _product                = product;
    _sel_interface          = interface;
    _recv_endpoint_addr     = recv_endpoint_addr;
    _send_endpoint_addr     = send_endpoint_addr;

    if(_communicating)
    {
        setError("Stop communication before attempting to init this USB again.");
        return false;
    }

    if(_usbhandle)
    {
        libusb_close(_usbhandle);
        _device_ready = false;
    }

    /// Gets a list of all USB-connected devices. Filter from what we need.
    /// If nothing is found, it just returns false. Nothing more.
    int numdevices                  = libusb_get_device_list(_usbctx, &devlist);

    /// Searches between all devices one that matches our vendor and product IDs, and uses the first it finds.
    for(int devcnt = 0; devcnt < numdevices; devcnt++)
    {
        currentdev =    devlist[devcnt];

        /// Retrieves the descriptor in order to know vendor and product ID. Skips if we can't obtain it.
        int errorno =   libusb_get_device_descriptor(currentdev, &devdesc);
        if(errorno < 0)
            continue;

        /// Checks if the current device matches.
        if(devdesc.idProduct == product && devdesc.idVendor == vendor)
        {
            found =     true;
            _usbdev =   currentdev;
            libusb_ref_device(currentdev);
            break;
        }
    }

    if(!found)
        setError("Could not find the USB device %d.");

    /// Releases all other references to devices, we don't need them any longer.
    libusb_free_device_list(devlist, 1);

    if(found == false)
        return false;

    /// Now, let's open an interface for communication.
    int devopen = libusb_open(_usbdev, &_usbhandle);
    switch (devopen)
    {
        case 0:
            /// Success.
        break;
        case LIBUSB_ERROR_ACCESS:
            setError("Couldn't access to the USB device %d. This may be happening because " \
                     "this program hasn't the proper permissions to operate USB devices. Try with sudo.");
            return false;
        break;
        case LIBUSB_ERROR_NO_DEVICE:
            setError("No device detected. Maybe you disconnected the device before attempting communication to it?");
            return false;
        break;
        default:
            setError("An unknown error happened while trying to open the device.");
            return false;
        break;
    }

    /// Request to the OS to release our USB Serial Interface for letting us to use it.
    /// I have yet to find a way to do the same thing on Windows or Darwin.
    int kerval = libusb_kernel_driver_active(_usbhandle, interface);

    if(kerval == 1)
    {
        /// Linux. We try to disconnect the Kernel drivers from the interface.
        int kerdetach = libusb_detach_kernel_driver(_usbhandle, interface);
        switch(kerdetach)
        {
            case 0: /// Success!
            break;
            case LIBUSB_ERROR_NOT_FOUND:
                setError("No kernel driver active. Couldn't detach.");
                return false;
            break;
            case LIBUSB_ERROR_INVALID_PARAM:
                setError("The interface %i for the given USB device %d doesn't exist. Try using lsusb -v -d%d to know what are the available interfaces.");
                return false;
            break;
            case LIBUSB_ERROR_NO_DEVICE:
                setError("No device detected. Maybe you disconnected the device before attempting detaching kernel drivers from it?");
                return false;
            break;
            default:
                setError("An unknown error happened while trying to detach kernel drivers.");
                return false;
            break;
        }
    }
    else if(kerval == LIBUSB_ERROR_NOT_SUPPORTED)
    {
        /// Windows or Darwin.
    }
    else if(kerval != 0)
    {
        switch(kerval)
        {
            case LIBUSB_ERROR_NO_DEVICE:
                setError("No device detected. Maybe you disconnected the device before detaching kernel drivers from it?");
                return false;
            break;
            default:
                setError("An unknown error happened while trying to detach its kernel drivers.");
                return false;
            break;
        }
    }

    /// Let's claim the interface.
    int claimval = libusb_claim_interface(_usbhandle, interface);
    switch(claimval)
    {
        case 0: /// Success!
        break;
        case LIBUSB_ERROR_NO_DEVICE:
            setError("No device detected. Maybe you disconnected the device before claiming its I/O interface?");
            return false;
        break;
        case LIBUSB_ERROR_NOT_FOUND:
            setError("The interface %i for the given USB device %d doesn't exist. Try using lsusb -v -d%d to know what are the available interfaces.");
            return false;
        break;
        case LIBUSB_ERROR_BUSY:
            setError("The interface %i for the given USB device %d is busy at the moment.");
            return false;
        break;
        default:
            setError("An unknown error happened while trying to claim the interface %i.");
            return false;
        break;
    }

    _device_ready = true;

    //libusb_fill_bulk_transfer(&_recv_transfer_template, _usbhandle, _recv_endpoint_addr, _recv_buffer_data, RECV_BUFFER_SIZE, __callbackExternalMethod<LibUSBReceiveCB>, this, 30000);

    setNoProblem();
    return true;
}

bool SerialCommunicator::startCommunication()
{
    if(!_device_ready)
    {
        setError("Your device is not yet ready for serial communication. Use init() first, please.");
        return false;
    }

    _communicating = true;

    //pthread_create(&_senderth, NULL, __callbackExternalMethod<SenderThreadCB>, this);
    pthread_create(&_receiverth, NULL, __callbackExternalMethod<ReceiverThreadCB>, this);
    return true;
}

bool SerialCommunicator::stopCommunication()
{
    pthread_mutex_lock(&_comm_bool_mutex);
    _communicating = false;
    pthread_mutex_unlock(&_comm_bool_mutex);

    //pthread_join(_senderth, NULL);
    pthread_join(_receiverth, NULL);
    return true;
}

/**
 * Main loop for the Sender thread.
 */
void SerialCommunicator::senderThreadMethod()
{
    do
    {
        sleep(2);
        std::cout << "Send" << std::endl;
    } while(checkIfItIsCommunicating());
}

/**
 * Main loop for the Receiver thread. Each time this receives a packet, it's allocated in a stack.
 */
void SerialCommunicator::receiverThreadMethod()
{
    unsigned char* recvdata =   new unsigned char[RECV_BUFFER_SIZE];
    int bytes_transferred;
    int retval;

    do
    {
        // Locks this thread until it receives some data or half a second is elapsed.
        retval = libusb_bulk_transfer(_usbhandle, _recv_endpoint_addr, recvdata, RECV_BUFFER_SIZE, &bytes_transferred, 500);

        switch(retval)
        {
            case LIBUSB_ERROR_PIPE:
            setError("There was a problem with the pipe communication.");
            break;
            case LIBUSB_ERROR_NO_DEVICE:
            setError("The device was disconnected.");
            break;
        }

        // Mutexes and appends to the Received stack.
        if(bytes_transferred != 0)
        {
            pthread_mutex_lock(&_recvstack_mutex);
            _recvstack.push_back(std::vector<unsigned char>(&(recvdata[0]), &(recvdata[bytes_transferred])));
            pthread_mutex_unlock(&_recvstack_mutex);

            pthread_cond_broadcast(&_recvlock_cond);
            atReceiving(std::vector<unsigned char>(&(recvdata[0]), &(recvdata[bytes_transferred])));
        }
        // And broadcasts the recvlock() methods.
    } while(checkIfItIsCommunicating());

    delete recvdata;
}

void SerialCommunicator::setError(std::string which)
{
    int argpos;

    char *deviceid = new char[10];
    char *interstr = new char[10];
    snprintf(deviceid, 10, "%04x:%04x", _vendor, _product);
    snprintf(interstr, 10, "%d", _sel_interface);

    while((argpos = which.find("%d")) != std::string::npos)
        which.replace(argpos, 2, deviceid);

    while((argpos = which.find("%i")) != std::string::npos)
        which.replace(argpos, 2, interstr);

    _lasterror = which;

    delete(deviceid);
    delete(interstr);

    throw(_lasterror);
}

void SerialCommunicator::setNoProblem()
{
    _lasterror = std::string("No problem.");
}

std::string SerialCommunicator::getLastError() const
{
    return std::string(_lasterror);
}

void SerialCommunicator::printLastError() const
{
    std::cerr << "[Serial Communicator Error] " <<
                 _lasterror << std::endl;
}

bool SerialCommunicator::checkIfItIsCommunicating()
{
    bool retval;
    pthread_mutex_lock(&_comm_bool_mutex);
    retval = _communicating;
    pthread_mutex_unlock(&_comm_bool_mutex);

    return retval;
}

size_t SerialCommunicator::send(std::vector<unsigned char> data)
{
    return send(data.data(), data.size());
}

size_t SerialCommunicator::send(unsigned char *data, size_t length)
{
    //if(checkIfItIsCommunicating())
    if(1)
    {
        int bytes_transferred;
        int retval = libusb_bulk_transfer(_usbhandle, _send_endpoint_addr, data, length, &bytes_transferred, 0);
        switch(retval)
        {
            case 0:
                return bytes_transferred;
            break;
            case LIBUSB_ERROR_PIPE:
                setError("The endpoint halted when trying to send.");
                return 0;
            break;
            case LIBUSB_ERROR_NO_DEVICE:
                setError("The device has been disconnected. The communication has stopped.");
                stopCommunication();
                return 0;
            break;
            default:
                setError("Unknown error when trying to send data.");
                return 0;
            break;
        }
    }
    else
    {
        setError("You haven't started communication yet. Use startCommunication() to do this.");
        return 0;
    }
}

std::vector<unsigned char> SerialCommunicator::recv()
{
    unsigned char* recvdata =   new unsigned char[RECV_BUFFER_SIZE];
    int bytes_transferred;
    int retusb = libusb_bulk_transfer(_usbhandle, _recv_endpoint_addr, recvdata, RECV_BUFFER_SIZE, &bytes_transferred, 0);

    std::vector<unsigned char> retval = std::vector<unsigned char>(&(recvdata[0]), &(recvdata[bytes_transferred]));

    delete recvdata;
    return retval;
}

std::vector<unsigned char> SerialCommunicator::recvlock()
{
    std::vector<unsigned char> retval;

    pthread_cond_wait(&_recvlock_cond, &_recvlock_mutex);
    pthread_mutex_lock(&_recvstack_mutex);
    retval = std::vector<unsigned char>(_recvstack.back());
    pthread_mutex_unlock(&_recvstack_mutex);

    return retval;
}

template<CallbackType methodSelector>
void* __callbackExternalMethod(void *castedSCParameter)
{
    SerialCommunicator* sc;

    switch(methodSelector)
    {
        case SenderThreadCB:
            sc = static_cast<SerialCommunicator*>(castedSCParameter);
            sc->senderThreadMethod();
        break;
        case ReceiverThreadCB:
            sc = static_cast<SerialCommunicator*>(castedSCParameter);
            sc->receiverThreadMethod();
        break;
        case LibUSBReceiveCB:

        break;
    }
    return NULL;
}
