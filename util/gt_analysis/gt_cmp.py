#!/usr/bin/env python3

import analyze
from analyze.lib import block_stats
from analyze.lib import compile_times


def blocks_enumerated_optimally(blocks):
    return [blk for blk in blocks if 'DagSolvedOptimally' in blk or 'HeuristicScheduleOptimal' in blk]


class GtCmp(analyze.Analyzer):
    POSITIONAL = {
        'nogt': 'Logs without Graph Transformations',
        'gt': 'Logs with Graph Transformations'
    }.items()

    def run_bench(self, args):
        nogt = args[0]
        gt = args[1]

        nodesx = block_stats.NodesExamined()

        nogt_opt = set(blk.uniqueid()
                       for blk in blocks_enumerated_optimally(nogt.all_blocks()))
        gt_opt = set(blk.uniqueid()
                     for blk in blocks_enumerated_optimally(gt.all_blocks()))
        both_opt = nogt_opt & gt_opt

        nogt = nogt.keep_blocks_if(lambda blk: blk.uniqueid() in both_opt)
        gt = gt.keep_blocks_if(lambda blk: blk.uniqueid() in both_opt)

        self.stat(
            nogt_nodes_examined=nodesx.calc(nogt.all_blocks()),
            gt_nodes_examined=nodesx.calc(gt.all_blocks()),
        )


if __name__ == "__main__":
    analyze.main(GtCmp)
