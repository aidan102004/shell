#!/usr/bin/env python3
import sys

command = sys.argv[1] if len(sys.argv) > 1 else ""
curr_word = sys.argv[2] if len(sys.argv) > 2 else ""
prev_word = sys.argv[3] if len(sys.argv) > 3 else ""

candidates = ["start", "stop", "status", "restart", "reload"]

matches = [c for c in candidates if c.startswith(curr_word)]

for m in matches:
    print(m)