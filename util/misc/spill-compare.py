#!/usr/bin/python3
# Compare two log files using the OptSched scheduler with simulate register
# allocation enabled. Find instances where a reduction in cost does not
# correspond with a reduction in spills.

import os, sys
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import regexs
from regexs import block as block_re

regions = {}
totalBlocks = 0
totalMismatches = 0
lowestLength = sys.maxsize
smallestFoundRegion = ''
foundRegion = False


with open(str(sys.argv[1])) as logfile:
    log1 = logfile.read()
    blocks = [block for block in block_re.BLOCK_DELIMITER.split(log1) if block_re.COST_LOWER_BOUND.search(block)]
    for block in blocks:
        if not block_re.COST_LOWER_BOUND.search(block):
            print("WARNING: Block does not have a logged lower bound.", out=sys.stderr)

        totalBlocks += 1

        lowerBound = int(block_re.COST_LOWER_BOUND.search(block).group(1))
        regionCostMatchB = block_re.COST_BEST.findall(block)
        regionName = regionCostMatchB[0]['name']
        regionCostBest = int(regionCostMatchB[0]['cost'])
        regionLengthBest = int(regionCostMatchB[0]['length'])

        if not block_re.SPILLS_BEST.search(block):
            print(regionName)

        regionCostMatchH = block_re.COST_HEURISTIC.findall(block)
        regionCostHeuristic = int(regionCostMatchH[0]['spill_cost'])

        regionSpillsMatchB = block_re.SPILLS_BEST.findall(block)
        regionSpillsBest = int(regionSpillsMatchB[0]['count'])

        regionSpillsMatchH = block_re.SPILLS_HEURISTIC.findall(block)
        regionSpillsHeuristic = int(regionSpillsMatchH[0]['count'])

        if (regionCostBest < regionCostHeuristic and regionSpillsBest > regionSpillsHeuristic):
            totalMismatches+=1
            print("Found Region: "  + regionName + " With Length: " + str(regionLengthBest))
            print("Best Cost: " + str(regionCostBest) + " Heuristic Cost: " + str(regionCostHeuristic))
            print("Best Cost (Absolute): " + (lowerBound + regionCostBest))
            print("Best Spills: " + str(regionSpillsBest) + " Heurisitc Spills: " + str(regionSpillsHeuristic))
            if (regionLengthBest < lowestLength):
                foundRegion = True
                smallestFoundRegion = regionName
                lowestLength = regionLengthBest

    if (foundRegion):
        print("Smallest region with mismatch is: " + str(smallestFoundRegion) + " with length " + str(lowestLength))

    print("Processed " + str(totalBlocks) + " blocks")
    print("Found " + str(totalMismatches) + " mismatches")
