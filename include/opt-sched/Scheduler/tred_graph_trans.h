/*******************************************************************************
Description:  Implements the Transitive Reduction as a graph transformation
Author:       Justin Bassett
Created:      July 2020
Last Update:  July 2020
*******************************************************************************/

#ifndef OPTSCHED_TRED_GRAPH_TRANS_H
#define OPTSCHED_TRED_GRAPH_TRANS_H

#include "opt-sched/Scheduler/graph_trans.h"

namespace llvm {
namespace opt_sched {

/**
 * A GraphTrans which computes the transitive reduction of the DataDepGraph.
 *
 * The transitive reduction removes all transitive (redundant) edges from the
 * graph.
 */
class TransitiveReductionTrans : public GraphTrans {
public:
  // Inherit GraphTrans constructors.
  using GraphTrans::GraphTrans;

  FUNC_RESULT ApplyTrans() override;
};

} // namespace opt_sched
} // namespace llvm

#endif
