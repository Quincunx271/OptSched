#!/usr/bin/env python3

import analyze
from analyze.lib import block_stats
from analyze.lib import compile_times

# run(compile_times.TotalCompileTime, _0)
analyze.main(
    analyze.analyze_all(
        compile_times.InstructionSchedulingTime,
        block_stats.NodesExamined,
        block_stats.NumEnumerated,
        block_stats.NumNotOptimalAndImproved,
        block_stats.NumNotOptimalNotImproved,
    )
)
