#!/usr/bin/env python3

import analyze


class FindNegativeBlocks(analyze.Analyzer):
    '''
    Compare the number of nodes examined during enumeration (first - second); spit out which blocks are negative
    '''

    POSITIONAL = [
        ('first', ''),
        ('second', ''),
    ]

    def run_bench(self, args):
        first = args[0]
        second = args[1]

        logs = zip((blk for blk in first.all_blocks() if 'NodeExamineCount' in blk),
                   (blk for blk in second.all_blocks() if 'NodeExamineCount' in blk))
        logs = [(f.single('ProcessDag')['name'],
                 f.single('NodeExamineCount')['num_nodes'] - s.single('NodeExamineCount')['num_nodes'])
                for f, s in logs]
        logs = [(name, diff) for name, diff in logs if diff < 0]

        for name, diff in logs:
            self.stat(name, diff)


if __name__ == '__main__':
    analyze.main(FindNegativeBlocks)
