#include "nfqueue.h"

#include <string.h>
#include <netinet/in.h>
#include <linux/netfilter.h>

/* for uint types in libnetfilter_queue */
#include <stdint.h>

#include <libnetfilter_queue/libnetfilter_queue.h>


/* nfqueue object is in <data> parameter because
 * I don't know how to pass pointer to member function to nfq_create_queue */
namespace
{
    u_int32_t get_packet_id(nfq_data *nfa);
}

int callback_cast(nfq_q_handle *qh, nfgenmsg *nfmsg, nfq_data *nfa, void *data);

nfqueue::nfqueue(u_int16_t pf, u_int16_t qn, size_t buffer_size)
    :protocol_family(pf), queue_num(qn), handle(0), queue_handle(0), buffer_size(buffer_size)
{
    buffer = new char[buffer_size];
}


nfqueue::~nfqueue()
{
    close();
    delete[] buffer;
}


void nfqueue::open()
{
    handle = nfq_open();
    if (!handle) {
        throw nfqueue_exception("error during nfq_open()");
    }

    if (nfq_unbind_pf(handle, protocol_family) < 0) {
        throw nfqueue_exception("error during nfq_unbind_pf()");
    }

    if (nfq_bind_pf(handle, protocol_family) < 0) {
        throw nfqueue_exception("error during nfq_bind_pf()");
    }

    queue_handle = nfq_create_queue(handle,  queue_num, callback_cast, this);
    if (!queue_handle) {
        throw nfqueue_exception("error during nfq_create_queue()");
    }

    if (nfq_set_mode(queue_handle, NFQNL_COPY_PACKET, 0xffff) < 0) {
        throw nfqueue_exception("can't set packet_copy mode");
    }

    fd = nfq_fd(handle);
}


void nfqueue::close()
{
    if (handle)
    {
        if (queue_handle)
        {
            unsafe_destroy_queue();
        }
        unsafe_close_handle();
    }
}


int nfqueue::handle_next_packet()
{
    int len = recv(fd, buffer, buffer_size, 0);
    if (len > 0)
    {
        return nfq_handle_packet(handle, buffer, len);
    } else
    {
        return len;
    }
}


int nfqueue::accept_packet(const nfq_packet& packet)
{
    return nfq_set_verdict(queue_handle, packet.get_id(), NF_ACCEPT, packet.data_len, packet.data);
}

void nfqueue::unsafe_destroy_queue()
{
    nfq_destroy_queue(queue_handle);
}


void nfqueue::unsafe_close_handle()
{
#ifdef INSANE
    /* normally, applications SHOULD NOT issue this command, since
     * it detaches other programs/sockets from <protocol_family>, too ! */
    nfq_unbind_pf(handle, protocol_family);
#endif
    nfq_close(handle);
}


nfq_packet::nfq_packet()
    : data_len(0), data(new unsigned char [BUFFER_SIZE]), id(0)
{}


nfq_packet::~nfq_packet()
{
    delete[] data;
}


nfq_packet& nfq_packet::operator=(const nfq_packet& other)
{
    id = other.id;
    data_len = other.data_len;
    memcpy(data, other.data, BUFFER_SIZE);
    return *this;
}


u_int32_t nfq_packet::get_id() const
{
    return id;
}


int callback_cast(nfq_q_handle *qh, nfgenmsg *nfmsg, nfq_data *nfa, void *data)
{
    nfqueue *nfq = static_cast<nfqueue *>(data);
    unsigned char *buf;
    int len = nfq_get_payload(nfa, &buf);

    nfq->packet.id = get_packet_id(nfa);
    nfq->packet.data_len = static_cast<u_int32_t>(len);
    memcpy(nfq->packet.data, buf, len);
    return nfq->callback(nfq->packet);
}


namespace
{
    u_int32_t get_packet_id(nfq_data *nfa)
    {
        nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(nfa);
        if (ph)
        {
            return ntohl(ph->packet_id);
        } else
        {
            throw nfqueue_exception("bad packet header");
        }
    }
}
