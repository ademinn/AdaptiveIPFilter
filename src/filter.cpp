/*
 * proper iptables configuration:
 * Chain OUTPUT (policy ACCEPT)
 * target     prot opt source               destination         
 * ACCEPT     all  --  anywhere             anywhere             mark match 0x2
 *
 * Chain POSTROUTING (policy ACCEPT)
 * target     prot opt source               destination         
 * ACCEPT     all  --  anywhere             anywhere             mark match 0x2
 * NFQUEUE    all  --  anywhere             anywhere             NFQUEUE num 0
 */
#include <iostream>

#include <thread>

#include "nfqueue.h"
#include "ring_buffer.h"
#include "raw_socket.h"


int process_packets(nfqueue& queue, ring_buffer<packet>& rbuf, raw_socket sock)
{
    packet rp;
    nfq_packet p;
    while (true)
    {
        rp = rbuf.pop();
        if (queue.handle_packet(rp, p) != 0)
        {
            continue;
        }
        sock.send_empty_packet(p.src(), p.dst());
        p.set_data_len(1500);
        queue.accept_packet(p);
    }
}


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "interface not specified" << std::endl;
        return 1;
    }
    raw_socket sock(argv[1], 2);
    sock.open();

    nfqueue queue;
    ring_buffer<packet> rbuf;
    queue.open();
    std::thread t(process_packets, std::ref(queue), std::ref(rbuf), std::ref(sock));
    packet rp;
    while (true)
    {
        if (queue.recv_packet(rp) < 0)
        {
            continue;
        }
        rbuf.put(rp);
    }
    t.join();
    queue.close();
    sock.close();
    return 0;
}
