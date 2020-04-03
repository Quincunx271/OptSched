'''
SLIL-specific log-parsing regexs
'''
import re

ABSOLUTE_COST = re.compile(r'Dag (?P<name>.*?) (?P<result>.*?) absolute cost (?P<cost>\d+?) time (?P<time>\d+)')
PEAK = re.compile(r'DAG (?P<name>.*?) PEAK (?P<peak>\d+)')

LB_AND_FACTOR_INFO = re.compile(r'DAG (?P<name>.*?) spillCostLB (?P<sc_lb>\d+) scFactor (?P<sc_factor>\d+) lengthLB (?P<len_lb>\d+) lenFactor (?P<len_factor>\d+) staticLB (?P<static_lb>\d+)')

STATS = re.compile(r'SLIL stats: DAG (?P<name>.*?) static LB (?P<static_lb>\d+) gap size (?P<gap_size>\d+) enumerated (?P<enumerated>.*?) optimal (?P<optimal>.*?) PERP higher (?P<perp_higher>[^(]*?)')
