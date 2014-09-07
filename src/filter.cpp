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
#include <queue>
#include <chrono>
#include <thread>
#include <random>
#include <jsoncpp/json/json.h>

#include "config.h"
#include "packet_filter.h"


class history
{
    public:
        history(size_t size)
            : max_size(size),
            count(0)
        {}

        void add_zero();
        void add_one();
        
        size_t count_ones() const;
    private:
        void align();

        size_t max_size;
        std::queue<bool> queue;

        size_t count;
};


void history::add_zero()
{
    queue.push(false);
    align();
}


void history::add_one()
{
    queue.push(true);
    count++;
    align();
}


size_t history::count_ones() const
{
    return count;
}


void history::align()
{
    while (queue.size() > max_size)
    {
        bool value = queue.front();
        if (value)
        {
            count--;
        }
        queue.pop();
    }
}


class filter_config : public config
{
    public:
        filter_config(const Json::Value& value);

        int delay;
        int hist_size;

        std::vector<double> coeff;
};


filter_config::filter_config(const Json::Value& value)
    : config(value),
    delay(value["delay"].asInt()),
    hist_size(value["N"].asInt() - 1)
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
            coeff(conf.coeff),
            hist(conf.hist_size),
            dist(0.0, 1.0)
        {}
    protected:
        void process_packets();
    private:
        bool event(double p);

        std::chrono::milliseconds delay;
        std::vector<double> coeff;
        history hist;

        std::default_random_engine gen;
        std::uniform_real_distribution<double> dist;
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
            if (event(coeff[hist.count_ones()]))
            {
                sock.send_empty_packet(p.src(), p.dst(), nfq_packet::MAX_PAYLOAD);
                hist.add_zero();
            }
        } else
        {
            hist.add_one();
        }
        std::this_thread::sleep_for(delay);
    }
    std::cout << "exit processing packets" << std::endl;
}


bool adaptive_filter::event(double p)
{
    return p > dist(gen);
}


int main(int argc, char **argv)
{
    adaptive_filter af(filter_config(parse(argc, argv)));
    return af.execute();
}
