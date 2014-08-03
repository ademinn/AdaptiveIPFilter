#include "nfq_packet.h"

#include <string.h>
#include <stdlib.h>

#include <netinet/ip.h>


namespace
{
    int sum_words(u_int16_t *buf, int nwords);
    void ip_checksum(struct ip *ip);
}


nfq_packet::nfq_packet()
    : packet(), id(0)
{}


nfq_packet::nfq_packet(const nfq_packet& p)
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


void nfq_packet::drop_ip_payload()
{
    u_int16_t ip_len = sizeof(ip);
    ip *iph = static_cast<ip *>(data);
    iph->ip_len = htons(ip_len);
    ip_checksum(iph);
    void *non_data = static_cast<char *>(data) + ip_len;
    size_t non_data_len = BUFFER_SIZE - ip_len;
    memset(non_data, 0, non_data_len);
}


void nfq_packet::copy_addr(const nfq_packet& p)
{
    ip *iph = static_cast<ip *>(data);
    ip *p_iph = static_cast<ip *>(p.data);
    iph->ip_src = p_iph->ip_src;
    iph->ip_dst = p_iph->ip_dst;
    ip_checksum(iph);
}


namespace
{
    int sum_words(u_int16_t *buf, int nwords)
    {
        u_int32_t sum = 0;

        while (nwords >= 16)
        {
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            sum += (u_int16_t) ntohs(*buf++);
            nwords -= 16;
        }
        while (nwords--)
            sum += (u_int16_t) ntohs(*buf++);
        return(sum);
    }


    void ip_checksum(struct ip *ip)
    {
        u_int32_t sum;

        ip->ip_sum = 0;
        sum = sum_words((u_int16_t *) ip, ip->ip_hl << 1);

        sum = (sum >> 16) + (sum & 0xFFFF);
        sum += (sum >> 16);
        sum = ~sum;

        ip->ip_sum = htons(sum);
    }
}
