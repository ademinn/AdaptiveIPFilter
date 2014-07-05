import ctypes
import socket
import bindings.libnetfilter_queue as lq


BUF_SIZE = 4096


def callback(query, _, nfq_data, d):
    print('callback')
    ptr_packet = ctypes.c_void_p(0)
    data_len = lq.nfq_get_payload(nfq_data, ctypes.byref(ptr_packet))
    data = ctypes.string_at(ptr_packet, data_len)
    packet_id = lq.nfq_get_packet_id(nfq_data)
    print(data_len)
    if data_len == 1500:
        return lq.nfq_set_verdict(query, packet_id, data_len, data)
    else:
        return lq.nfq_set_verdict(query, packet_id, 1500, data)


if __name__ == '__main__':
    handle = lq.nfq_open()
    lq.nfq_bind(handle, socket.AF_INET)
    qh = lq.nfq_create_queue(handle, 0, callback)
    fd = lq.nfq_fd(handle)
    sock = socket.fromfd(fd, 0, 0)
    data = sock.recv(BUF_SIZE)
    while data:
        lq.nfq_handle_packet(handle, data, BUF_SIZE)
        data = sock.recv(BUF_SIZE)
    sock.close()
    lq.nfq_destroy_queue(qh)
    lq.nfq_unbind(handle, socket.AF_INET)
    lq.nfq_close(handle)