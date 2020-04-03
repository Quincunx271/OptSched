'''
Block-based log-parsing regexs
'''

import re

BLOCK_DELIMITER = re.compile(r'INFO: \*{4,}? Opt Scheduling \*{4,}?')

DAG_NAME_SIZE_LATENCY = re.compile(r'Processing DAG (?P<name>.*) with (?P<size>\d+) insts and max latency (?P<latency>\d+)')

IS_NOT_ENUMERATED = re.compile(r'The list schedule .* is optimal')
IS_ZERO_TIME_LIMIT = re.compile(r'Bypassing optimal scheduling due to zero time limit')
IS_ENUMERATED_OPTIMAL = re.compile(r'DAG solved optimally')
IS_LIST_FAILED = re.compile(r'List scheduling failed')
IS_RP_MISMATCH = re.compile(r'RP-mismatch falling back!')
IS_OPTIMAL = re.compile(r'The schedule is optimal')

COST_LOWER_BOUND = re.compile(r'Lower bound of cost before scheduling: (\d+)')
COST_BEST = re.compile(r'Best schedule for DAG (?P<name>.*) has cost (?P<cost>\d+) and length (?P<length>\d+). The schedule is (?P<optimal>not optimal|optimal)')
COST_HEURISTIC = re.compile(r'list schedule is of length (?P<length>\d+) and spill cost (?P<spill_cost>\d+). Tot cost = (?P<total_cost>\d+)')
COST_IMPROVEMENT = re.compile(r'cost imp=(?P<improvement>\d+)')

SPILLS_BEST = re.compile(r'OPT_SCHED LOCAL RA: DAG Name: (?P<name>\S+) Number of spills: (?P<count>\d+)')
SPILLS_HEURISTIC= re.compile(r'OPT_SCHED LOCAL RA: DAG Name: (?P<name>.*) \*\*\*heuristic_schedule\*\*\* Number of spills: (?P<count>\d+)')

START_TIME = re.compile(r'-{20} \(Time = (?P<time>\d+) ms\)')
END_TIME = re.compile(r'verified successfully \(Time = (?P<time>\d+) ms\)')
PEAK_REG_PRESSURE = re.compile(r'PeakRegPresAfter Index (?P<index>\d+) Name (?P<name>.*) Peak (?P<peak>\d+) Limit (?P<limit>\d+)')
PEAK_REG_BLOCK_NAME = re.compile(r'LLVM max pressure after scheduling for BB (?P<name>\S+)')
