#pragma once

#include <sys/types.h>
#include <sys/socket.h>

#include <stdexcept>

#include <mutex>

#include "packet.h"
#include "nfq_packet.h"


struct nfq_handle;
struct nfq_q_handle;
struct nfgenmsg;
struct nfq_data;


class nfqueue_exception : public std::runtime_error
{
    public:
        nfqueue_exception(const char* what_arg)
            : std::runtime_error(what_arg)
        {}
};


class nfqueue
{
    public:
        static const u_int16_t PROTOCOL_FAMILY = AF_INET;

        nfqueue(u_int16_t qn = 0,
                u_int16_t pf = PROTOCOL_FAMILY);
        ~nfqueue();

        void open();
        void close();

        int recv_packet(packet&);
        int handle_packet(const packet&, nfq_packet&);
        int accept_packet(const nfq_packet&);
    private:
        static int callback(nfq_q_handle *, nfgenmsg *, nfq_data *, void *);

        void unsafe_destroy_queue();
        void unsafe_close_handle();

        u_int16_t protocol_family;
        u_int16_t queue_num;

        nfq_handle *handle;
        nfq_q_handle *queue_handle;
        int fd;

        nfq_packet *nfq_p;
        char * buffer;
};
