'''
Log-parsing regexs for plaidbench
'''

import re

PASS_NUM = re.compile(r'End of (?P<pass>.*) pass through')
OCCUPANCY = re.compile(r'Final occupancy for function (?P<name>.*):(?P<count>\d+)')
