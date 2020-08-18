#include "opt-sched/Scheduler/graph_trans_ilp.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include <cassert>
#include <vector>

using namespace llvm::opt_sched;

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
      : DDG_(DDG), DistanceTable_(CreateDistanceTable()),
        Superior_(CreateSuperiorArray()),
        SuperiorNodesList_(CreateSuperiorNodesList()),
        NumNodes_(DDG.GetNodeCnt()) {}

  // Part of the algorithm
  bool ShouldContinue() const;
  void UpdateDistanceTable(int i, int j);
  void RemoveRedundantEdges(int i, int j);
  void UpdateSuperiorNodes(int i, int j);

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
  std::vector<int> DistanceTable_;
  std::vector<int> Superior_;
  std::vector<std::pair<int, int>> SuperiorNodesList_;
  int NumNodes_;
};
} // namespace

int ILPTransformState::DistanceTable(int i, int j) const {
  return DistanceTable_[ComputeIndex(i, j, NumNodes_, NumNodes_)];
}

void ILPTransformState::SetDistanceTable(int i, int j, int val) {
  MutableArrayRef2D<int> DistanceTable(DistanceTable_, NumNodes_, NumNodes_);
  int oldDistance = DistanceTable[{i, j}];
  DistanceTable[{i, j}] = val;

  if (val > oldDistance) {
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
    GraphNode *StartNode = DDG_.GetInstByTplgclOrdr(StartIndex);
    const int Start = StartNode->GetNum();

    for (int Index = StartIndex + 1; Index < NumNodes_; ++Index) {
      GraphNode *ToNode = DDG_.GetInstByTplgclOrdr(Index);
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
    }
  }

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

  const int NumBadPredecessors =
      llvm::count_if(NodeI->GetPredecessors(), [&](GraphEdge &e) {
        return e.label > DistanceTable(e.from->GetNum(), i);
      });

  const int NumBadSuccessors =
      llvm::count_if(NodeJ->GetSuccessors(), [&](GraphEdge &e) {
        return e.label > DistanceTable(j, e.to->GetNum());
      });

  return NumBadPredecessors + NumBadSuccessors;
}

void ILPTransformState::UpdateSuperiorArray(int i, int j) {
  const int OldValue = SuperiorArray()[{i, j}];
  const int NewValue = ComputeSuperiorArrayValue(i, j);
  SuperiorArray()[{i, j}] = NewValue;
  assert(NewValue <= OldValue);

  if (NewValue == 0) {
    SuperiorNodesList_.push_back({i, j});
  }
}

std::vector<int> ILPTransformState::CreateSuperiorArray() {
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
      }
    }
  }

  return Superior_;
}

std::vector<std::pair<int, int>> ILPTransformState::CreateSuperiorNodesList() {
  const InstCount NumNodes = DDG_.GetNodeCnt();
  assert(NumNodes >= 0);

  std::vector<std::pair<int, int>> SuperiorNodes;

  for (int I = 0; I < NumNodes; ++I) {
    for (int J = 0; J < NumNodes; ++J) {
      if (SuperiorArray()[{I, J}] == 0) {
        SuperiorNodes.push_back({I, J});
      }
    }
  }

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
      }
    }
  }
}

static void AddLatencyZeroEdge(DataDepGraph &ddg, int i, int j) {
  SchedInstruction *NodeI = ddg.GetInstByIndx(i);
  SchedInstruction *NodeJ = ddg.GetInstByIndx(j);
  ddg.CreateEdge(NodeI, NodeJ, 0, DEP_OTHER);
  UpdatePredecessorsAndSuccessors(NodeI, NodeJ);
}

static void AddNecessaryResourceEdges(DataDepGraph &ddg, int i, int j) {}

void ILPTransformState::UpdateDistanceTable(int i, int j) {
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
    const int NewDistance = std::min(MaxLatency, DistanceTable(j, k));

    if (NewDistance > OldDistance) {
      SetDistanceTable(i, k, NewDistance);

      for (GraphNode &Pred : *IPredecessors) {
        const int p = Pred.GetNum();
        const int NewPossiblePK = NewDistance + DistanceTable(p, i);
        const int OldPK = DistanceTable(p, k);

        SetDistanceTable(p, k,
                         std::min(MaxLatency, std::max(NewPossiblePK, OldPK)));
      }
    }
  }
}

void ILPTransformState::RemoveRedundantEdges(int i, int j) {
  SchedInstruction *NodeI = DDG_.GetInstByIndx(i);
  SchedInstruction *NodeJ = DDG_.GetInstByIndx(j);

  for (GraphNode &Pred : *NodeI->GetRecursivePredecessors()) {
    LinkedList<GraphEdge> &PSuccs = Pred.GetSuccessors();

    for (auto it = PSuccs.begin(); it != PSuccs.end();) {
      GraphEdge &e = *it;

      if (NodeJ->IsRcrsvScsr(e.to) &&
          e.label <= DistanceTable(e.from->GetNum(), e.to->GetNum())) {
        it = PSuccs.RemoveAt(it);
        e.to->RemovePredFrom(&Pred);
        // DEBUG_LOG("  Deleting GraphEdge* at %p: (%d, %d)", (void *)e,
        //           u->GetNum(), vp->GetNum());
        delete &e;
      } else {
        ++it;
      }
    }
  }
}

StaticNodeSupILPTrans::StaticNodeSupILPTrans(DataDepGraph *dataDepGraph)
    : GraphTrans(dataDepGraph) {}

FUNC_RESULT StaticNodeSupILPTrans::ApplyTrans() {
  DataDepGraph &ddg = *GetDataDepGraph_();
  assert(GetNumNodesInGraph_() == ddg.GetNodeCnt());

  ILPTransformState state(ddg);

  while (state.ShouldContinue()) {
    auto ij = state.PopNextSuperiorNodePair();
    const int i = ij.first;
    const int j = ij.second;
    if (!AreNodesIndependent(ddg.GetInstByIndx(i), ddg.GetInstByIndx(j))) {
      continue;
    }

    AddLatencyZeroEdge(ddg, i, j);
    AddNecessaryResourceEdges(ddg, i, j);

    state.UpdateDistanceTable(i, j);
    state.UpdateSuperiorArray(i, j);
    state.RemoveRedundantEdges(i, j);
  }

  return RES_SUCCESS;
}
