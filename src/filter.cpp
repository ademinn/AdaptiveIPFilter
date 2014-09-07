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
#include <algorithm>
#include <vector>
#include <chrono>
#include <thread>
#include <jsoncpp/json/json.h>

#include "config.h"
#include "packet_filter.h"


class filter_config : public config
{
    public:
        filter_config(const Json::Value& value);

        int delay;
        std::vector<double> coeff;
};


filter_config::filter_config(const Json::Value& value)
    : config(value),
    delay(value["delay"].asInt())
{
    Json::Value c = value["coeff"];
    coeff.resize(c.size());
    std::transform(c.begin(), c.end(), coeff.begin(), [](const Json::Value& v){return v.asDouble();});
}


class adaptive_filter : public packet_filter
{
    public:
        adaptive_filter(const filter_config& conf)
            : packet_filter(conf, {"OUTPUT", "POSTROUTING"}, "POSTROUTING"),
            delay(conf.delay),
            coeff(conf.coeff)
        {}
    protected:
        void process_packets();
    private:
        std::chrono::milliseconds delay;

        std::vector<double> coeff;
};


void adaptive_filter::process_packets()
{
    packet rp;
    nfq_packet p;
    bool packet_sent;
    while (!exit_flag)
    {
        packet_sent = false;
        while (!rbuf.is_empty() && !packet_sent)
        {
            rp = rbuf.pop();
            if (queue.handle_packet(rp, p) != 0)
            {
                continue;
            }
            p.set_data_len(nfq_packet::MAX_PAYLOAD);
            queue.accept_packet(p);
            packet_sent = true;
        }
        if (!packet_sent)
        {
            sock.send_empty_packet(p.src(), p.dst());
        }
        std::this_thread::sleep_for(delay);
    }
    std::cout << "exit processing packets" << std::endl;
}


int main(int argc, char **argv)
{
    adaptive_filter af(filter_config(parse(argc, argv)));
    return af.execute();
}
