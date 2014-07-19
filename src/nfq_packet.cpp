#include "nfq_packet.h"

#include <string.h>


nfq_packet::nfq_packet()
    : data_len(0), data(new unsigned char [BUFFER_SIZE]), id(0)
{}


nfq_packet::~nfq_packet()
{
    delete[] data;
}


nfq_packet& nfq_packet::operator=(const nfq_packet& other)
{
    id = other.id;
    data_len = other.data_len;
    memcpy(data, other.data, BUFFER_SIZE);
    return *this;
}


u_int32_t nfq_packet::get_id() const
{
    return id;
}
