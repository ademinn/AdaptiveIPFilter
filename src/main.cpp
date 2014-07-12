#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <algorithm>

const u_int32_t MAX_LEN = 1500;
unsigned char * res_buf;

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

static int cb(nfq_q_handle *qh, nfgenmsg *nfmsg, nfq_data *nfa, void *data)
{
        u_int32_t id = get_packet_id(nfa);
        printf("entering callback\n");
        unsigned char *buf;
        int len = nfq_get_payload(nfa, &buf);
        u_int32_t res_len = std::max(MAX_LEN, (u_int32_t) len);
        memcpy(res_buf, buf, len);
        u_int32_t diff = res_len - len;
        memset(res_buf + len, 0, diff);
        return nfq_set_verdict(qh, id, NF_ACCEPT, res_len, res_buf);
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

        while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
                printf("pkt received\n");
                nfq_handle_packet(h, buf, rv);
        }

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
