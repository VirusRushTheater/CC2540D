#ifndef BinarySender_H
#define BinarySender_H

#include <vector>
#include <cstring>

/**
 * Convenience class to deal with the unsigned char vectors.
 */
template <unsigned int bitAmount>
struct BinarySender
{
    unsigned char contents[bitAmount];

    BinarySender(long long fill)
    {
        if(bitAmount <= 8)
            memcpy(contents, &fill, bitAmount);
        else
            memset(contents, static_cast<int>(fill), bitAmount);
    }
};

template <unsigned int bitAmount>
struct BinaryGrabber
{
    unsigned int    position;
    unsigned char*  destination;

    BinaryGrabber(unsigned int _position, void* _destination) :
        position        (_position),
        destination     (static_cast<unsigned char*>(_destination))
    {
    }
};

template <unsigned int bitAmount>
std::vector<unsigned char> operator<< (const std::vector<unsigned char> vec, BinarySender<bitAmount> p8b)
{
    std::vector<unsigned char> retval;
    retval.reserve(vec.size() + bitAmount);
    retval.insert(retval.end(), vec.begin(), vec.end());
    retval.insert(retval.end(), &(p8b.contents[0]), &(p8b.contents[bitAmount]));
    return retval;
}

template <unsigned int bitAmount>
std::vector<unsigned char> operator>> (const std::vector<unsigned char> vec, BinaryGrabber<bitAmount> p8b)
{
    const unsigned char* vecdata = vec.data();
    memcpy(p8b.destination, &(vecdata[p8b.position]), bitAmount);
    return vec;
}

#endif // BinarySender_H
