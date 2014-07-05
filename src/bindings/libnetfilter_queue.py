import ctypes
import socket

__NFQNL_COPY_PACKET = 2
__NF_ACCEPT = 1
__LIBNAME = 'libnetfilter_queue.so'
__LIB = ctypes.CDLL(__LIBNAME)


#class nfq_handle(ctypes.Structure):
#    pass
#

class NFQNLMessagePacketHeader(ctypes.Structure):
    _fields_ = [
        ('packet_id', ctypes.c_uint32),
        ('hw_protocol', ctypes.c_uint16),
        ('hook', ctypes.c_uint8)
    ]


__LIB.nfq_open.restype = ctypes.c_void_p

__LIB.nfq_close.restype = ctypes.c_int
__LIB.nfq_close.argtypes = (ctypes.c_void_p, )

__LIB.nfq_bind_pf.restype = ctypes.c_int
__LIB.nfq_bind_pf.argtypes = ctypes.c_void_p, ctypes.c_int16

__LIB.nfq_unbind_pf.restype = ctypes.c_int
__LIB.nfq_unbind_pf.argtypes = ctypes.c_void_p, ctypes.c_int16

__LIB.nfq_create_queue.restype = ctypes.c_void_p
__LIB.nfq_create_queue.argtypes = ctypes.c_void_p, ctypes.c_uint16, ctypes.c_void_p, ctypes.c_void_p

__LIB.nfq_set_mode.restype = ctypes.c_int
__LIB.nfq_set_mode.argtypes = ctypes.c_void_p, ctypes.c_int8, ctypes.c_int32

__LIB.nfq_destroy_queue.restype = ctypes.c_int
__LIB.nfq_destroy_queue.argtypes = (ctypes.c_void_p, )

__LIB.nfq_fd.restype = ctypes.c_int
__LIB.nfq_fd.argtypes = (ctypes.c_void_p, )

__LIB.nfq_handle_packet.restype = ctypes.c_int
__LIB.nfq_handle_packet.argtypes = ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int


__LIB.nfq_get_msg_packet_hdr.restype = ctypes.POINTER(NFQNLMessagePacketHeader)
__LIB.nfq_get_msg_packet_hdr.argtypes = (ctypes.c_void_p, )

__LIB.nfq_get_payload.restype = ctypes.c_int
__LIB.nfq_get_payload.argtypes = ctypes.c_void_p, ctypes.c_void_p

__LIB.nfq_set_verdict.restype = ctypes.c_int
__LIB.nfq_set_verdict.argtypes = ctypes.c_void_p, ctypes.c_int32, ctypes.c_int32, ctypes.c_int32, ctypes.c_void_p


def nfq_open():
    print('nfq_open')
    return __LIB.nfq_open()


def nfq_close(handle):
    print('nfq_close')
    return __LIB.nfq_close(handle)


def nfq_bind(handle, protocol_family):
    print('nfq_bind')
    return __LIB.nfq_bind_pf(handle, protocol_family)


def nfq_unbind(handle, protocol_family):
    print('nfq_unbind')
    return __LIB.nfq_unbind_pf(handle, protocol_family)


def nfq_create_queue(handle, num, callback):
    print('nfq_create_queue')
    f = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p)
    q = __LIB.nfq_create_queue(handle, num, f(callback), None)
    print(str(q))
    print('queue created')
    __LIB.nfq_set_mode(q, __NFQNL_COPY_PACKET, 0xFFFF)
    print('set mode')
    return q


def nfq_destroy_queue(queue):
    print('nfq_destroy_queue')
    return __LIB.nfq_destroy_queue(queue)


def nfq_fd(handle):
    print('nfq_fd')
    return __LIB.nfq_fd(handle)


def nfq_handle_packet(handle, buf, data_len):
    print('handle_packet')
    return __LIB.nfq_handle_packet(handle, buf, data_len)


def nfq_get_packet_id(nfq_data):
    print('get_packet_id')
    packet_id = 0
    packet_header = __LIB.nfq_get_msg_packet_hdr(nfq_data)
    if packet_header:
        packet_id = socket.ntohl(packet_header.contents.packet_id)
    return packet_id


def nfq_get_payload(data, buf_ptr):
    print('get_payload')
    return __LIB.nfq_get_payload(data, buf_ptr)


def nfq_set_verdict(query, packet_id, data_len, buf):
    print('set_verdict')
    return __LIB.nfq_set_verdict(query, packet_id, __NF_ACCEPT, data_len, buf)
