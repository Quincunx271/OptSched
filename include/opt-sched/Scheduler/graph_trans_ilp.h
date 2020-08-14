/*******************************************************************************
Description:  Implement graph transformations to be applied before scheduling.
Author:       Austin Kerbow
Created:      June. 2017
Last Update:  June. 2017
*******************************************************************************/

#ifndef OPTSCHED_BASIC_GRAPH_TRANS_ILP_H
#define OPTSCHED_BASIC_GRAPH_TRANS_ILP_H

#include "opt-sched/Scheduler/graph_trans.h"

namespace llvm {
namespace opt_sched {

// Node superiority ILP graph transformation.
class StaticNodeSupILPTrans : public GraphTrans {
public:
  StaticNodeSupILPTrans(DataDepGraph *dataDepGraph);

  FUNC_RESULT ApplyTrans() override;
};

} // namespace opt_sched
} // namespace llvm

#endif
