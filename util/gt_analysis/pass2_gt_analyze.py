#!/usr/bin/env python3

from analyze.script import *
import gt_analyze

run(gt_analyze.gt_analyze, _0) \
    .keep_if(lambda blk: 'PassFinished' in blk and blk.single('PassFinished')['num'] == 2)
