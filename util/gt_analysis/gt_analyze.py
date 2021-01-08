#!/usr/bin/env python3

import analyze
from analyze.lib import block_stats
from analyze.lib import compile_times

gt_analyze = analyze.analyze_all(
    compile_times.InstructionSchedulingTime,
    block_stats.NodesExamined,
    block_stats.NumEnumerated,
    block_stats.NumNotOptimalAndImproved,
    block_stats.NumNotOptimalNotImproved,
)

if __name__ == "__main__":
    analyze.main(gt_analyze)
