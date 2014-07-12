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

#define MY_DEST_MAC0    0x00
#define MY_DEST_MAC1    0x00
#define MY_DEST_MAC2    0x00
#define MY_DEST_MAC3    0x00
#define MY_DEST_MAC4    0x00
#define MY_DEST_MAC5    0x00

#define DEFAULT_IF  "eth0"
#define BUF_SIZ     1024

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

void init_socket()
{
    struct ifreq if_idx;
    char ifName[IFNAMSIZ];

    strcpy(ifName, DEFAULT_IF);

    /* Open RAW socket to send on */
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror("socket");
        exit(1);
    }

    /* Get the index of the interface to send on */
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
    if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
    {
        perror("SIOCGIFINDEX");
        exit(1);
    }
    /* Get the MAC address of the interface to send on */
    memset(&if_mac, 0, sizeof(struct ifreq));
    strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
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
    socket_address.sll_addr[0] = MY_DEST_MAC0;
    socket_address.sll_addr[1] = MY_DEST_MAC1;
    socket_address.sll_addr[2] = MY_DEST_MAC2;
    socket_address.sll_addr[3] = MY_DEST_MAC3;
    socket_address.sll_addr[4] = MY_DEST_MAC4;
    socket_address.sll_addr[5] = MY_DEST_MAC5;
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
    eh->ether_dhost[0] = MY_DEST_MAC0;
    eh->ether_dhost[1] = MY_DEST_MAC1;
    eh->ether_dhost[2] = MY_DEST_MAC2;
    eh->ether_dhost[3] = MY_DEST_MAC3;
    eh->ether_dhost[4] = MY_DEST_MAC4;
    eh->ether_dhost[5] = MY_DEST_MAC5;
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
    inet_aton("192.168.1.106", &ip->ip_dst);
    ip_checksum(ip);
    tx_len += sizeof(struct ip);

    /* Send packet */
    if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
        printf("Send failed\n");

}

struct packet
{
    u_int32_t id;
    nfq_q_handle *qh;
    u_int32_t data_len;
    unsigned char *data;
};

const u_int32_t MAX_LEN = 1500;
unsigned char * res_buf;

std::mutex read_mutex;
std::mutex write_mutex;
std::condition_variable read_condition;
std::condition_variable write_condition;
const size_t RING_BUFFER_SIZE = 5;
packet ring_buffer[RING_BUFFER_SIZE];
volatile size_t first = 0;
volatile size_t last = 0;

u_int32_t get_packet_id(nfq_data *nfa)
{
    nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(nfa);
    if (ph)
    {
        return ntohl(ph->packet_id);
    } else
    {
        return 0;
    }
}

size_t next(size_t current)
{
    return (current + 1) % RING_BUFFER_SIZE;
}

bool is_full()
{
    printf("call is_full\n");
    return next(last) == first;
}

bool is_empty()
{
    printf("call is_empty\n");
    return first == last;
}

void add_packet(int id, nfq_q_handle *qh, u_int32_t data_len, unsigned char *data)
{
    std::unique_lock<std::mutex> lock(write_mutex);
    while (is_full())
    {
        write_condition.wait(lock);
    }
    packet *p = &ring_buffer[last];
    p->id = id;
    p->qh = qh;
    p->data_len = data_len;
    if (p->data)
    {
        delete[] p->data;
    }
    p->data = data;
    last = next(last);
    read_condition.notify_one();
}

packet *get_packet()
{
    std::unique_lock<std::mutex> lock(read_mutex);
    while (is_empty())
    {
        read_condition.wait(lock);
    }
    packet *p = &ring_buffer[first];
    first = next(first);
    write_condition.notify_one();
    return p;
}

int cb(nfq_q_handle *qh, nfgenmsg *nfmsg, nfq_data *nfa, void *data)
{
    u_int32_t id = get_packet_id(nfa);
    unsigned char *buf;
    int len = nfq_get_payload(nfa, &buf);
    u_int32_t res_len = std::max(MAX_LEN, (u_int32_t) len);
    unsigned char *res_buf = new unsigned char[res_len];
    memcpy(res_buf, buf, len);
    u_int32_t diff = res_len - len;
    memset(res_buf + len, 0, diff);
    ip *ip_ = (ip *) buf;
    if (ip_->ip_p == 253)
    {
        exit(1);
    }
    printf("%s", inet_ntoa(ip_->ip_dst));
    add_packet(id, qh, res_len, res_buf);
    return 0;
}

int process_packets()
{
    while (true)
    {
        printf("entering callback\n");
        packet *p = get_packet();
        ip *ip_ = (ip *) p->data;
        send_empty_packet(ip_->ip_src, ip_->ip_dst);
        nfq_set_verdict(p->qh, p->id, NF_ACCEPT, p->data_len, p->data);
    }
}

int main(int argc, char **argv)
{
        init_socket();
        res_buf = new unsigned char[MAX_LEN];
        char buf[4096];
        nfq_handle *h;
        nfq_q_handle *qh;
        int fd;
        int rv;

        printf("opening library handle\n");
        h = nfq_open();
        if (!h) {
                fprintf(stderr, "error during nfq_open()\n");
                exit(1);
        }

        printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
        if (nfq_unbind_pf(h, AF_INET) < 0) {
                fprintf(stderr, "error during nfq_unbind_pf()\n");
                exit(1);
        }

        printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
        if (nfq_bind_pf(h, AF_INET) < 0) {
                fprintf(stderr, "error during nfq_bind_pf()\n");
                exit(1);
        }

        printf("binding this socket to queue '0'\n");
        qh = nfq_create_queue(h,  0, &cb, NULL);
        if (!qh) {
                fprintf(stderr, "error during nfq_create_queue()\n");
                exit(1);
        }

        printf("setting copy_packet mode\n");
        if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
                fprintf(stderr, "can't set packet_copy mode\n");
                exit(1);
        }

        fd = nfq_fd(h);
        std::thread t(process_packets);

        while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
                printf("pkt received\n");
                nfq_handle_packet(h, buf, rv);
        }
        t.join();

        printf("unbinding from queue 0\n");
        nfq_destroy_queue(qh);

#ifdef INSANE
        /* normally, applications SHOULD NOT issue this command, since
         * it detaches other programs/sockets from AF_INET, too ! */
        printf("unbinding from AF_INET\n");
        nfq_unbind_pf(h, AF_INET);
#endif

        printf("closing library handle\n");
        nfq_close(h);

        exit(0);
}
