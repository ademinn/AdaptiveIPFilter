#include "raw_socket.h"

#include <iostream>

#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset(), and memcpy()

#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <sys/ioctl.h>        // macro ioctl is defined
#include <net/if.h>           // struct ifreq

#include "nfq_packet.h"


namespace
{
    int sum_words(u_int16_t *buf, int nwords);
    void ip_checksum(ip *iph);
}


const int raw_socket::ON = 1;


raw_socket::raw_socket(const std::string& if_name, int mark)
    : sd(-1), mark(mark), if_idx(new ifreq),
    iph(static_cast<ip *>(calloc(nfq_packet::MAX_PAYLOAD, 1))),
    sin(new sockaddr_in)
{
    memset(if_idx, 0, sizeof(ifreq));
    strncpy(if_idx->ifr_name, if_name.c_str(), IFNAMSIZ - 1);

    sin->sin_family = AF_INET;

    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_len = htons(sizeof(ip));
    iph->ip_id = 0;
    iph->ip_off = 0;
    iph->ip_ttl = 40;
    iph->ip_p = 253;
}


void raw_socket::open()
{
    /* Open RAW socket to send on */
    if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
    {
        throw raw_socket_exception("socket() failed to get socket descriptor for using ioctl()");
    }

    /* Get the index of the interface to send on */
    if (ioctl(sd, SIOCGIFINDEX, if_idx) < 0)
    {
        throw raw_socket_exception("SIOCGIFINDEX");
    }

    if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, &raw_socket::ON, sizeof(raw_socket::ON)) < 0)
    {
        throw raw_socket_exception("setsockopt() failed to set IP_HDRINCL");
    }

    // Bind socket to interface index.
    if (setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, if_idx, sizeof(ifreq)) < 0)
    {
        throw raw_socket_exception("setsockopt() failed to bind to interface");
    }
    if (setsockopt (sd, SOL_SOCKET, SO_MARK, &mark, sizeof(mark)) < 0)
    {
        throw raw_socket_exception("setsockopt() failed to set mark");
    }
}


void raw_socket::close()
{
    std::cout << "try close socket" << std::endl;
    ::close(sd);
    std::cout << "socket closed" << std::endl;
}


raw_socket::~raw_socket()
{
    close();
    delete iph;
    delete sin;
}


void raw_socket::send_empty_packet(const in_addr &src, const in_addr &dst, size_t payload_len)
{
    if (!payload_len)
    {
        payload_len = sizeof(ip);
    }
    iph->ip_src = src;
    iph->ip_dst = dst;
    ip_checksum(iph);
    sin->sin_addr.s_addr = dst.s_addr;
    if (sendto(sd, iph, payload_len, 0, reinterpret_cast<sockaddr *>(sin), sizeof(sockaddr)) < 0)
    {
        throw raw_socket_exception("sendto() failed");
    }
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


    void ip_checksum(ip *iph)
    {
        u_int32_t sum;

        iph->ip_sum = 0;
        sum = sum_words((u_int16_t *) iph, iph->ip_hl << 1);

        sum = (sum >> 16) + (sum & 0xFFFF);
        sum += (sum >> 16);
        sum = ~sum;

        iph->ip_sum = htons(sum);
    }
}
