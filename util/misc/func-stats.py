#!/bin/python3
# Find the number of functions that are compiled more than once by LLVM.

import os, sys
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import regexs
from regexs import block as block_re

if __name__ == "__main__":
    with open(sys.argv[1]) as logfile:
        blocks = {}
        bench = None
        totalRepeats = 0
        totalMismatches = 0
        for line in logfile.readlines():
            matchBench = regexs.NEW_BENCH.findall(line)
            matchBlock = block_re.DAG_NAME_SIZE_LATENCY.findall(line)

            if matchBench:
                if bench:
                    print('In bench ' + bench + '  found ' + str(totalRepeats) + ' repeat blocks and ' + str(totalMismatches) + ' mismatches in length.')
                funcs = {}
                totalRepeats = 0
                totalMismatches = 0
                bench = matchBench[0][1]

            if matchBlock:
                name = matchBlock[0]['name']
                insts = matchBlock[0]['size']

                if name in blocks:
                    if blocks[name] != insts:
                        totalMismatches += 1

                    totalRepeats += 1
                    continue
                else:
                    blocks[name] = insts
