#!/usr/bin/env python3

import analyze
from analyze.lib import block_stats
from analyze.lib import compile_times


def blocks_enumerated_optimally(blocks):
    return [blk for blk in blocks if 'DagSolvedOptimally' in blk or 'HeuristicScheduleOptimal' in blk]


def _rp_ilp_gt_elapsed(blk):
    if 'GraphTransOccupancyPreservingILPNodeSuperiority' not in blk:
        return 0
    return blk.single('GraphTransOccupancyPreservingILPNodeSuperiorityFinished')['time'] \
        - blk.single('GraphTransOccupancyPreservingILPNodeSuperiority')['time']


def _elapsed_before_enumeration(blk):
    assert 'CostLowerBound' in blk
    return blk.single('CostLowerBound')['time']


def _total_gt_elapsed(blk):
    if 'GraphTransformationsStart' not in blk:
        return 0
    return blk.single('GraphTransformationsFinished')['time'] \
        - blk.single('GraphTransformationsStart')['time']


def _enum_time(blk):
    if 'DagSolvedOptimally' not in blk:
        return 0
    return blk.single('DagSolvedOptimally')['time'] - blk['Enumerating'][0]['time']


class RawCombinedRpIlpGtTime(analyze.Analyzer):
    def run_bench(self, args):
        logs = args[0]

        self.stat(combined_rp_ilp_gt_time=self.calc(logs.all_blocks()))

    def calc(self, blocks):
        return sum(_rp_ilp_gt_elapsed(blk) for blk in blocks)


class TotalGtTime(analyze.Analyzer):
    def run_bench(self, args):
        logs = args[0]

        self.stat(total_gt_time=self.calc(logs.all_blocks()))

    def calc(self, blocks):
        return sum(_total_gt_elapsed(blk) for blk in blocks)


class GtCmp(analyze.Analyzer):
    POSITIONAL = {
        'nogt': 'Logs without Graph Transformations',
        'gt': 'Logs with Graph Transformations'
    }.items()

    def run_bench(self, args):
        nogt = args[0]
        gt = args[1]

        nodesx = block_stats.NodesExamined()
        sched_time = compile_times.InstructionSchedulingTime()
        num_enum = block_stats.NumEnumerated()
        num_not_opt_imp = block_stats.NumNotOptimalAndImproved()
        num_not_opt_not_imp = block_stats.NumNotOptimalNotImproved()
        total_gt_time = TotalGtTime()

        nogt_opt = set(blk.uniqueid()
                       for blk in blocks_enumerated_optimally(nogt.all_blocks()))
        gt_opt = set(blk.uniqueid()
                     for blk in blocks_enumerated_optimally(gt.all_blocks()))
        both_opt = nogt_opt & gt_opt

        nogt = nogt.keep_blocks_if(lambda blk: blk.uniqueid() in both_opt)
        gt = gt.keep_blocks_if(lambda blk: blk.uniqueid() in both_opt)

        self.stat(
            nogt_sched_time=sched_time.calc(nogt.all_blocks()),
            gt_sched_time=sched_time.calc(gt.all_blocks()),
            nogt_enum_time = sum(_enum_time(blk) for blk in nogt.all_blocks()),
            gt_enum_time = sum(_enum_time(blk) for blk in gt.all_blocks()),
            nogt_nodes_examined=nodesx.calc(nogt.all_blocks()),
            gt_nodes_examined=nodesx.calc(gt.all_blocks()),
            nogt_num_enumerated=num_enum.calc(nogt.all_blocks()),
            gt_num_enumerated=num_enum.calc(gt.all_blocks()),
            nogt_num_not_optimal_but_improved=num_not_opt_imp.calc(
                nogt.all_blocks()),
            gt_num_not_optimal_but_improved=num_not_opt_imp.calc(
                gt.all_blocks()),
            nogt_num_not_optimal_and_not_improved=num_not_opt_not_imp.calc(
                nogt.all_blocks()),
            gt_num_not_optimal_and_not_improved=num_not_opt_not_imp.calc(
                gt.all_blocks()),
            total_gt_time=total_gt_time.calc(gt.all_blocks()),
        )


if __name__ == "__main__":
    analyze.main(GtCmp)
