#/usr/bin/python3
# Calculate how often OptSched's register pressure estimates match LLVM's
# You must compile OptSched with IS_DEBUG_PEAK_PRESSURE flag enabled.

import os, sys
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import regexs
from regexs import block as block_re

# The number of register types.
MAX_REG_TYPES = 30

totalBlocks = 0
totalMismatches = 0
majorMismatches = 0

with open(str(sys.argv[1])) as logfile:
    log = logfile.read()
    blocks = log.split("INFO: ********** Opt Scheduling **********")

for block in blocks:
    if not block_re.DAG_NAME_SIZE_LATENCY.search(block): continue

    optSchedPressures = [None]*MAX_REG_TYPES
    llvmPressures = [None]*MAX_REG_TYPES

    totalBlocks+=1
    blockName = RP_DAG_NAME.findall(block)[0]

    for matchOpt in regexs.OPT_INFO.finditer(block):
        index = int(matchOpt['index'])
        name = matchOpt['name']
        peak = matchOpt['peak']
        limit = matchOpt['limit']

        optSchedPressures[index] = {
            'name': name,
            'peak': peak,
            'limit': limit,
        }

    for matchLLVM in regexs.AFTER_INFO.finditer(block):
        index = int(matchLLVM['index'])
        name = matchLLVM['name']
        peak = matchLLVM['peak']
        limit = matchLLVM['limit']
        llvmPressures[index] = {
            'name': name,
            'peak': peak,
            'limit': limit,
        }

    for i in range(MAX_REG_TYPES):
        optP = optSchedPressures[i]
        llvmP = llvmPressures[i]

        if (optP['peak'] != llvmP['peak']):
            print('Mismatch in block ' + blockName + '.')
            print('Reg type with mismatch ' + optP['name'] + \
                  ' Limit ' + optP['limit'] + ' Peak OptSched ' + optP['peak'] + \
                  ' Peak LLVM ' + llvmP['peak'] + '.')
            totalMismatches+=1
            # A major mismatch occurs when peak pressure is over physical limit.
            if (max(int(optP['peak']), int(llvmP['peak'])) > int(optP['limit'])):
                print('Major mismatch!')
                majorMismatches+=1

print('Total blocks processed ' + str(totalBlocks) + '.')
print('Total mismatches ' + str(totalMismatches) + '.')
print('Total major mismatches ' + str(majorMismatches) + '.')
