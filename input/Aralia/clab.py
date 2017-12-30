#!/usr/bin/env python3

import os
import re
import sys

_NEG_REGEX = re.compile(r"\(!(\w+)\)")

def parse_input(line):
    if not line:
        return
    if line.startswith("variable") or line.startswith("rule"):
        return
    if line.startswith("bool"):
        assert line.endswith(";")
        line = line[5:-1]
        args = line.split(", ")
        args = [x for x in args if x.startswith("e")]
        for event in args:
            print("p(" + event + ")=0.01")
    elif line.startswith("("):
        assert line.endswith(");")
        line = line[1:-2]
        assert "==" in line
        line = line.replace("==", ":=", 1)
        if "!" in line:
            line = _NEG_REGEX.sub(r"~\1", line)

        if "||" in line:
            print(line.replace("||", "|"))
        elif "&&" in line:
            print(line.replace("&&", "&"))
        elif "+" in line:
            line = line.replace("+", ",")
            line = line.replace("(", "@(", 1)
            line = line.replace("==", ",", 1)
            line = line.replace(" ", "")
            line = line.replace("))", "])")
            line = line.replace(",(", ",[", 1)
            print(line)
        else:
            print(line)
    else:
        assert False


def main():
    name = os.path.basename(sys.argv[1])
    name = os.path.splitext(name)[0]
    assert name
    print(name)
    with open(sys.argv[1], "r") as clab_file:
        for line in clab_file:
            try:
                parse_input(line.strip())
            except AssertionError as err:
                print(line, file=sys.stderr)
                raise err


if __name__ == "__main__":
    main()
