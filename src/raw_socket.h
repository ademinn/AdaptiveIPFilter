#pragma once

#include <stdexcept>
#include <string>
#include <netinet/in.h>

struct ip;
struct ifreq;


class raw_socket_exception : public std::runtime_error
{
    public:
        raw_socket_exception(const char* what_arg)
            : std::runtime_error(what_arg)
        {}
};


class raw_socket
{
    public:
        static const int ON;

        raw_socket(const std::string& if_name, int mark);
        ~raw_socket();

        void open();
        void close();

        void send_empty_packet(const in_addr &src, const in_addr &dst);
    private:
        int sd;
        int mark;
        ifreq *if_idx;
        ip *iph;
        sockaddr_in *sin;
};
