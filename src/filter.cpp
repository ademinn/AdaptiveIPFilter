#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <thread>
#include <functional>
#include <chrono>

#include "nfqueue.h"
#include "ring_buffer.h"
//#include "ethernet_socket.h"

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <algorithm>

int process_packets(nfqueue& queue, ring_buffer<packet>& rbuf)
{
    std::chrono::milliseconds timeout(10);
    packet rp;
    nfq_packet p;
    packet empty;
    nfq_packet empty_nfq;
    while (true)
    {
        //std::this_thread::sleep_for(timeout);
        //if (queue.handle_packet(empty, p) != 0)
        //{
        //    continue;
        //}
        rp = rbuf.pop();
        printf("next packet\n");
        //rp = rbuf.pop();
        if (queue.handle_packet(rp, p) != 0)
        {
            continue;
        }
        //sock.send_empty_packet(p);
        //sock.copy_data(empty);
        //queue.accept_packet(empty);
        //printf("%d\n", p.get_id());
        p.set_data_len(1500);
        queue.accept_packet(p);
        queue.handle_packet(empty, empty_nfq);
        empty_nfq.drop_ip_payload();
        empty_nfq.copy_addr(p);
        queue.accept_packet(empty_nfq);
    }
}


int main()
{
//    if (argc < 3)
//    {
//        printf("not enough arguments");
//        exit(1);
//    }
//    ethernet_socket sock(argv[1], argv[2]);
//
    nfqueue queue;
    ring_buffer<packet> rbuf;
    queue.open();
    std::thread t(process_packets, std::ref(queue), std::ref(rbuf));
    packet rp;
    nfq_packet p;
    while (true)
    {
        if (queue.recv_packet(rp) < 0)
        {
            continue;
        }
        //if (queue.handle_packet(rp, p) != 0)
        //{
        //    continue;
        //}
        rbuf.put(rp);
        //printf("%ld\n", p.data_len);
        //for (int i = 0; i < std::min(40Lu, p.data_len - sizeof(ip)); i++)
        //{
        //    ip *iph = reinterpret_cast<ip *>(queue.buffer + i);
        //    printf("%d -> %s\n", i, inet_ntoa(iph->ip_src));
        //}
        //memset(p.data + p.data_len, 0, nfq_packet::BUFFER_SIZE - p.data_len);
        //p.data_len = nfq_packet::BUFFER_SIZE;
        //rbuf.put(p);
        //if (queue.handle_next_packet(p) != 0)
        //{
        //    printf("err\n");
        //}
        //memset(p.data + p.data_len, 0, nfq_packet::BUFFER_SIZE - p.data_len);
        //p.data_len = 100;
        //rbuf.put(p);
    }
    t.join();
    queue.close();
    return 0;
}
