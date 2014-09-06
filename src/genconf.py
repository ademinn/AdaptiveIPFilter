import json
import argparse
import probability
import coder


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('interface')
    parser.add_argument('-N', type=int, default=10)
    parser.add_argument('-M', type=int, default=10)
    parser.add_argument('-c', type=float, default=0.15)
    parser.add_argument('--conf', default='config.json')
    parser.add_argument('--mark', type=int, default=1)
    parser.add_argument('--queue', type=int, default=1)
    parser.add_argument('--msg')
    args = parser.parse_args()

    result = dict()

    result['interface'] = args.interface
    result['mark'] = args.mark
    result['queue'] = args.queue
    result['msg'] = [False, False, False] + coder.get_bits(args.msg) + [True, True, True]

    a = probability.calc(args.N, args.M, args.c)
    result['coeff'] = a
    with open(args.conf, 'w') as f:
        json.dump(result, f)
