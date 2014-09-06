from bitarray import bitarray
from datetime import datetime, timedelta

def decode(path):
    with open(path) as f:
        timestamps = []
        start = [0, 0, 0]
        end = [1, 1, 1]
        zero_min = timedelta(milliseconds=50)
        zero_max = timedelta(milliseconds=150)
        one_min = timedelta(milliseconds=450)
        one_max = timedelta(milliseconds=550)
        for line in f:
            ts = line.split(" ", 1)[0]
            ts = datetime.strptime(ts, '%H:%M:%S.%f')
            timestamps.append(ts)
        def map_delta(delta):
            print delta.microseconds / 1000
            if zero_min < delta < zero_max:
                return 0
            elif one_min < delta < one_max:
                return 1
            else:
                return 2
        bits = [map_delta(t2 - t1) for t2, t1 in zip(timestamps[1:], timestamps[:-1])]
        bits = extract_message(bits, start, end)
        print len(bits)
        print bits
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
    return bits[start_index:start_index + end_index - 1]

def get_message(bits):
    return bitarray(bits).tostring()

def get_bits(msg):
    ba = bitarray()
    ba.fromstring(msg)
    return ba.tolist()


if __name__ == '__main__':
    decode('/home/ademinn/temp/coded.txt')
