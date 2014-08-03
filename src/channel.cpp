/*
 * proper iptables configuration:
 * Chain OUTPUT (policy ACCEPT)
 * target     prot opt source               destination         
 * ACCEPT     all  --  anywhere             anywhere             mark match 0x1
 * NFQUEUE    all  --  anywhere             anywhere             NFQUEUE num 1
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <thread>
#include <functional>
#include <chrono>

#include <vector>

#include "nfqueue.h"
#include "ring_buffer.h"
#include "ethernet_socket.h"


const std::chrono::milliseconds ZERO(100);
const size_t ZERO_COUNT = 10;

const std::chrono::milliseconds ONE(50);
const size_t ONE_COUNT = 20;

const size_t WARMUP = 10;


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset(), and memcpy()

#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_RAW, IPPROTO_IP, IPPROTO_TCP, INET_ADDRSTRLEN
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#define __FAVOR_BSD           // Use BSD format of tcp header
#include <netinet/tcp.h>      // struct tcphdr
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
#include <net/if.h>           // struct ifreq

#include <errno.h>            // errno, perror()

int sd;
void *send_buf;
ifreq if_idx;
sockaddr_in sin;
ip *iph;

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

void init_socket(const char *if_name)
{
    int on = 1;
    int one = 1;

    /* Open RAW socket to send on */
    if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror ("socket() failed to get socket descriptor for using ioctl() ");
        exit (1);
    }

    /* Get the index of the interface to send on */
    memset(&if_idx, 0, sizeof(ifreq));
    strncpy(if_idx.ifr_name, if_name, IFNAMSIZ-1);
    if (ioctl(sd, SIOCGIFINDEX, &if_idx) < 0)
    {
        throw ethernet_exception("SIOCGIFINDEX");
    }
    sin.sin_family = AF_INET;
    if (setsockopt (sd, IPPROTO_IP, IP_HDRINCL, &on, sizeof (on)) < 0) {
        perror ("setsockopt() failed to set IP_HDRINCL ");
        exit (1);
    }

    // Bind socket to interface index.
    if (setsockopt (sd, SOL_SOCKET, SO_BINDTODEVICE, &if_idx, sizeof (if_idx)) < 0) {
        perror ("setsockopt() failed to bind to interface ");
        exit (1);
    }
    if (setsockopt (sd, SOL_SOCKET, SO_MARK, &one, sizeof(one)) < 0) {
        perror ("setsockopt() failed to set mark");
        exit (1);
    }
    iph = static_cast<ip *>(send_buf);
}

void send_empty_packet(nfq_packet &p)
{
    ip *_iph = reinterpret_cast<ip *>(p.data);
    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_len = htons(sizeof(ip));
    iph->ip_id = 0;
    iph->ip_off = 0;
    iph->ip_ttl = 40;
    iph->ip_p = 253;
    iph->ip_src = _iph->ip_src;
    iph->ip_dst = _iph->ip_dst;
    ip_checksum(iph);
    sin.sin_addr.s_addr = _iph->ip_dst.s_addr;
    if (sendto (sd, send_buf, sizeof(ip), 0, (struct sockaddr *) &sin, sizeof (struct sockaddr)) < 0)  {
        perror ("sendto() failed ");
        exit (1);
    }
}


int process_packets(const std::vector<bool>& code, nfqueue& queue, ring_buffer<packet>& rbuf)
{
    packet rp;
    nfq_packet p;
    // std::chrono::milliseconds timeout;
    // size_t count;
    // for (size_t i = 0; i < WARMUP; i++)
    // {
    //     p = rbuf.pop();
    //     queue.accept_packet(p);
    // }
    // for (const bool &b : code)
    // {
    //     if (b)
    //     {
    //         timeout = ONE;
    //         count = ONE_COUNT;
    //     } else
    //     {
    //         timeout = ZERO;
    //         count = ZERO_COUNT;
    //     }
    //     for (size_t i = 0; i < count; i++)
    //     {
    //         std::this_thread::sleep_for(timeout);
    //         if (rbuf.is_empty())
    //         {
    //             printf("send empty\n");
    //             sock.send_empty_packet(p);
    //         } else
    //         {
    //             printf("send non-empty\n");
    //             p = rbuf.pop();
    //             queue.accept_packet(p);
    //         }
    //     }
    // }
    while (true)
    {
        rp = rbuf.pop();
        queue.handle_packet(rp, p);
        //sock.send_empty_packet(p);
        send_empty_packet(p);
        p.set_data_len(1500);
        queue.accept_packet(p);
    }
}


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("not enough arguments");
        exit(1);
    }
    send_buf = malloc(1024);
    init_socket(argv[1]);

    nfqueue queue(nfqueue::PROTOCOL_FAMILY, 1);
    ring_buffer<packet> rbuf;
    queue.open();
    std::vector<bool> code = { false, true, true, false, true };
    std::thread t(process_packets, std::ref(code), std::ref(queue), std::ref(rbuf));
    packet rp;
    //nfq_packet p;
    while (true)
    {
        if (queue.recv_packet(rp) < 0)
        {
            continue;
        }
        //memset(p.data + p.data_len, 0, nfq_packet::BUFFER_SIZE - p.data_len);
        ////p.data_len = nfq_packet::BUFFER_SIZE;
        //rbuf.put(p);
        //if (queue.handle_next_packet(p) != 0)
        //{
        //    printf("err\n");
        //}
        //memset(p.data + p.data_len, 0, nfq_packet::BUFFER_SIZE - p.data_len);
        //p.data_len = 100;
        rbuf.put(rp);
    }
    t.join();
    queue.close();
    free(send_buf);
    close(sd);
}
