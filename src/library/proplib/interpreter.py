#!/usr/bin/env python3

import sys


def write_response(success, text):
    data = text.encode('utf-8')
    header = ('%c%010d' % (success, len(data))).encode('ascii')
    sys.stdout.buffer.write(header)
    sys.stdout.buffer.write(data)
    sys.stdout.buffer.flush()


def read_line():
    # Windows pipe text mode may turn '\n' into '\r\n'; strip both ends.
    return sys.stdin.readline().rstrip('\r\n')


while True:
    header = read_line()
    if header == "exit":
        break

    assert header == "<expr>", repr(header)

    expr = []
    while True:
        line = read_line()
        if line == "</expr>":
            break
        else:
            expr.append(line + '\n')

    expr = ''.join(expr)

    try:
        result = str(eval(expr))
        write_response('S', result)
    except:
        msg = str(sys.exc_info()[1])
        write_response('F', msg)
