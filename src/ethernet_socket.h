#pragma once

#include <stdexcept>

#include "nfq_packet.h"

struct ether_addr;
struct ifreq;
struct sockaddr_ll;
struct ip;


class ethernet_exception : public std::runtime_error
{
    public:
        ethernet_exception(const char* what_arg)
            : std::runtime_error(what_arg)
        {}
};


class ethernet_socket
{
    public:
        static const size_t BUFFER_SIZE = 1024;
        ethernet_socket(const char *if_name, const char *mac_addr, size_t buffer_size = BUFFER_SIZE);
        ~ethernet_socket();

        void send_empty_packet(const nfq_packet&);
    private:
        void init_socket(const char *if_name, const ether_addr *dest_mac);
        void fill_send_buf(const ether_addr *dest_mac);

        ifreq *if_mac;
        sockaddr_ll *socket_address;

        int sockfd;

        size_t buffer_size;
        void *send_buf;
        ip *iph;
        size_t msg_len;
};
