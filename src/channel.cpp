/*
 * proper iptables configuration:
 * Chain OUTPUT (policy ACCEPT)
 * target     prot opt source               destination         
 * ACCEPT     all  --  anywhere             anywhere             mark match 0x1
 * NFQUEUE    all  --  anywhere             anywhere             NFQUEUE num 1
 */
#include <iostream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <thread>
#include <jsoncpp/json/json.h>

#include "config.h"
#include "packet_filter.h"


class channel_config : public config
{
    public:
        channel_config(const Json::Value& value);

        std::vector<bool> bits;
};


channel_config::channel_config(const Json::Value& value)
    : config(value)
{
    Json::Value msg = value["msg"];
    bits.resize(msg.size());
    std::transform(msg.begin(), msg.end(), bits.begin(), [](const Json::Value& v){return v.asBool();});
}


class channel_filter : public packet_filter
{
    public:
        channel_filter(const channel_config& conf)
            : packet_filter(conf, {"OUTPUT"}, "OUTPUT"), bits(conf.bits)
        {}
    protected:
        void process_packets();
    private:
        std::vector<bool> bits;
};


void channel_filter::process_packets()
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
    for (bool value : bits)
    {
        if (value)
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


int main(int argc, char **argv)
{
    channel_filter cf(channel_config(parse(argc, argv)));
    return cf.execute();
}
