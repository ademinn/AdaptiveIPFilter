#include "nfq_packet.h"

#include <string.h>
#include <stdlib.h>

#include <netinet/ip.h>


nfq_packet::nfq_packet()
    : packet(), id(0)
{
    iph = static_cast<ip *>(data);
}


/* implementing copy constructor using assignment operator to avoid unnecessary malloc/free */
nfq_packet::nfq_packet(const nfq_packet& p)
    : nfq_packet()
{
    operator=(p);
}


nfq_packet& nfq_packet::operator=(const nfq_packet& other)
{
    packet::operator=(other);
    id = other.id;
    return *this;
}


void nfq_packet::set_data_len(u_int32_t new_len)
{
    data_len = new_len;
}


const in_addr& nfq_packet::src() const
{
    return iph->ip_src;
}


const in_addr& nfq_packet::dst() const
{
    return iph->ip_dst;
}
