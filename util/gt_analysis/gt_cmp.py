#!/usr/bin/env python3

import argparse

import analyze
from analyze import Block, Logs, utils
from analyze.lib import block_stats, compile_times

sched_time = compile_times.sched_time


def blocks_enumerated_optimally(blocks):
    return [blk for blk in blocks if 'DagSolvedOptimally' in blk or 'HeuristicScheduleOptimal' in blk]


def rp_ilp_gt_elapsed_for_blk(blk: Block) -> int:
    if 'GraphTransOccupancyPreservingILPNodeSuperiority' not in blk:
        return 0
    return blk.single('GraphTransOccupancyPreservingILPNodeSuperiorityFinished')['time'] \
        - blk.single('GraphTransOccupancyPreservingILPNodeSuperiority')['time']


def rp_only_gt_elapsed_for_blk(blk: Block) -> int:
    if 'GraphTransRPNodeSuperiority' not in blk:
        return 0
    return blk.single('GraphTransRPNodeSuperiorityFinished')['time'] \
        - blk.single('GraphTransRPNodeSuperiority')['time']


def ilp_only_gt_elapsed_for_blk(blk: Block) -> int:
    if 'GraphTransILPNodeSuperiority' not in blk:
        return 0
    return blk.single('GraphTransILPNodeSuperiorityFinished')['time'] \
        - blk.single('GraphTransILPNodeSuperiority')['time']


def raw_gt_elapsed_for_blk(blk: Block) -> int:
    return rp_ilp_gt_elapsed_for_blk(blk) \
        + rp_only_gt_elapsed_for_blk(blk) \
        + ilp_only_gt_elapsed_for_blk(blk)


def total_gt_elapsed_for_blk(blk: Block) -> int:
    if 'GraphTransformationsStart' not in blk:
        return 0
    return blk.single('GraphTransformationsFinished')['time'] \
        - blk.single('GraphTransformationsStart')['time']


def elapsed_before_enumeration_for_blk(blk: Block) -> int:
    assert 'CostLowerBound' in blk
    return blk['CostLowerBound'][-1]['time']


def enum_time_for_blk(blk: Block) -> int:
    if 'DagSolvedOptimally' not in blk:
        return 0
    return blk.single('DagSolvedOptimally')['time'] - blk['Enumerating'][0]['time']


def cost_for_blk(blk: Block) -> int:
    return blk.single('BestResult')['cost'] + blk['CostLowerBound'][-1]['cost']


def is_improved(before: Block, after: Block):
    return cost_for_blk(before) > cost_for_blk(after)


def compute_stats(nogt: Logs, gt: Logs):
    TOTAL_BLOCKS = utils.count(nogt)

    nogt, gt = utils.zipped_keep_blocks_if(
        nogt, gt, pred=block_stats.is_enumerated)

    result = {
        'Total Blocks in Benchsuite': TOTAL_BLOCKS,
        'Num Blocks with GT applied': utils.count(nogt),
        'Num Nodes Examined (opt. blocks only) (No GT)': utils.sum_stat_for_all(block_stats.nodes_examined_for_blk, nogt),
        'Num Nodes Examined (opt. blocks only) (GT)': utils.sum_stat_for_all(block_stats.nodes_examined_for_blk, gt),

        'Total Sched Time (No GT)': sched_time(nogt),
        'Total Sched Time (GT)': sched_time(gt),
        'Enum Time (No GT)': utils.sum_stat_for_all(enum_time_for_blk, nogt),
        'Enum Time (GT)': utils.sum_stat_for_all(enum_time_for_blk, gt),
        'Total GT Time': utils.sum_stat_for_all(total_gt_elapsed_for_blk, gt),
        'Block Cost - Relative (No GT)': utils.sum_stat_for_all(block_stats.block_relative_cost, nogt),
        'Block Cost - Relative (GT)': utils.sum_stat_for_all(block_stats.block_relative_cost, gt),
        'Block Cost (No GT)': utils.sum_stat_for_all(block_stats.block_cost, nogt),
        'Block Cost (GT)': utils.sum_stat_for_all(block_stats.block_cost, gt),

        'Num Timeout Unimproved (No GT)': utils.count(blk for blk in nogt
                                                      if block_stats.is_timed_out(blk)
                                                      and not block_stats.is_improved(blk)),
        'Num Timeout Unimproved (GT)': utils.count(blk for blk in gt
                                                   if block_stats.is_timed_out(blk)
                                                   and not block_stats.is_improved(blk)),
        'Num Timeout Improved (No GT)': utils.count(blk for blk in nogt
                                                    if block_stats.is_timed_out(blk)
                                                    and block_stats.is_improved(blk)),
        'Num Timeout Improved (GT)': utils.count(blk for blk in gt
                                                 if block_stats.is_timed_out(blk)
                                                 and block_stats.is_improved(blk)),
    }

    return result


if __name__ == "__main__":
    import sys
    import csv

    parser = argparse.ArgumentParser()
    parser.add_argument('nogt')
    parser.add_argument('gt')
    args = analyze.parse_args(parser, 'nogt', 'gt')

    results = utils.foreach_bench(compute_stats, args.nogt, args.gt)

    writer = csv.DictWriter(sys.stdout,
                            fieldnames=['Benchmark'] + list(results['Total'].keys()))
    writer.writeheader()
    for bench, bench_res in results.items():
        writer.writerow({'Benchmark': bench, **bench_res})
