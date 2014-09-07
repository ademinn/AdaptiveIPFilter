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

        int zero_delay;
        int one_delay;
        int warm;

        std::vector<bool> bits;
};


channel_config::channel_config(const Json::Value& value)
    : config(value),
    zero_delay(value["zero_delay"].asInt()),
    one_delay(value["one_delay"].asInt()),
    warm(value["warm"].asInt())
{
    Json::Value msg = value["msg"];
    bits.resize(msg.size());
    std::transform(msg.begin(), msg.end(), bits.begin(), [](const Json::Value& v){return v.asBool();});
}


class channel_filter : public packet_filter
{
    public:
        channel_filter(const channel_config& conf)
            : packet_filter(conf, {"OUTPUT"}, "OUTPUT"),
            zero(conf.zero_delay),
            one(conf.one_delay),
            warm(conf.warm),
            bits(conf.bits)
        {}
    protected:
        void process_packets();
    private:
        std::chrono::milliseconds zero;
        std::chrono::milliseconds one;

        int warm;

        std::vector<bool> bits;
};


void channel_filter::process_packets()
{
    packet rp;
    nfq_packet p;
    for (int i = 0; i < 10; i++)
    {
        rp = rbuf.pop();
        if (queue.handle_packet(rp, p) != 0)
        {
            continue;
        }
        queue.accept_packet(p);
    }
    std::cout << "sending message..." << std::endl;
    int last_shown = -1;
    int done = 0;
    int count = 0;
    bool packet_sent;
    for (bool value : bits)
    {
        if (value)
        {
            std::this_thread::sleep_for(one);
        } else
        {
            std::this_thread::sleep_for(zero);
        }
        packet_sent = false;
        while (!rbuf.is_empty() && !packet_sent)
        {
            rp = rbuf.pop();
            if (queue.handle_packet(rp, p) != 0)
            {
                continue;
            }
            queue.accept_packet(p);
            packet_sent = true;
        }
        if (!packet_sent)
        {
            sock.send_empty_packet(p.src(), p.dst());
        }
        //count++;
        //done = (count * 100) / bits.size();
        //if (done != last_shown)
        //{
        //    std::cout << done << "%" << std::endl;
        //    last_shown = done;
        //}
    }
    std::cout << "message sent" << std::endl;
    while (!exit_flag)
    {
        rp = rbuf.pop();
        if (queue.handle_packet(rp, p) != 0)
        {
            continue;
        }
        queue.accept_packet(p);
    }
    std::cout << "exit processing packets" << std::endl;
}


int main(int argc, char **argv)
{
    channel_filter cf(channel_config(parse(argc, argv)));
    return cf.execute();
}
