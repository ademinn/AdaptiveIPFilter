#include "packet_filter.h"

#include <iostream>
#include <sstream>

#include <thread>


packet_filter::packet_filter(
        const config& conf,
        const std::vector<std::string>& ac,
        const std::string& nc)
    : exit_flag(false), queue(conf.queue_num), sock(conf.interface, conf.mark),
    accept_chains(ac), nfqueue_chain(nc)
{
    std::stringstream accept_rule_stream;
    accept_rule_stream << "-t mangle -j ACCEPT -m mark" <<
        " --mark " << conf.mark <<
        " -o " << conf.interface;
    accept_rule = accept_rule_stream.str();

    std::stringstream nfqueue_rule_stream;
    nfqueue_rule_stream << "-t mangle -j NFQUEUE" <<
        " --queue-num " << conf.queue_num <<
        " -o " << conf.interface;
    nfqueue_rule = nfqueue_rule_stream.str();
}


int packet_filter::execute()
{
    queue.open();
    sock.open();
    std::thread process_thread([this](){this->process_packets();});
    std::thread capture_thread([this](){this->capture_packets();});

    for (const std::string& chain : accept_chains)
    {
        ipt.add_rule(chain, accept_rule);
    }
    ipt.add_rule(nfqueue_chain, nfqueue_rule);

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
    ipt.clear();
    queue.close();
    sock.close();
    return 0;
}


void packet_filter::capture_packets()
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
