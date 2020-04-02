import mmap
import optparse

import os, sys
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import regexs

def getNodeCount(fileName):
  count = 0
  with open(fileName) as bff:
    bffm = mmap.mmap(bff.fileno(), 0, access=mmap.ACCESS_READ)

    for match in regexs.NUM_EXAMINED.finditer(bffm):
      count += int(match['count'])
      
    bffm.close()

  return count

parser = optparse.OptionParser(
    description='Wrapper around runspec for collecting spill counts.')
parser.add_option('-p', '--path',
                  metavar='path',
                  default=None,
                  help='Log file.')
parser.add_option('--isfolder',
                  action='store_true',
                  help='Specify if parsing a foldere.')

args = parser.parse_args()[0]

total = 0

if args.isfolder:
  if not os.path.isdir(args.path):
    raise Error("Please specify a valid folder.")
  for filename in os.listdir(args.path):
    total += getNodeCount(os.path.join(args.path, filename))
else:
  if not os.path.isfile(args.path):
    raise Error("Please specify a valid log file.")
  total += getNodeCount(args.path)
  
print(total)
