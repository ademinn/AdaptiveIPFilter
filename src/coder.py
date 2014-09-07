import argparse
import sys
import reedsolo
from bitarray import bitarray
from datetime import datetime, timedelta

RS = 10
START = 'START'
START_LEN = 15 * 8
LEN = 12 * 8

def decode(path, zero_delay, one_delay):
    with open(path) as f:
        timestamps = []
        start = [1 if x else 0 for x in get_bits('START').tolist()]
        middle = timedelta(milliseconds=(zero_delay + one_delay) / 2)
        #zero_min = timedelta(milliseconds=zero_delay - delta)
        #zero_max = timedelta(milliseconds=zero_delay + delta)
        #one_min = timedelta(milliseconds=one_delay - delta)
        #one_max = timedelta(milliseconds=one_delay + delta)
        for line in f:
            ts = line.split(" ", 1)[0]
            ts = datetime.strptime(ts, '%H:%M:%S.%f')
            timestamps.append(ts)
        def map_delta(delta):
            print delta.microseconds / 1000
            return delta > middle
            # print delta.microseconds / 1000
            #s = str(delta.microseconds / 1000) + " -> %d"
            #if zero_min < delta < zero_max:
            #    print s % 0
            #    return 0
            #elif one_min < delta < one_max:
            #    print s % 1
            #    return 1
            #else:
            #    print s % 2
            #    return 2
        bits = [map_delta(t2 - t1) for t2, t1 in zip(timestamps[1:], timestamps[:-1])]
        bits = bits[9:]
        print extract_message(bitarray(bits))

def decode_str(bits):
    rs = reedsolo.RSCodec(RS)
    return rs.decode(bytearray(bits.tobytes()))

def decode_int(bits):
    rs = reedsolo.RSCodec(RS)
    return int(bitarray(str(decode_str(bits))).to01(), 2)

def find_index(bits):
    for i in range(len(bits) - START_LEN + 1):
        try:
            s = decode_str(bits[i:i+START_LEN])
            print "%d -> %s" % (i, s)
            if s == 'START':
                return i
        except Exception, e:
            print "%d -> %s" % (i, str(e))
            pass
        # if list[i:i+len(sublist)] == sublist:
        #     return i
    return None

def extract_message(bits):
    print bits
    start_index = find_index(bits)
    len_index = start_index + START_LEN
    length = decode_int(bits[len_index:len_index+LEN]) * 8
    msg_index = len_index + LEN
    end_index = msg_index + length
    return decode_str(bits[msg_index:end_index])

def get_message(bits):
    return bitarray(bits).tostring()

def get_bits(msg):
    rs = reedsolo.RSCodec(RS)
    e = rs.encode(msg)
    ba = bitarray()
    ba.frombytes(str(e))
    return ba

def get_bits_int(i):
    rs = reedsolo.RSCodec(RS)
    b1 = bitarray('{0:016b}'.format(i))
    e = rs.encode(b1.tobytes())
    b2 = bitarray()
    b2.frombytes(str(e))
    return b2

def encode(msg):
    b1 = get_bits('START')
    b3 = get_bits(msg)
    b2 = get_bits_int(len(b3) / 8)
    return (b1 + b2 + b3).tolist()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('dump')
    parser.add_argument('zero_delay', type=int)
    parser.add_argument('one_delay', type=int)
    args = parser.parse_args()
    decode(args.dump, args.zero_delay, args.one_delay)
