from bitarray import bitarray
from datetime import datetime, timedelta

def decode():
    with open() as f:
        timestamps = []
        start = [0, 0, 0]
        end = [1, 1, 1]
        delta = timedelta(seconds=1)
        for line in f:
            ts = line.split(" ", 1)[0]
            ts = datetime.strptime(ts, '%H:%M:%S.%f')
            timestamps.append(ts)
        bits = [int(t2 - t1 > delta) for t2, t1 in zip(timestamps[1:], timestamps[:-1])]
        bits = extract_message(bits, start, end)
        print get_message(bits)

def find_index(list, sublist):
    print('Find {} in {}'.format(sublist, list))
    for i in range(len(list) - len(sublist) + 1):
        if list[i:i+len(sublist)] == sublist:
            return i
    return None

def extract_message(bits, start, end):
    start_index = find_index(bits, start) + len(start)
    end_index = find_index(bits[start_index:], end)
    return bits[start_index:start_index + end_index]

def get_message(bits):
    return bitarray(bits).tostring()

def get_bits(msg):
    ba = bitarray.bitarray()
    ba.fromstring(msg)
    return ba.tolist()
