/*
 * proper iptables configuration:
 * Chain OUTPUT (policy ACCEPT)
 * target     prot opt source               destination         
 * ACCEPT     all  --  anywhere             anywhere             mark match 0x1
 * NFQUEUE    all  --  anywhere             anywhere             NFQUEUE num 1
 */
#include <iostream>

#include <thread>
#include <functional>
#include <chrono>

#include <vector>

#include "nfqueue.h"
#include "ring_buffer.h"
#include "raw_socket.h"


const std::chrono::milliseconds ZERO(100);
const size_t ZERO_COUNT = 10;

const std::chrono::milliseconds ONE(50);
const size_t ONE_COUNT = 20;

const size_t WARMUP = 10;


int process_packets(const std::vector<bool>& code, nfqueue& queue, ring_buffer<packet>& rbuf, raw_socket sock)
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
        if (queue.handle_packet(rp, p) != 0)
        {
            continue;
        }
        sock.send_empty_packet(p.src(), p.dst());
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
    raw_socket sock(argv[1], 1);
    sock.open();

    nfqueue queue(nfqueue::PROTOCOL_FAMILY, 1);
    ring_buffer<packet> rbuf;
    queue.open();
    std::vector<bool> code = { false, true, true, false, true };
    std::thread t(process_packets, std::ref(code), std::ref(queue), std::ref(rbuf), std::ref(sock));
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
