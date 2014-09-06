#include "nfqueue.h"

#include <string.h>
#include <netinet/in.h>
#include <linux/netfilter.h>

/* for uint types in libnetfilter_queue */
#include <stdint.h>

#include <libnetfilter_queue/libnetfilter_queue.h>

#include <stdio.h>

namespace
{
    u_int32_t get_packet_id(nfq_data *nfa);
}


nfqueue::nfqueue(u_int16_t pf, u_int16_t qn)
    :protocol_family(pf), queue_num(qn), handle(0), queue_handle(0), nfq_p(0), buffer(new char[8192])
{
}


nfqueue::~nfqueue()
{
    close();
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

    queue_handle = nfq_create_queue(handle,  queue_num, callback, &(this->nfq_p));
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


int nfqueue::recv_packet(packet& rp)
{
    int len = recv(fd, rp.data, rp.BUFFER_SIZE, 0);
    rp.data_len = static_cast<u_int32_t>(len);
    return len;
}


int nfqueue::handle_packet(const packet& rp, nfq_packet& p)
{
    nfq_p = &p;
    int result = nfq_handle_packet(handle, static_cast<char *>(rp.data), rp.data_len);
    return result;
}


int nfqueue::accept_packet(const nfq_packet& p)
{
    int result = nfq_set_verdict(queue_handle, p.id, NF_ACCEPT, p.data_len, static_cast<unsigned char *>(p.data));
    return result;
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


int nfqueue::callback(nfq_q_handle *, nfgenmsg *, nfq_data *nfa, void *data)
{
    nfq_packet *p = *(static_cast<nfq_packet **>(data));
    unsigned char *buf;
    int len = nfq_get_payload(nfa, &buf);

    p->id = get_packet_id(nfa);
    p->data_len = static_cast<u_int32_t>(len);
    memcpy(p->data, buf, len);
    return 0;
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
