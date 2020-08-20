#include "opt-sched/Scheduler/graph_trans_ilp.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include <cassert>
#include <vector>

using namespace llvm::opt_sched;

#define IS_DEBUG_ILP_GRAPH_TRANS

#ifdef IS_DEBUG_ILP_GRAPH_TRANS
#define DEBUG_LOG(...) Logger::Info(__VA_ARGS__)
#else
#define DEBUG_LOG(...) static_cast<void>(0)
#endif

namespace {
size_t ComputeIndex(int i, int j, size_t Rows, size_t Columns) {
  assert(i >= 0);
  assert(j >= 0);

  size_t index = size_t(i) * Columns + size_t(j);
  assert(index < Rows * Columns);
  return index;
}

template <typename T> class MutableArrayRef2D {
public:
  explicit MutableArrayRef2D(llvm::MutableArrayRef<T> Ref, size_t Rows,
                             size_t Columns)
      : Ref_(Ref), Rows_(Rows), Columns_(Columns) {
    assert(Rows * Columns == Ref.size());
  }

  size_t Rows() const { return Rows_; }
  size_t Columns() const { return Columns_; }

  T &operator[](size_t(&&RCIndices)[2]) const {
    return Ref_[ComputeIndex(RCIndices[0], RCIndices[1], Rows_, Columns_)];
  }

  llvm::MutableArrayRef<T> UnderlyingData() const { return Ref_; }

private:
  llvm::MutableArrayRef<T> Ref_;
  size_t Rows_;
  size_t Columns_;
};

struct ILPTransformState {
  explicit ILPTransformState(DataDepGraph &DDG)
      : DDG_(DDG), NumNodes_(DDG.GetNodeCnt()),
        DistanceTable_(CreateDistanceTable()), Superior_(CreateSuperiorArray()),
        SuperiorNodesList_(CreateSuperiorNodesList()) {}

  // Part of the algorithm
  bool ShouldContinue() const { return !SuperiorNodesList_.empty(); }
  void UpdateDistanceTable(int i, int j);
  void RemoveRedundantEdges(int i, int j);
  void UpdateSuperiorNodes(int i, int j);
  void AddLatencyZeroEdge(int i, int j);
  void AddNecessaryResourceEdges(int i, int j);

  std::pair<int, int> PopNextSuperiorNodePair() {
    auto result = SuperiorNodesList_.back();
    SuperiorNodesList_.pop_back();
    return result;
  }

  // Accessors
  int DistanceTable(int i, int j) const;
  void SetDistanceTable(int i, int j, int val);
  void UpdateSuperiorArray(int i, int j);
  int ComputeSuperiorArrayValue(int i, int j) const;
  MutableArrayRef2D<int> SuperiorArray();

  // Initialization only
  std::vector<int> CreateDistanceTable();
  std::vector<int> CreateSuperiorArray();
  std::vector<std::pair<int, int>> CreateSuperiorNodesList();

  DataDepGraph &DDG_;
  int NumNodes_;
  std::vector<int> DistanceTable_;
  std::vector<int> Superior_;
  std::vector<std::pair<int, int>> SuperiorNodesList_;

  int NumEdgesAdded = 0;
  int NumEdgesRemoved = 0;
  int NumResourceEdgesAdded = 0;
};
} // namespace

int ILPTransformState::DistanceTable(int i, int j) const {
  return DistanceTable_[ComputeIndex(i, j, NumNodes_, NumNodes_)];
}

void ILPTransformState::SetDistanceTable(int i, int j, int Val) {
  MutableArrayRef2D<int> DistanceTable(DistanceTable_, NumNodes_, NumNodes_);
  const int OldDistance = DistanceTable[{i, j}];
  DistanceTable[{i, j}] = Val;
  DEBUG_LOG("  Updated DISTANCE(%d, %d) = %d (old = %d)", i, j, Val,
            OldDistance);

  if (Val > OldDistance) {
    SchedInstruction *NodeI = DDG_.GetInstByIndx(i);
    SchedInstruction *NodeJ = DDG_.GetInstByIndx(j);

    for (GraphEdge &e : NodeI->GetPredecessors()) {
      UpdateSuperiorArray(e.from->GetNum(), j);
    }
    for (GraphEdge &e : NodeJ->GetSuccessors()) {
      UpdateSuperiorArray(i, e.to->GetNum());
    }
  }
}

MutableArrayRef2D<int> ILPTransformState::SuperiorArray() {
  return MutableArrayRef2D<int>(Superior_, NumNodes_, NumNodes_);
}

std::vector<int> ILPTransformState::CreateDistanceTable() {
  const int MaxLatency = DDG_.GetMaxLtncy();
  DEBUG_LOG("Creating DISTANCE() table with MaxLatency: %d", MaxLatency);

  std::vector<int> DistanceTable_;
  DistanceTable_.resize(
      NumNodes_ * NumNodes_,
      // DISTANCE(i, j) where no edge (i, j) exists = -infinity
      std::numeric_limits<int>::lowest());
  MutableArrayRef2D<int> DistanceTable(DistanceTable_, NumNodes_, NumNodes_);

  // DISTANCE(i, i) = 0
  for (int Index = 0; Index < NumNodes_; ++Index) {
    DistanceTable[{Index, Index}] = 0;
  }

  for (int StartIndex = 0; StartIndex < NumNodes_; ++StartIndex) {
    SchedInstruction *StartNode = DDG_.GetInstByTplgclOrdr(StartIndex);
    const int Start = StartNode->GetNum();

    for (int Index = StartIndex + 1; Index < NumNodes_; ++Index) {
      SchedInstruction *ToNode = DDG_.GetInstByTplgclOrdr(Index);
      const int To = ToNode->GetNum();

      int CurrentMax = -1;

      for (GraphEdge &e : ToNode->GetPredecessors()) {
        const int From = e.from->GetNum();
        CurrentMax =
            std::max(CurrentMax, e.label + DistanceTable[{Start, From}]);

        if (CurrentMax >= MaxLatency) {
          CurrentMax = MaxLatency;
          break;
        }
      }

      DistanceTable[{Start, To}] = CurrentMax;
      DEBUG_LOG(" Distance %d -> %d is %d. Old computation as %d.", Start, To,
                CurrentMax,
                StartNode->GetRltvCrtclPath(DIRECTION::DIR_FRWRD, ToNode));
    }
  }

  DEBUG_LOG("Finished creating DISTANCE() table\n");

  return DistanceTable_;
}

static bool AreNodesIndependent(SchedInstruction *A, SchedInstruction *B) {
  return A != B && !A->IsRcrsvPrdcsr(B) && !A->IsRcrsvScsr(B);
}

int ILPTransformState::ComputeSuperiorArrayValue(int i, int j) const {
  SchedInstruction *NodeI = DDG_.GetInstByIndx(i);
  SchedInstruction *NodeJ = DDG_.GetInstByIndx(j);

  if (NodeI->GetInstType() != NodeJ->GetInstType() ||
      !AreNodesIndependent(NodeI, NodeJ)) {
    return -1;
  }

  DEBUG_LOG("   Counting bad IPred");
  const int NumBadPredecessors =
      llvm::count_if(NodeI->GetPredecessors(), [&](GraphEdge &e) {
        DEBUG_LOG("    LATENCY(%d, %d) = %d <> DISTANCE(%d, %d) = %d",
                  e.from->GetNum(), i, e.label, //
                  e.from->GetNum(), j, DistanceTable(e.from->GetNum(), j));
        return e.label > DistanceTable(e.from->GetNum(), j);
      });

  DEBUG_LOG("   Counting bad ISucc");
  const int NumBadSuccessors =
      llvm::count_if(NodeJ->GetSuccessors(), [&](GraphEdge &e) {
        DEBUG_LOG("    LATENCY(%d, %d) = %d <> DISTANCE(%d, %d) = %d", //
                  j, e.from->GetNum(), e.label,                        //
                  i, e.from->GetNum(), DistanceTable(i, e.to->GetNum()));
        return e.label > DistanceTable(i, e.to->GetNum());
      });

  return NumBadPredecessors + NumBadSuccessors;
}

void ILPTransformState::UpdateSuperiorArray(int i, int j) {
  const int OldValue = SuperiorArray()[{i, j}];
  const int NewValue = ComputeSuperiorArrayValue(i, j);
  SuperiorArray()[{i, j}] = NewValue;
  DEBUG_LOG("  Updating SUPERIOR(%d, %d) = %d (old = %d)", i, j, NewValue,
            OldValue);
  assert(NewValue <= OldValue);

  if (NewValue == 0) {
    DEBUG_LOG("   Tracking (%d, %d) as a possible superior edge", i, j);
    SuperiorNodesList_.push_back({i, j});
  }
}

std::vector<int> ILPTransformState::CreateSuperiorArray() {
  DEBUG_LOG("Creating SUPERIOR() array");
  std::vector<int> Superior_;
  Superior_.resize(NumNodes_ * NumNodes_, -1);
  MutableArrayRef2D<int> Superior(Superior_, NumNodes_, NumNodes_);

  for (int I = 0; I < NumNodes_; ++I) {
    for (int J = 0; J < NumNodes_; ++J) {
      SchedInstruction *NodeI = DDG_.GetInstByIndx(I);
      SchedInstruction *NodeJ = DDG_.GetInstByIndx(J);

      if (NodeI->GetInstType() == NodeJ->GetInstType() &&
          AreNodesIndependent(NodeI, NodeJ)) {
        Superior[{I, J}] = ComputeSuperiorArrayValue(I, J);
        DEBUG_LOG(" SUPERIOR(%d, %d) = %d", I, J, Superior[{I, J}]);
      }
    }
  }
  DEBUG_LOG("Finished creating SUPERIOR() array\n");

  return Superior_;
}

std::vector<std::pair<int, int>> ILPTransformState::CreateSuperiorNodesList() {
  DEBUG_LOG("Creating SuperiorList of nodes with superiority available");
  const InstCount NumNodes = DDG_.GetNodeCnt();
  assert(NumNodes >= 0);

  std::vector<std::pair<int, int>> SuperiorNodes;

  for (int I = 0; I < NumNodes; ++I) {
    for (int J = 0; J < NumNodes; ++J) {
      if (SuperiorArray()[{I, J}] == 0) {
        SuperiorNodes.push_back({I, J});
        DEBUG_LOG(" Tracking (%d, %d) as SUPERIOR(...) = 0", I, J);
      }
    }
  }
  DEBUG_LOG("Finished initial values for SuperiorList\n");

  return SuperiorNodes;
}

static void UpdatePredecessorsAndSuccessors(SchedInstruction *NodeI,
                                            SchedInstruction *NodeJ) {
  LinkedList<GraphNode> *nodeBScsrLst = NodeJ->GetRcrsvNghbrLst(DIR_FRWRD);
  LinkedList<GraphNode> *nodeAPrdcsrLst = NodeI->GetRcrsvNghbrLst(DIR_BKWRD);

  // Update lists for the nodes themselves.
  NodeI->AddRcrsvScsr(NodeJ);
  NodeJ->AddRcrsvPrdcsr(NodeI);

  for (GraphEdge &X : NodeI->GetPredecessors()) {
    for (GraphEdge &Y : NodeJ->GetSuccessors()) {
      if (!X.from->IsRcrsvScsr(Y.to)) {
        Y.to->AddRcrsvPrdcsr(X.from);
        X.from->AddRcrsvScsr(Y.to);
        DEBUG_LOG("  Created new recursive path from %d --> %d",
                  X.from->GetNum(), Y.to->GetNum());
      }
    }
  }
}

void ILPTransformState::AddLatencyZeroEdge(int i, int j) {
  SchedInstruction *NodeI = DDG_.GetInstByIndx(i);
  SchedInstruction *NodeJ = DDG_.GetInstByIndx(j);
  DDG_.CreateEdge(NodeI, NodeJ, 0, DEP_OTHER);
  ++NumEdgesAdded;
  DEBUG_LOG(" Added (%d, %d) superior edge", i, j);
  UpdatePredecessorsAndSuccessors(NodeI, NodeJ);
}

void ILPTransformState::AddNecessaryResourceEdges(int i, int j) {
  DEBUG_LOG(" Resource edges not currently implemented");
}

void ILPTransformState::UpdateDistanceTable(int i, int j) {
  DEBUG_LOG(" Updating DISTANCE() table");
  // Adding the edge (i, j) increases DISTANCE(i, j) to 0 (from -infinity).
  SetDistanceTable(i, j, 0);

  const int MaxLatency = DDG_.GetMaxLtncy();

  SchedInstruction *NodeI = DDG_.GetInstByIndx(i);
  SchedInstruction *NodeJ = DDG_.GetInstByIndx(j);

  LinkedList<GraphNode> *JSuccessors = NodeJ->GetRecursiveSuccessors();
  LinkedList<GraphNode> *IPredecessors = NodeI->GetRecursivePredecessors();

  for (GraphNode &Succ : *JSuccessors) {
    const int k = Succ.GetNum();
    const int OldDistance = DistanceTable(i, k);
    // The "new" DISTANCE(i, k) = DISTANCE(j, k) because we added a latency 0
    // edge (i, j), but only if this "new distance" is larger.
    const int NewDistance = std::min(MaxLatency, DistanceTable(j, k));

    if (NewDistance > OldDistance) {
      DEBUG_LOG("  Increased DISTANCE(%d, %d) = %d (old = %d)", i, k,
                NewDistance, OldDistance);
      SetDistanceTable(i, k, NewDistance);

      for (GraphNode &Pred : *IPredecessors) {
        const int p = Pred.GetNum();
        const int NewPossiblePK =
            std::min(MaxLatency, NewDistance + DistanceTable(p, i));
        const int OldPK = DistanceTable(p, k);

        if (NewPossiblePK > OldPK) {
          DEBUG_LOG("   Increased (i, k) distance resulted in increased "
                    "DISTANCE(%d, %d) = %d (old = %d)",
                    p, k, NewPossiblePK, OldPK);
          SetDistanceTable(p, k, NewPossiblePK);
        }
      }
    }
  }
}

void ILPTransformState::RemoveRedundantEdges(int i, int j) {
  // We can't remove redundant edges at this time, because the LinkedList class
  // doesn't support removal if it uses its custom allocator.

  // DEBUG_LOG(" Removing redundant edges");
  // SchedInstruction *NodeI = DDG_.GetInstByIndx(i);
  // SchedInstruction *NodeJ = DDG_.GetInstByIndx(j);

  // for (GraphNode &Pred : *NodeI->GetRecursivePredecessors()) {
  //   LinkedList<GraphEdge> &PSuccs = Pred.GetSuccessors();

  //   for (auto it = PSuccs.begin(); it != PSuccs.end();) {
  //     GraphEdge &e = *it;

  //     if (NodeJ->IsRcrsvScsr(e.to) &&
  //         e.label <= DistanceTable(e.from->GetNum(), e.to->GetNum())) {
  //       it = PSuccs.RemoveAt(it);
  //       e.to->RemovePredFrom(&Pred);
  //       DEBUG_LOG("  Deleting GraphEdge* at %p: (%d, %d)", (void *)&e,
  //                 e.from->GetNum(), e.to->GetNum());
  //       delete &e;
  //       ++NumEdgesRemoved;
  //     } else {
  //       ++it;
  //     }
  //   }
  // }
}

StaticNodeSupILPTrans::StaticNodeSupILPTrans(DataDepGraph *dataDepGraph)
    : GraphTrans(dataDepGraph) {}

FUNC_RESULT StaticNodeSupILPTrans::ApplyTrans() {
  DataDepGraph &ddg = *GetDataDepGraph_();
  assert(GetNumNodesInGraph_() == ddg.GetNodeCnt());
  Logger::Info("Performing ILP graph transformations");

  ILPTransformState state(ddg);

  DEBUG_LOG("Starting main algorithm");
  while (state.ShouldContinue()) {
    auto ij = state.PopNextSuperiorNodePair();
    const int i = ij.first;
    const int j = ij.second;
    DEBUG_LOG("Considering adding a superior edge (%d, %d)", i, j);

    if (!AreNodesIndependent(ddg.GetInstByIndx(i), ddg.GetInstByIndx(j))) {
      DEBUG_LOG("Skipping (%d, %d) because nodes are no longer independent\n",
                i, j);
      continue;
    }

    state.AddLatencyZeroEdge(i, j);
    state.AddNecessaryResourceEdges(i, j);

    state.UpdateDistanceTable(i, j);
    state.UpdateSuperiorArray(i, j);
    state.RemoveRedundantEdges(i, j);

    DEBUG_LOG("Finished iteration for (%d, %d)\n", i, j);
  }

  Logger::Info("Finished ILP graph transformations. Added edges: %d. Removed "
               "redundant edges: %d. Resource edges utilized: %d",
               state.NumEdgesAdded, state.NumEdgesRemoved,
               state.NumResourceEdgesAdded);

  return RES_SUCCESS;
}
