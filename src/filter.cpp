#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <thread>
#include <functional>

#include "nfqueue.h"
#include "ring_buffer.h"
#include "ethernet_socket.h"


int process_packets(nfqueue& queue, ring_buffer<nfq_packet>& rbuf, ethernet_socket& sock)
{
    nfq_packet p;
    while (true)
    {
        p = rbuf.pop();
        sock.send_empty_packet(p);
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
    ethernet_socket sock(argv[1], argv[2]);

    nfqueue queue;
    ring_buffer<nfq_packet> rbuf;
    queue.open();
    std::thread t(process_packets, std::ref(queue), std::ref(rbuf), std::ref(sock));
    nfq_packet p;
    while (true)
    {
        if (queue.handle_next_packet(p) != 0)
        {
            continue;
        }
        memset(p.data + p.data_len, 0, nfq_packet::BUFFER_SIZE - p.data_len);
        p.data_len = nfq_packet::BUFFER_SIZE;
        rbuf.put(p);
    }
    t.join();
    queue.close();
}
