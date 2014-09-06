#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <atomic>

#include "config.h"

#include "raw_socket.h"
#include "nfqueue.h"
#include "ring_buffer.h"
#include "iptables.h"


class packet_filter_exception : public std::runtime_error
{
    public:
        packet_filter_exception(const std::string& what_arg)
            : std::runtime_error(what_arg)
        {}
};


class packet_filter
{
    public:
        packet_filter(
                const config& conf,
                const std::vector<std::string>& ac,
                const std::string& nc);

        int execute();

    protected:
        virtual void process_packets() = 0;

        std::atomic_bool exit_flag;

        nfqueue queue;
        ring_buffer<packet> rbuf;
        raw_socket sock;

    private:
        void capture_packets();

        std::vector<std::string> accept_chains;
        std::string nfqueue_chain;

        std::string accept_rule;
        std::string nfqueue_rule;

        iptables ipt;
};
