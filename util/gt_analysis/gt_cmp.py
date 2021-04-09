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


def _cost(blk):
    return blk.single('BestResult')['cost'] + blk.single('CostLowerBound')['cost']


def _is_improved(before, after):
    return _cost(before) > _cost(after)


def _is_timed_out(blk):
    return 'DagTimedOut' in blk


def _cost_improvement(before, after):
    return _cost(before) - _cost(after)


def _unary_pred(pred):
    return lambda a, b: pred(a) and pred(b)


def _dual_filter(first, second, pred):
    joint_p = [(f, s) for f, s in zip(first.all_blocks(), second.all_blocks()) if pred(f, s)]

    first_p = set(f.uniqueid() for f, _ in joint_p)
    second_p = set(s.uniqueid() for _, s in joint_p)
    both_p = first_p & second_p

    first = first.keep_blocks_if(lambda blk: blk.uniqueid() in both_p)
    second = second.keep_blocks_if(lambda blk: blk.uniqueid() in both_p)

    return first, second

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


        nogt_timeout, gt_timeout = _dual_filter(nogt, gt, _unary_pred(_is_timed_out))
        nogt_to_imp, gt_to_imp = _dual_filter(nogt_timeout, gt_timeout, _is_improved)
        nogt_to_worse, gt_to_worse = _dual_filter(nogt_timeout, gt_timeout, lambda a, b: _cost(a) < _cost(b))
        timeout_imp = sum(_cost_improvement(before, after) for before, after in zip(nogt_to_imp.all_blocks(), gt_to_imp.all_blocks()))
        timeout_imp_baseline = sum(_cost(before) for before in nogt_to_imp.all_blocks())
        timeout_worse = sum(_cost_improvement(before, after) for before, after in zip(nogt_to_worse.all_blocks(), gt_to_worse.all_blocks()))


        nogt, gt = _dual_filter(nogt, gt, _unary_pred(lambda blk: 'DagSolvedOptimally' in blk or 'HeuristicScheduleOptimal' in blk))

        self.stat(
            nogt_sched_time=sched_time.calc(nogt.all_blocks()),
            gt_sched_time=sched_time.calc(gt.all_blocks()),
            nogt_sched_time_enumerated=sched_time.calc([blk for blk in nogt.all_blocks() if 'Enumerating' in blk]),
            gt_sched_time_enumerated=sched_time.calc([blk for blk in gt.all_blocks() if 'Enumerating' in blk]),
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
            num_timed_out=len(list(nogt_timeout.all_blocks())),
            num_timed_out_improved=len(list(nogt_to_imp.all_blocks())),
            num_timed_out_worse=len(list(nogt_to_worse.all_blocks())),
            timeout_improvement=timeout_imp,
            timeout_imp_baseline=timeout_imp_baseline,
            timeout_worsen=timeout_worse,
        )


if __name__ == "__main__":
    analyze.main(GtCmp)
