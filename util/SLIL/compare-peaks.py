import os, sys
import mmap

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from regexs import slil as slil_re

def readPeakCosts(logFile):
  peakCosts = {}
  with open(logFile) as f:
    m = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
    for match in slil_re.PEAK.finditer(m):
      peakCosts[match['name']] = int(match['peak'])
    m.close()
  return peakCosts
  
def compareWrapperLogs(path1, path2, logFile):
  benchName = logFile.split(".")[0]
  if not (os.path.isfile(os.path.join(path1, logFile)) and os.path.isfile(os.path.join(path2, logFile))):
    return
    
  peakCosts1 = readPeakCosts(os.path.join(path1, logFile))
  peakCosts2 = readPeakCosts(os.path.join(path2, logFile))
  
  for key in peakCosts1:
    if key in peakCosts2:
      print("%s,%s,%d,%d" % (benchName, key, peakCosts1[key], peakCosts2[key]))
  
if len(sys.argv) != 3:
  raise Exception("Invalid number of arguments")
  
if not os.path.isdir(sys.argv[1]):
  raise Exception("'%s' is not a valid directory" % sys.argv[1])
  
if not os.path.isdir(sys.argv[2]):
  raise Exception("'%s' is not a valid directory" % sys.argv[2])

for subdirs, dirs, files in os.walk(sys.argv[1]):
  for f in files:
    compareWrapperLogs(sys.argv[1], sys.argv[2], f)
