#include "ethernet_socket.h"

#include <string.h>

#include <stdlib.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>


namespace
{
    int sum_words(u_int16_t *buf, int nwords);
    void ip_checksum(struct ip *ip);
}


ethernet_socket::ethernet_socket(const char *if_name, const char *mac_addr, size_t buffer_size)
    : buffer_size(buffer_size)
{
    ether_addr *dest_mac = ether_aton(mac_addr);
    if_mac = new ifreq();
    socket_address = new sockaddr_ll();
    send_buf = malloc(buffer_size);
    msg_len = sizeof(ether_header) + 4 + sizeof(ip);

    init_socket(if_name, dest_mac);
    fill_send_buf(dest_mac);
}


ethernet_socket::~ethernet_socket()
{
    free(send_buf);
    delete socket_address;
    delete if_mac;
}


void ethernet_socket::send_empty_packet(const nfq_packet& p)
{
    ip *packet_ip = reinterpret_cast<ip *>(p.data);
    iph->ip_src = packet_ip->ip_src;
    iph->ip_dst = packet_ip->ip_dst;
    ip_checksum(iph);

    /* Send packet */
    if (sendto(sockfd, send_buf, msg_len, 0, reinterpret_cast<sockaddr *>(socket_address), sizeof(sockaddr_ll)) < 0)
    {
        throw ethernet_exception("Send failed");
    }
}


void ethernet_socket::init_socket(const char *if_name, const ether_addr* dest_mac)
{
    ifreq if_idx;

    /* Open RAW socket to send on */
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
        throw ethernet_exception("cannot create socket");
    }

    /* Get the index of the interface to send on */
    memset(&if_idx, 0, sizeof(ifreq));
    strncpy(if_idx.ifr_name, if_name, IFNAMSIZ-1);
    if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
    {
        throw ethernet_exception("SIOCGIFINDEX");
    }
    /* Get the MAC address of the interface to send on */
    memset(if_mac, 0, sizeof(ifreq));
    strncpy(if_mac->ifr_name, if_name, IFNAMSIZ-1);
    if (ioctl(sockfd, SIOCGIFHWADDR, if_mac) < 0)
    {
        throw ethernet_exception("SIOCGIFHWADDR");
    }

    /* Index of the network device */
    socket_address->sll_ifindex = if_idx.ifr_ifindex;
    /* Address length*/
    socket_address->sll_halen = ETH_ALEN;
    /* Destination MAC */
    memcpy(socket_address->sll_addr, dest_mac->ether_addr_octet, ETH_ALEN);
}


void ethernet_socket::fill_send_buf(const ether_addr *dest_mac)
{
    ether_header *eh = static_cast<ether_header *>(send_buf);
    iph = reinterpret_cast<ip *>(static_cast<char *>(send_buf) + sizeof(ether_header));

    /* Construct the Ethernet header */
    memset(send_buf, 0, buffer_size);
    /* Ethernet header */
    eh->ether_shost[0] = ((uint8_t *)if_mac->ifr_hwaddr.sa_data)[0];
    eh->ether_shost[1] = ((uint8_t *)if_mac->ifr_hwaddr.sa_data)[1];
    eh->ether_shost[2] = ((uint8_t *)if_mac->ifr_hwaddr.sa_data)[2];
    eh->ether_shost[3] = ((uint8_t *)if_mac->ifr_hwaddr.sa_data)[3];
    eh->ether_shost[4] = ((uint8_t *)if_mac->ifr_hwaddr.sa_data)[4];
    eh->ether_shost[5] = ((uint8_t *)if_mac->ifr_hwaddr.sa_data)[5];
    memcpy(eh->ether_dhost, dest_mac->ether_addr_octet, ETH_ALEN);
    /* Ethertype field */
    eh->ether_type = htons(ETH_P_IP);

    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_len = htons(sizeof(ip));
    iph->ip_id = 0;
    iph->ip_off = 0;
    iph->ip_ttl = 40;
    iph->ip_p = 253;
}


namespace
{
    int sum_words(u_int16_t *buf, int nwords)
    {
        register u_int32_t sum = 0;

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
        register u_int32_t    sum;

        ip->ip_sum = 0;
        sum = sum_words((u_int16_t *) ip, ip->ip_hl << 1);

        sum = (sum >> 16) + (sum & 0xFFFF);
        sum += (sum >> 16);
        sum = ~sum;

        ip->ip_sum = htons(sum);
    }
}
