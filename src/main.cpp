#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ether.h>

// for ether_aton
#include <net/ethernet.h>

#include "nfqueue.h"
#include "ring_buffer.h"

#define BUF_SIZ     1024

ether_addr *dest_mac;

static int sum_words(u_int16_t *buf, int nwords)
{
  register u_int32_t    sum = 0;

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

int sockfd;
struct ifreq if_mac;
struct sockaddr_ll socket_address;

void init_socket(const char *if_name)
{
    struct ifreq if_idx;
    //char ifName[IFNAMSIZ];

    //strcpy(ifName, DEFAULT_IF);

    /* Open RAW socket to send on */
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror("socket");
        exit(1);
    }

    /* Get the index of the interface to send on */
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, if_name, IFNAMSIZ-1);
    if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
    {
        perror("SIOCGIFINDEX");
        exit(1);
    }
    /* Get the MAC address of the interface to send on */
    memset(&if_mac, 0, sizeof(struct ifreq));
    strncpy(if_mac.ifr_name, if_name, IFNAMSIZ-1);
    if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
    {
        perror("SIOCGIFHWADDR");
        exit(1);
    }

    /* Index of the network device */
    socket_address.sll_ifindex = if_idx.ifr_ifindex;
    /* Address length*/
    socket_address.sll_halen = ETH_ALEN;
    /* Destination MAC */
    memcpy(socket_address.sll_addr, dest_mac->ether_addr_octet, ETH_ALEN);
}

void send_empty_packet(in_addr src, in_addr dst)
{
    int tx_len = 0;
    char sendbuf[BUF_SIZ];
    struct ether_header *eh = (struct ether_header *) sendbuf;
    struct ip *ip = (struct ip *) (sendbuf + sizeof(struct ether_header));

    /* Construct the Ethernet header */
    memset(sendbuf, 0, BUF_SIZ);
    /* Ethernet header */
    eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
    eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
    eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
    eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
    eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
    eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
    memcpy(eh->ether_dhost, dest_mac->ether_addr_octet, ETH_ALEN);
    /* Ethertype field */
    eh->ether_type = htons(ETH_P_IP);
    tx_len += sizeof(struct ether_header);

    /* Packet data */
    tx_len += 4;

    ip->ip_hl = 5;
    ip->ip_v = 4;
    ip->ip_len = htons(sizeof(struct iphdr));
    ip->ip_id = 0;
    ip->ip_off = 0;
    ip->ip_ttl = 40;
    ip->ip_p = 253;
    ip->ip_src = src;
    ip->ip_dst = dst;
    ip_checksum(ip);
    tx_len += sizeof(struct ip);

    /* Send packet */
    if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
        printf("Send failed\n");

}


nfqueue queue;
ring_buffer<nfq_packet> rbuf;


int process_packets()
{
    nfq_packet p;
    while (true)
    {
        p = rbuf.pop();
        ip *ip_ = (ip *) p.data;
        send_empty_packet(ip_->ip_src, ip_->ip_dst);
        queue.accept_packet(p);
    }
}


int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("not enough arguments");
        exit(1);
    }
    dest_mac = ether_aton(argv[2]);

    init_socket(argv[1]);
    queue.open();
    nfq_packet p;
    std::thread t(process_packets);
    while (true)
    {
        if (queue.handle_next_packet(p) < 0)
        {
            break;
        }
        memset(p.data + p.data_len, 0, nfq_packet::BUFFER_SIZE - p.data_len);
        p.data_len = nfq_packet::BUFFER_SIZE;
        rbuf.put(p);
    }
    t.join();
    queue.close();
}
