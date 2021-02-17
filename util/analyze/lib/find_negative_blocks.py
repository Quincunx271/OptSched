#!/usr/bin/env python3

import analyze


def blocks_enumerated_optimally(blocks):
    return [blk for blk in blocks if 'DagSolvedOptimally' in blk and 'NodesExamined' in blk]


class NegativeBlocks(analyze.Analyzer):
    POSITIONAL = {
        'first': 'The base to compare against',
        'second': 'Report any blocks here which took more nodes to examine',
    }.items()

    OPTIONS = {
        'absolute-threshold': {'help': 'Ignore any blocks with a delta < threshold', 'default': 0},
        'percent-threshold': {'help': 'Ignore any blocks with a %%-difference < threshold', 'default': 0},
    }

    def __init__(self, *, absolute_threshold, percent_threshold, **options):
        super().__init__(**options)
        self.absolute_threshold = absolute_threshold
        self.percent_threshold = percent_threshold

    def run(self, args):
        first = args[0]
        second = args[1]

        self.stats = []

        first_opt = set(blk.uniqueid()
                        for blk in blocks_enumerated_optimally(first.all_blocks()))
        second_opt = set(blk.uniqueid()
                         for blk in blocks_enumerated_optimally(second.all_blocks()))
        both_opt = first_opt & second_opt

        first = first.keep_blocks_if(lambda blk: blk.uniqueid() in both_opt)
        second = second.keep_blocks_if(lambda blk: blk.uniqueid() in both_opt)

        problems = []

        for f, s in zip(first.all_blocks(), second.all_blocks()):
            if f.name != s.name:
                raise AssertionError(f'{f} and {s} are not the same block')
            if 'NodesExamined' not in s or 'NodesExamined' not in f:
                raise AssertionError(
                    f'NodesExamined event not in blocks {f} and {s}:\n' + f.raw_log + '\n' + s.raw_log)

            if f.single('NodesExamined')['num'] < s.single('NodesExamined'):
                problems.append((f, s))

        for f, s in problems:
            fnodesx = f.single('NodesExamined')['num']
            snodesx = s.single('NodesExamined')['num']
            diff = fnodesx - snodesx
            percent_diff = diff / fnodesx
            if abs(diff) < self.absolute_threshold:
                continue
            if abs(percent_diff) < self.percent_threshold:
                continue
            self.stats.append({
                'benchmark': f.info['benchmark'],
                'block': f.name,
                'first_nodes_examined': fnodesx,
                'second_nodes_examined': snodesx,
                'delta': diff,
                '%-delta': percent_diff,
            })

        if not self.stats:
            self.stats.append({'no_negative_cases_found': True})


if __name__ == "__main__":
    analyze.main(NegativeBlocks)
