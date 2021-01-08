#!/usr/bin/env python3

import re
import glob

EAST_CONST = re.compile(r'''
    \b([a-zA-Z0-9_]+)       # An identifier
                            # Which "identifier" is not one of these keywords:
    (?<!\bstatic)
    (?<!\bconstexpr)
    (?<!\binline)
    (?<!\bvirtual)
    (?<!\belse)
    (?<!\bnew)
    (?<!\bsizeof)
    (?<!\btypedef)
    (?<!\btypename)
    (?<!\bvolatile)
    (?<!\bconsteval)
    (?<!\bconstinit)

    \ const\b               # Followed by "const"
''', re.VERBOSE)

files = []
for rootdir in ('include', 'lib', 'unittests'):
    for ext in ('h', 'cpp'):
        files += glob.glob('{rootdir}/**/*.{ext}'.format(rootdir=rootdir,
                                                         ext=ext), recursive=True)

for file in files:
    with open('lib/Scheduler/aco.cpp', 'r+') as f:
        contents = f.read()
        contents = EAST_CONST.sub(r'const \1', contents)
        f.seek(0)
        f.write(contents)
        f.truncate()
