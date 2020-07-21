#include "opt-sched/Scheduler/tred_graph_trans.h"

#include "opt-sched/Scheduler/logger.h"
#include <cassert>

using namespace llvm::opt_sched;

FUNC_RESULT TransitiveReductionTrans::ApplyTrans() {
  const InstCount numNodes = GetNumNodesInGraph_();
  DataDepGraph *const graph = GetDataDepGraph_();

  int numRemoved = 0;
  Logger::Info("Applying transitive reduction graph transformation.");

  // For every distinct nodes w, u, v,
  // if there are paths w --> u, u --> v, and edge (w, v),
  // then remove the transitive edge (w, v)
  for (int i = 0; i < numNodes; i++) {
    for (int j = 0; j < numNodes; j++) {
      if (i == j) {
        continue;
      }
      for (int k = 0; k < numNodes; k++) {
        if (i == k || j == k) {
          continue;
        }

        assert(i != j && j != k && i != k);
        SchedInstruction *w = graph->GetInstByIndx(i);
        SchedInstruction *u = graph->GetInstByIndx(j);
        SchedInstruction *v = graph->GetInstByIndx(k);

        // Look up the paths and edge in question
        BitVector *wfwd = w->GetRcrsvNghbrBitVector(DIR_FRWRD);
        BitVector *ufwd = w->GetRcrsvNghbrBitVector(DIR_FRWRD);
        GraphEdge *wvS = w->FindScsr(v); // nullptr if no edge

        const bool pathWtoU = wfwd->GetBit(u->GetNum());
        const bool pathUtoV = ufwd->GetBit(v->GetNum());
        if (pathWtoU && pathUtoV && wvS) {
          w->RemoveSuccTo(v);
          v->RemovePredFrom(w);
          delete wvS;
          ++numRemoved;
        }
      }
    }
  }

  Logger::Info("Removed edges: %d", numRemoved);

  return RES_SUCCESS;
}
