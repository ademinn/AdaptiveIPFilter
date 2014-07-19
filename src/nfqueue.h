#pragma once

#include <sys/types.h>
#include <sys/socket.h>

#include <stdexcept>

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
        static const size_t BUFFER_SIZE = 4096;

        nfqueue(u_int16_t pf = PROTOCOL_FAMILY,
                u_int16_t qn = 0,
                size_t buffer_size = BUFFER_SIZE);
        ~nfqueue();

        void open();
        void close();

        int handle_next_packet(nfq_packet&);
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

        size_t buffer_size;
        char *buffer;

        /* need something like singleton here */
        nfq_packet *packet;
};
