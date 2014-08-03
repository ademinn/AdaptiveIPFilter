#pragma once

#include <sys/types.h>

class nfqueue;

class packet
{
    public:

        static const u_int32_t BUFFER_SIZE = 8192;

        packet();
        packet(const packet&);
        ~packet();

        packet& operator=(const packet&);

        void clear();
        void * const data;
    protected:
        friend class nfqueue;

        u_int32_t data_len;
};
