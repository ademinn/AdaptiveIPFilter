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


int process_packets(const std::vector<bool>& code, nfqueue& queue, ring_buffer<nfq_packet>& rbuf, ethernet_socket& sock)
{
    nfq_packet p;
    std::chrono::milliseconds timeout;
    size_t count;
    for (size_t i = 0; i < WARMUP; i++)
    {
        p = rbuf.pop();
        queue.accept_packet(p);
    }
    for (const bool &b : code)
    {
        if (b)
        {
            timeout = ONE;
            count = ONE_COUNT;
        } else
        {
            timeout = ZERO;
            count = ZERO_COUNT;
        }
        for (size_t i = 0; i < count; i++)
        {
            std::this_thread::sleep_for(timeout);
            if (rbuf.is_empty())
            {
                printf("send empty\n");
                sock.send_empty_packet(p);
            } else
            {
                printf("send non-empty\n");
                p = rbuf.pop();
                queue.accept_packet(p);
            }
        }
    }
    while (true)
    {
        p = rbuf.pop();
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
    std::vector<bool> code = { false, true, true, false, true };
    std::thread t(process_packets, std::ref(code), std::ref(queue), std::ref(rbuf), std::ref(sock));
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
