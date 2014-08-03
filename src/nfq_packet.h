#pragma once

#include "packet.h"

class nfq_packet : public packet
{
    public:
        nfq_packet();
        nfq_packet(const nfq_packet&);

        nfq_packet& operator=(const nfq_packet&);

        void set_data_len(u_int32_t);
        void drop_ip_payload();
        void copy_addr(const nfq_packet&);
    private:
        friend class nfqueue;
        u_int32_t id;
};
