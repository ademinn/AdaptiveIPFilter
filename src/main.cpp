#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>

struct packet
{
    u_int32_t id;
    nfq_q_handle *qh;
    u_int32_t data_len;
    unsigned char *data;
};

const u_int32_t MAX_LEN = 1500;
unsigned char * res_buf;

std::mutex read_mutex;
std::mutex write_mutex;
std::condition_variable read_condition;
std::condition_variable write_condition;
const size_t RING_BUFFER_SIZE = 5;
packet ring_buffer[RING_BUFFER_SIZE];
volatile size_t first = 0;
volatile size_t last = 0;

u_int32_t get_packet_id(nfq_data *nfa)
{
    nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(nfa);
    if (ph)
    {
        return ntohl(ph->packet_id);
    } else
    {
        return 0;
    }
}

size_t next(size_t current)
{
    return (current + 1) % RING_BUFFER_SIZE;
}

bool is_full()
{
    printf("call is_full\n");
    return next(last) == first;
}

bool is_empty()
{
    printf("call is_empty\n");
    return first == last;
}

void add_packet(int id, nfq_q_handle *qh, u_int32_t data_len, unsigned char *data)
{
    std::unique_lock<std::mutex> lock(write_mutex);
    while (is_full())
    {
        write_condition.wait(lock);
    }
    packet *p = &ring_buffer[last];
    p->id = id;
    p->qh = qh;
    p->data_len = data_len;
    if (p->data)
    {
        delete[] p->data;
    }
    p->data = data;
    last = next(last);
    read_condition.notify_one();
}

packet *get_packet()
{
    std::unique_lock<std::mutex> lock(read_mutex);
    while (is_empty())
    {
        read_condition.wait(lock);
    }
    packet *p = &ring_buffer[first];
    first = next(first);
    write_condition.notify_one();
    return p;
}

int cb(nfq_q_handle *qh, nfgenmsg *nfmsg, nfq_data *nfa, void *data)
{
    u_int32_t id = get_packet_id(nfa);
    unsigned char *buf;
    int len = nfq_get_payload(nfa, &buf);
    u_int32_t res_len = std::max(MAX_LEN, (u_int32_t) len);
    unsigned char *res_buf = new unsigned char[res_len];
    memcpy(res_buf, buf, len);
    u_int32_t diff = res_len - len;
    memset(res_buf + len, 0, diff);
    add_packet(id, qh, res_len, res_buf);
    return 0;
}

int process_packets()
{
    while (true)
    {
        printf("entering callback\n");
        packet *p = get_packet();
        nfq_set_verdict(p->qh, p->id, NF_ACCEPT, p->data_len, p->data);
    }
}

int main(int argc, char **argv)
{
        res_buf = new unsigned char[MAX_LEN];
        char buf[4096];
        nfq_handle *h;
        nfq_q_handle *qh;
        int fd;
        int rv;

        printf("opening library handle\n");
        h = nfq_open();
        if (!h) {
                fprintf(stderr, "error during nfq_open()\n");
                exit(1);
        }

        printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
        if (nfq_unbind_pf(h, AF_INET) < 0) {
                fprintf(stderr, "error during nfq_unbind_pf()\n");
                exit(1);
        }

        printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
        if (nfq_bind_pf(h, AF_INET) < 0) {
                fprintf(stderr, "error during nfq_bind_pf()\n");
                exit(1);
        }

        printf("binding this socket to queue '0'\n");
        qh = nfq_create_queue(h,  0, &cb, NULL);
        if (!qh) {
                fprintf(stderr, "error during nfq_create_queue()\n");
                exit(1);
        }

        printf("setting copy_packet mode\n");
        if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
                fprintf(stderr, "can't set packet_copy mode\n");
                exit(1);
        }

        fd = nfq_fd(h);
        std::thread t(process_packets);

        while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
                printf("pkt received\n");
                nfq_handle_packet(h, buf, rv);
        }
        t.join();

        printf("unbinding from queue 0\n");
        nfq_destroy_queue(qh);

#ifdef INSANE
        /* normally, applications SHOULD NOT issue this command, since
         * it detaches other programs/sockets from AF_INET, too ! */
        printf("unbinding from AF_INET\n");
        nfq_unbind_pf(h, AF_INET);
#endif

        printf("closing library handle\n");
        nfq_close(h);

        exit(0);
}
