import json
import argparse
import probability
import coder


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--interface')
    parser.add_argument('--conf')
    parser.add_argument('--mark', type=int)
    parser.add_argument('--queue-num', dest='queue_num', type=int)

    subparsers = parser.add_subparsers(dest='conf_type')

    filter_parser = subparsers.add_parser('filter')
    filter_parser.add_argument('-N', type=int, default=10)
    filter_parser.add_argument('-M', type=int, default=100)
    filter_parser.add_argument('-c', type=float, default=0.15)
    filter_parser.add_argument('--delay', type=int)

    channel_parser = subparsers.add_parser('channel')
    channel_parser.add_argument('--zero-delay', dest='zero_delay', type=int)
    channel_parser.add_argument('--one-delay', dest='one_delay', type=int)
    channel_parser.add_argument('--msg')
    channel_parser.add_argument('--path')
    channel_parser.add_argument('--warm', type=int, default=10)

    args = parser.parse_args()

    result = vars(args)

    if args.conf_type == 'filter':
        a = probability.calc(args.N, args.M, args.c)
        result['coeff'] = a
    else:
        if args.msg:
            msg = args.msg
        else:
            with open(args.path, 'r') as f:
                msg = f.read()
        result['msg'] = coder.encode(msg)

    with open(args.conf, 'w') as f:
        json.dump(result, f)
