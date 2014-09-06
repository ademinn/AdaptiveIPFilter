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
#include "config.h"
#include "packet_filter.h"

class adaptive_filter : public packet_filter
{
    public:
        adaptive_filter(const config& conf)
            : packet_filter(conf, {"OUTPUT", "POSTROUTING"}, "POSTROUTING")
        {}
    protected:
        void process_packets();
};


void adaptive_filter::process_packets()
{
    packet rp;
    nfq_packet p;
    while (!exit_flag)
    {
        rp = rbuf.pop();
        if (queue.handle_packet(rp, p) != 0)
        {
            continue;
        }
        //sock.send_empty_packet(p.src(), p.dst());
        p.set_data_len(1500);
        queue.accept_packet(p);
    }
    std::cout << "exit processing packets" << std::endl;
}


int main(int argc, char **argv)
{
    adaptive_filter af(config(parse(argc, argv)));
    return af.execute();
}
