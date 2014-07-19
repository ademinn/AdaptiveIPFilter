#pragma once

#include <sys/types.h>

class nfqueue;

class nfq_packet
{
    public:
        static const u_int32_t BUFFER_SIZE = 1500;

        u_int32_t data_len;
        unsigned char * const data;

        nfq_packet();
        ~nfq_packet();

        nfq_packet& operator=(const nfq_packet&);

        u_int32_t get_id() const;

    private:
        friend class nfqueue;

        u_int32_t id;
};
