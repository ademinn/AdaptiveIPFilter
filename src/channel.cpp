/*
 * proper iptables configuration:
 * Chain OUTPUT (policy ACCEPT)
 * target     prot opt source               destination         
 * ACCEPT     all  --  anywhere             anywhere             mark match 0x1
 * NFQUEUE    all  --  anywhere             anywhere             NFQUEUE num 1
 */
#include <iostream>
#include <fstream>
#include <string>

#include <chrono>
#include <thread>
#include <atomic>

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>

#include "nfqueue.h"
#include "ring_buffer.h"
#include "raw_socket.h"

#include "iptables.h"


void process_packets(nfqueue& queue, ring_buffer<packet>& rbuf, raw_socket& sock, Json::Value& msg, std::atomic_bool& exit_flag)
{
    packet rp;
    nfq_packet p;
    std::chrono::milliseconds zero(100);
    std::chrono::milliseconds one(500);
    for (int i = 0; i < 10; i++)
    {
        rp = rbuf.pop();
        if (queue.handle_packet(rp, p) != 0)
        {
            continue;
        }
        queue.accept_packet(p);
    }
    int cnt = 0;
    for (Json::Value &value : msg)
    {
        if (value.asBool())
        {
            std::cout << "send one" << std::endl;
            std::this_thread::sleep_for(one);
        } else
        {
            std::cout << "send zero" << std::endl;
            std::this_thread::sleep_for(zero);
        }
        bool packet_sent = false;
        while (!rbuf.is_empty() && !packet_sent)
        {
            rp = rbuf.pop();
            if (queue.handle_packet(rp, p) != 0)
            {
                continue;
            }
            queue.accept_packet(p);
            packet_sent = true;
            std::cout << "bit sent" << std::endl;
        }
        if (!packet_sent)
        {
            sock.send_empty_packet(p.src(), p.dst());
            std::cout << "bit sent" << std::endl;
        }
        cnt++;
    }
    std::cout << "msg sent " << cnt << std::endl;
    while (!exit_flag)
    {
        rp = rbuf.pop();
        if (queue.handle_packet(rp, p) != 0)
        {
            continue;
        }
        //sock.send_empty_packet(p.src(), p.dst());
        //p.set_data_len(1500);
        queue.accept_packet(p);
    }
    std::cout << "exit processing packets" << std::endl;
}


void capture_packets(nfqueue& queue, ring_buffer<packet>& rbuf, std::atomic_bool& exit_flag)
{
    packet rp;
    while (!exit_flag)
    {
        if (queue.recv_packet(rp) < 0)
        {
            continue;
        }
        rbuf.put(rp);
    }
    std::cout << "exit capturing packets" << std::endl;
}


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "config file not specified" << std::endl;
        return 1;
    }
    std::ifstream config(argv[1], std::ios::in);
    //std::string config(argv[1]);
    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(config, root);
    if (!parsingSuccessful)
    {
        std::cout << "failed to parse config file" << std::endl <<
            reader.getFormatedErrorMessages();
        return 1;
    }

    std::string interface = root["interface"].asString();
    int mark = root["mark"].asInt();
    Json::Value msg = root["msg"];

    std::atomic_bool exit_flag(false);
    raw_socket sock(interface, 1);
    sock.open();

    nfqueue queue(nfqueue::PROTOCOL_FAMILY, 2);
    ring_buffer<packet> rbuf;
    queue.open();
    std::thread process_thread(process_packets, std::ref(queue), std::ref(rbuf), std::ref(sock), std::ref(msg), std::ref(exit_flag));
    std::thread capture_thread(capture_packets, std::ref(queue), std::ref(rbuf), std::ref(exit_flag));

    iptables ipt;
    ipt.add_rule("OUTPUT", "-t mangle -m mark --mark 1 -j ACCEPT");
    ipt.add_rule("OUTPUT", std::string("-t mangle -j NFQUEUE --queue-num 2 -o ") + interface);

    std::string action;
    while (!exit_flag)
    {
        std::cin >> action;
        if (action == "exit")
        {
            exit_flag = true;
        }
    }
    std::cout << "exit stdin" << std::endl;
    process_thread.join();
    capture_thread.join();
    queue.close();
    sock.close();
    return 0;
}
