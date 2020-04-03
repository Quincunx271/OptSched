import optparse
import os, sys
import mmap

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from regexs import slil as slil_re

def debugPrint(msg):
  #print(msg)
  pass

def getBool(msg):
  if msg == "True": return True
  elif msg == "False": return False
  raise Exception("msg is %s" % msg)

def getStatsFromLogFile(filename, path):
  # First, organize raw data before calculate aggregate stats
  functions = {}
  with open(os.path.join(path,filename)) as f:
    m = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)

    for match in slil_re.STATS.finditer(m):
      debugPrint("Found match: %s" % match.group(0))
      blockStats = {}
      dagName = match['name']

      functionName = dagName.split(':')[0]
      if not functionName in functions:
        debugPrint("Found function %s" % functionName)
        functions[functionName] = {}

      blockStats['staticLB'] = int(match['static_lb'])
      blockStats['gapSize'] = int(match['gap_size'])
      blockStats['isEnumerated'] = getBool(match['enumerated'])
      blockStats['isOptimal'] = getBool(match['optimal'])
      blockStats['isPerpHigher'] = getBool(match['perp_higher'])

      blockName = dagName.split(':')[1]
      if blockName in functions[functionName]:
        raise Exception("Block %s already exists in function %s!" % (blockName, functionName))
      functions[functionName][blockName] = blockStats

    m.close()

  # Then, calculate aggregate stats per function
  benchStats = {}
  for functionName in functions:
    benchStats[functionName] = {}
    totalGapSize = 0
    averageGapPercentage = 0
    maxGapPercentage = 0
    totalEnumerated = 0
    totalOptimal = 0
    totalOptimalAndEnumerated = 0
    totalHigherPerp = 0
    totalOptimalHigherPerp = 0
    for blockName in functions[functionName]:
      blockStats = functions[functionName][blockName]
      gapSize = blockStats['gapSize']
      totalGapSize += gapSize
      gapPercentage = float(gapSize) / blockStats['staticLB']
      averageGapPercentage += gapPercentage
      if gapPercentage > maxGapPercentage: maxGapPercentage = gapPercentage
      if blockStats['isOptimal']: totalOptimal += 1
      if blockStats['isEnumerated']: totalEnumerated += 1
      if gapSize == 0 and blockStats['isEnumerated']: totalOptimalAndEnumerated += 1
      if blockStats['isPerpHigher']: 
        totalHigherPerp += 1
        if blockStats['isOptimal']: totalOptimalHigherPerp += 1
    functionStats = benchStats[functionName]
    functionStats['totalBlocks'] = len(functions[functionName])
    functionStats['totalGapSize'] = totalGapSize
    functionStats['averageGapSize'] = float(totalGapSize) / len(functions[functionName])
    functionStats['averageGapPercentage'] = float(averageGapPercentage) / len(functions[functionName])
    functionStats['maxGapPercentage'] = maxGapPercentage
    functionStats['totalEnumerated'] = totalEnumerated
    functionStats['totalOptimal'] = totalOptimal
    functionStats['totalOptimalAndEnumerated'] = totalOptimalAndEnumerated
    functionStats['totalHigherPerp'] = totalHigherPerp
    functionStats['totalOptimalHigherPerp'] = totalOptimalHigherPerp
    
  return benchStats

parser = optparse.OptionParser(description='Wrapper around runspec for collecting spill counts.')
parser.add_option('-p', '--path',
                  metavar='path',
                  default=None,
                  help='Path to log files generated by runspec wrapper.')
args = parser.parse_args()[0]


if not os.path.isdir(args.path):
  raise Exception("Input path: %s is not a folder" % args.path)

stats = {}

for filename in os.listdir(args.path):
  benchName = filename.split('.')[0]
  stats[benchName] = getStatsFromLogFile(filename, args.path)

debugPrint(stats)


"""
    functionStats['totalGapSize'] = totalGapSize
    functionStats['averageGapSize'] = float(totalGapSize) / len(functions[functionName])
    functionStats['averageGapPercentage'] = float(averageGapPercentage) / len(functions[functionName])
    functionStats['totalEnumerated'] = totalEnumerated
    functionStats['totalOptimal'] = totalOptimal
    functionStats['totalHigherPerp'] = totalHigherPerp
    functionStats['totalOptimalHigherPerp'] = totalOptimalHigherPerp

"""

with open("slilStats.txt", 'w') as f:
  for benchName in stats:
    f.write("====================\n")
    f.write("Benchmark %s\n" % benchName)
    f.write("====================\n")
    for functionName in stats[benchName]:
       f.write("  Function %s\n  ----------------\n" % functionName)
       f.write("    Total blocks: %d\n" % stats[benchName][functionName]['totalBlocks'])
       f.write("    Total gap size: %d\n" % stats[benchName][functionName]['totalGapSize'])
       f.write("    Average gap size: %.02f\n" % stats[benchName][functionName]['averageGapSize'])
       f.write("    Average percent gap size: %.02f%%\n" % stats[benchName][functionName]['averageGapPercentage'])
       f.write("    Maximum percent gap size: %.02f%%\n" % stats[benchName][functionName]['maxGapPercentage'])
       f.write("    Enumerated: %d\n" % stats[benchName][functionName]['totalEnumerated'])
       f.write("    Optimal: %d\n" % stats[benchName][functionName]['totalOptimal'])
       f.write("    Enumerated and zero cost: %d\n" % stats[benchName][functionName]['totalOptimalAndEnumerated'])
       f.write("    Higher PERP: %d\n" % stats[benchName][functionName]['totalHigherPerp'])
       f.write("    Higher PERP and optimal: %d\n" % stats[benchName][functionName]['totalOptimalHigherPerp'])
