# /bin/python3
# Run this script with a CPU2006 logfile as the only argument.
# When using RegAllocFast, find the total number of spills and the proportion of
# those spills that are added at region and block boundaries.

import os, sys
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import regexs
from regexs import block as block_re

totalSpills = 0
totalCallBoundaryStores = 0
totalBlockBoundaryStores = 0
totalLiveInLoads = 0
totalFuncs = 0
#funcs = {}

if __name__ == '__main__':
    with open(sys.argv[1]) as inputLog:
        for line in inputLog.readlines():
            searchTotalSpills = regexs.NUM_SPILLS_FAST_RA.findall(line)
            searchCallBoundaryStores = regexs.CALL_BOUNDARY_STORES.findall(line)
            searchBlockBoundaryStores = regexs.BLOCK_BOUNDARY_STORES.findall(line)
            searchLiveInLoads = regexs.LIVE_IN_LOADS.findall(line)
            
            if searchTotalSpills:
                totalSpills += int(searchTotalSpills[0]['count'])
                totalFuncs+=1
            if searchCallBoundaryStores:
                totalCallBoundaryStores += int(searchCallBoundaryStores[0]['count'])
            if searchBlockBoundaryStores:
                totalBlockBoundaryStores += int(searchBlockBoundaryStores[0]['count'])
            if searchLiveInLoads:
                totalLiveInLoads += int(searchLiveInLoads[0]['count'])

    print("Total Spills: " + str(totalSpills))
    print("Total Call Boundary Stores: " + str(totalCallBoundaryStores))
    print("Total Block Boundary Stores: " + str(totalBlockBoundaryStores))
    print("Total Live-In Loads: " + str(totalLiveInLoads))
    print("Total funcs: " + str(totalFuncs))
