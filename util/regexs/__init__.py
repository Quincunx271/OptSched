'''
A central location for log-parsing regexs.
'''

import re

USE_OPT_SCHED_SETTING = re.compile(r'\bUSE_OPT_SCHED\b')
NUM_SPILLS_FAST_RA = re.compile(r'Function: (?P<name>.*?)\nEND FAST RA: Number of spills: (?P<count>\d+)\n')
NUM_SPILLS_GREEDY_RA = re.compile(r'Function: (?P<name>.*?)\nGREEDY RA: Number of spilled live ranges: (?P<count>\d+)')
NUM_SPILLS_SIMULATED_RA = re.compile(r'Function: (?P<name>.*?)\nTotal Simulated Spills: (?P<count>\d+)')
NUM_SPILLS_WEIGHTED = re.compile(r'SC in Function (?P<name>.*?) (?P<count>-?\d+)')

ELAPSED_TIME = re.compile(r'(\d+) total seconds elapsed')
REGION_OPTSCHED_SPILLS = re.compile(r"OPT_SCHED LOCAL RA: DAG Name: (?P<name>\S+) Number of spills: (?P<count>\d+) \(Time")

FUNC_NAME = re.compile(r'Function: (?P<name>.*?)\n')
CALL_BOUNDARY_STORES = re.compile(r'Call Boundary Stores in function: (?P<count>\d+)\n')
BLOCK_BOUNDARY_STORES = re.compile(r'Block Boundary Stores in function: (?P<count>\d+)\n')
LIVE_IN_LOADS = re.compile(r'Live-In Loads in function: (?P<count>\d+)\n')

NUM_EXAMINED = re.compile(r'Examined (?P<count>\d+) nodes')

NEW_BENCH = re.compile(r'(\d+)\.(.*) base \.exe default')

OPT_INFO = re.compile(r'OptSchPeakRegPres Index (?P<index>\d+) Name (?P<name>.+) Peak (?P<peak>\d+) Limit (?P<limit>\d+)')
AFTER_INFO = re.compile(r'PeakRegPresAfter  Index (?P<index>\d+) Name (?P<name>.+) Peak (?P<peak>\d+) Limit (?P<limit>\d+)')
