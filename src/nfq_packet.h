#pragma once

#include "packet.h"
#include <netinet/in.h>

struct ip;


class nfq_packet : public packet
{
    public:
        nfq_packet();
        nfq_packet(const nfq_packet&);

        nfq_packet& operator=(const nfq_packet&);

        void set_data_len(u_int32_t);
        const in_addr& src() const;
        const in_addr& dst() const;
    private:
        friend class nfqueue;

        u_int32_t id;
        ip *iph;
};
