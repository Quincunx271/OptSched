#!/usr/bin/env python3

import re

import analyze
from analyze import Block, Logs


def _block_time(block: Block):
    end = block.single('ScheduleVerifiedSuccessfully')['time']
    start = block.single('ProcessDag')['time']
    return end - start


class InstructionSchedulingTime(analyze.Analyzer):
    '''
    Computes the time spent doing instruction scheduling.
    '''

    def run_bench(self, args):
        logs = args[0]
        time = sum(_block_time(blk) for blk in logs.all_blocks())

        self.stat(sched_time=time)


class TotalCompileTime(analyze.Analyzer):
    '''
    Computes the total time spent compiling. Only valid for spec benchmarks.
    '''

    def __init__(self, **options):
        if 'benchsuite' in options:
            assert options['benchsuite'] == 'spec', \
                'TotalCompileTime only supported for SPEC benchmarks'
        super().__init__(**options)

    def run(self, args):
        logs: Logs = args[0]

        last_logs = logs.benchmarks[-1].blocks[-1].raw_log
        m = re.search(r'(\d+) total seconds elapsed', last_logs)
        assert m, \
            'Logs must contain "total seconds elapsed" output by the SPEC benchmark suite'

        self.stat(total_time_secs=int(m.group(1)))


if __name__ == '__main__':
    class SwitchedTime(analyze.Analyzer):
        '''
        Computes the time spent compiling.
        '''

        OPTIONS = {
            'variant': 'Which variant of timing to use. Valid options: total, sched',
        }

        def __init__(self, *, variant, **options):
            super().__init__(**options)
            assert variant in ('total', 'sched')

            if variant == 'total':
                self._impl = TotalCompileTime(**options)
            else:
                self._impl = InstructionSchedulingTime(**options)

            self._impl.stats = self.stats

        def run(self, args):
            self._impl.run(args)

    analyze.main(SwitchedTime)
