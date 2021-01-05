#!/usr/bin/env python3

import analyze
from analyze import Block


def _count(iter):
    try:
        return len(iter)
    except:
        return sum(1 for _ in iter)


class NodesExamined(analyze.Analyzer):
    '''
    Computes the number of nodes examined during enumeration
    '''

    def run_bench(self, args):
        logs = args[0]
        nodes_examined = sum(blk.single('NodeExamineCount')[
                             'num_nodes'] for blk in logs.all_blocks() if 'NodeExamineCount' in blk)

        self.stat(nodes_examined=nodes_examined)


class NumBlocks(analyze.Analyzer):
    '''
    Counts the total number of blocks in the benchmark logs
    '''

    def run_bench(self, args):
        logs = args[0]

        num_blocks = _count(logs.all_blocks())

        self.stat(num_blocks=num_blocks)


class NumEnumerated(analyze.Analyzer):
    '''
    Counts the number of blocks passed to the enumerator
    '''

    def run_bench(self, args):
        logs = args[0]

        self.stat(
            num_enumerated=_count(blk for blk in logs.all_blocks()
                                  if 'Enumerating' in blk)
        )


class NumOptimalAndImproved(analyze.Analyzer):
    '''
    Counts the number of blocks which were proved optimal by the enumerator
    after being improved
    '''

    def run_bench(self, args):
        logs = args[0]

        optimal = (blk for blk in logs.all_blocks()
                   if 'DagSolvedOptimally' in blk)
        optimal_and_improved = \
            (blk for blk in optimal
             if blk.single('DagSolvedOptimally')['cost_improvement'] > 0)

        self.stat(optimal_and_improved=_count(optimal_and_improved))


class NumOptimalNotImproved(analyze.Analyzer):
    '''
    Counts the number of blocks which were proved optimal by the enumerator
    but were not improved
    '''

    def run_bench(self, args):
        logs = args[0]

        optimal = (blk for blk in logs.all_blocks()
                   if 'DagSolvedOptimally' in blk)
        optimal_and_not_improved = \
            (blk for blk in optimal
             if blk.single('DagSolvedOptimally')['cost_improvement'] == 0)

        self.stat(optimal_and_not_improved=_count(optimal_and_not_improved))


class NumNotOptimalAndImproved(analyze.Analyzer):
    '''
    Counts the number of blocks which were not proved optimal by the enumerator
    but were improved
    '''

    def run_bench(self, args):
        logs = args[0]

        not_optimal = (blk for blk in logs.all_blocks()
                       if 'DagTimedOut' in blk)
        not_optimal_and_improved = \
            (blk for blk in not_optimal
             if blk.single('DagTimedOut')['cost_improvement'] > 0)

        self.stat(not_optimal_and_improved=_count(not_optimal_and_improved))


class NumNotOptimalNotImproved(analyze.Analyzer):
    '''
    Counts the number of blocks which were not proved optimal by the enumerator
    and were not improved
    '''

    def run_bench(self, args):
        logs = args[0]

        not_optimal = (blk for blk in logs.all_blocks()
                       if 'DagTimedOut' in blk)
        not_optimal_and_not_improved = \
            (blk for blk in not_optimal
             if blk.single('DagTimedOut')['cost_improvement'] == 0)

        self.stat(not_optimal_and_not_improved=_count(
            not_optimal_and_not_improved))


if __name__ == '__main__':
    analyze.main(analyze.analyze_all(
        NumBlocks,
        NumEnumerated,
        NumOptimalAndImproved,
        NumOptimalNotImproved,
        NumNotOptimalAndImproved,
        NumNotOptimalNotImproved,
        NodesExamined,
    ))
