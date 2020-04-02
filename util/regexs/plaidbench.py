'''
Log-parsing regexs for plaidbench
'''

import re

PASS_NUM = re.compile(r'End of (?P<nth>.*) pass through')
RE_OCCUPANCY = re.compile(r'Final occupancy for function (?P<name>.*):(?P<count>\d+)')
