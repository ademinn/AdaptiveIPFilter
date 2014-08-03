#include "packet.h"

#include <string.h>
#include <stdlib.h>


packet::packet()
    : data_len(0), data(malloc(BUFFER_SIZE))
{}


/* implementing copy constructor using assignment operator to avoid unnecessary malloc/free */
packet::packet(const packet& rp)
    : packet()
{
    operator=(rp);
}


packet::~packet()
{
    free(data);
}


packet& packet::operator=(const packet& other)
{
    data_len = other.data_len;
    memcpy(data, other.data, BUFFER_SIZE);
    return *this;
}


void packet::clear()
{
    data_len = 0;
    memset(data, 0, BUFFER_SIZE);
}
