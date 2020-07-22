#include "opt-sched/Scheduler/tred_graph_trans.h"

#include "opt-sched/Scheduler/logger.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include <cassert>

using namespace llvm::opt_sched;

template <typename F>
static void DepthFirstVisit_(llvm::SmallPtrSetImpl<GraphNode *> &seen,
                             GraphNode *node, F f) {
  seen.insert(node);
  f(node);

  for (GraphEdge &edge : node->GetSuccessors()) {
    GraphNode *next = edge.to;
    if (!seen.count(next)) {
      DepthFirstVisit_(seen, next, f);
    }
  }
}

template <typename F>
static void DepthFirstVisit(llvm::SmallPtrSetImpl<GraphNode *> &seen,
                            GraphNode *node, F f) {
  seen.clear();
  DepthFirstVisit_(seen, node, f);
}

FUNC_RESULT TransitiveReductionTrans::ApplyTrans() {
  const InstCount numNodes = GetNumNodesInGraph_();
  DataDepGraph *const graph = GetDataDepGraph_();

  int numRemoved = 0;
  Logger::Info("Applying transitive reduction graph transformation.");

  // Outside the loop: reuse memory across each iteration.
  llvm::SmallPtrSet<GraphNode *, 32> depthFirstHasSeen;
  // 1KB on a 64-bit machine, 1/2 a KB on a 32-bit machine
  llvm::SmallVector<GraphEdge *, 128> edgesToRemove;

  // For every node u,
  // For each neighbor v (that is, (u, v) is an edge),
  // Check every v' reachable from v (DFS); if (u, v') exists, remove it.
  for (int i = 0; i < numNodes; i++) {
    edgesToRemove.clear();

    // TODO: evaluate if graph->GetInstByTplgclOrdr(i); would be faster.
    SchedInstruction *u = graph->GetInstByIndx(i);

    for (GraphEdge &edge : u->GetSuccessors()) {
      GraphNode *v = edge.to;

      ::DepthFirstVisit(depthFirstHasSeen, v, [&](GraphNode *vp) {
        GraphEdge *uvp = u->FindScsr(vp);
        if (uvp) {
          edgesToRemove.push_back(uvp);
        }
      });
    }

    numRemoved += edgesToRemove.size();
    for (GraphEdge *e : edgesToRemove) {
      GraphNode *vp = e->to;
      u->RemoveSuccTo(vp);
      vp->RemovePredFrom(u);
      delete e;
    }
  }

  Logger::Info("Removed edges: %d", numRemoved);

  return RES_SUCCESS;
}
