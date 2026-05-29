#!/usr/bin/env python3

import sys


def write_response(success, text):
    data = text.encode('utf-8')
    header = ('%c%010d' % (success, len(data))).encode('ascii')
    sys.stdout.buffer.write(header)
    sys.stdout.buffer.write(data)
    sys.stdout.buffer.flush()


while True:
    header = sys.stdin.readline()
    if header == "exit\n":
        break

    assert header == "<expr>\n"

    expr = []
    while True:
        line = sys.stdin.readline()
        if line == "</expr>\n":
            break
        else:
            expr.append(line)

    expr = ''.join(expr)

    try:
        result = str(eval(expr))
        write_response('S', result)
    except:
        msg = str(sys.exc_info()[1])
        write_response('F', msg)
