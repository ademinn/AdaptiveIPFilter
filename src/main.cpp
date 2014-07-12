#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/types.h>
#include <linux/netfilter.h>            /* for NF_ACCEPT */

#include <libnetfilter_queue/libnetfilter_queue.h>
#include <algorithm>

static int sum_words(u_int16_t *buf, int nwords)
{
  register u_int32_t    sum = 0;

  while (nwords >= 16)
  {
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    nwords -= 16;
  }
  while (nwords--)
    sum += (u_int16_t) ntohs(*buf++);
  return(sum);
}

void ip_checksum(struct ip *ip)
{
  register u_int32_t    sum;

  ip->ip_sum = 0;
  sum = sum_words((u_int16_t *) ip, ip->ip_hl << 1);

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  sum = ~sum;

  ip->ip_sum = htons(sum);
}

/* returns packet id */
static u_int32_t print_pkt (struct nfq_data *tb)
{
        int id = 0;
        struct nfqnl_msg_packet_hdr *ph;
        struct nfqnl_msg_packet_hw *hwph;
        u_int32_t mark,ifi; 
        int ret;
        unsigned char *data;

        ph = nfq_get_msg_packet_hdr(tb);
        if (ph) {
                id = ntohl(ph->packet_id);
                printf("hw_protocol=0x%04x hook=%u id=%u ",
                        ntohs(ph->hw_protocol), ph->hook, id);
        }

        hwph = nfq_get_packet_hw(tb);
        if (hwph) {
                int i, hlen = ntohs(hwph->hw_addrlen);

                printf("hw_src_addr=");
                for (i = 0; i < hlen-1; i++)
                        printf("%02x:", hwph->hw_addr[i]);
                printf("%02x ", hwph->hw_addr[hlen-1]);
        }

        mark = nfq_get_nfmark(tb);
        if (mark)
                printf("mark=%u ", mark);

        ifi = nfq_get_indev(tb);
        if (ifi)
                printf("indev=%u ", ifi);

        ifi = nfq_get_outdev(tb);
        if (ifi)
                printf("outdev=%u ", ifi);
        ifi = nfq_get_physindev(tb);
        if (ifi)
                printf("physindev=%u ", ifi);

        ifi = nfq_get_physoutdev(tb);
        if (ifi)
                printf("physoutdev=%u ", ifi);

        ret = nfq_get_payload(tb, &data);
        if (ret >= 0)
                printf("payload_len=%d ", ret);

        fputc('\n', stdout);

        return id;
}
        
u_int32_t MAX_LEN = 1500;
unsigned char * res_buf;

unsigned short net2host(unsigned short s)
{
    unsigned char * buf = (unsigned char *)&s;
    return ((buf[0] << 8) & 0xFF00) | (buf[1] & 0x00FF);
}

struct pseudo_header { 
    /*for computing TCP checksum, see TCP/IP Illustrated p. 145 */
    unsigned long s_addr;
    unsigned long d_addr;
    char zer0;
    unsigned char protocol;
    unsigned short length;
};

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data)
{
        u_int32_t id = print_pkt(nfa);
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
        struct nfq_handle *h;
        struct nfq_q_handle *qh;
        int fd;
        int rv;
        char buf[4096] __attribute__ ((aligned));

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
