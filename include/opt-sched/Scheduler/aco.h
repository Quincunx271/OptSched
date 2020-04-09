/*******************************************************************************
Description:  Implements an Ant colony optimizing scheduler
Author:       Theodore Dubois
Created:      Nov. 2017
Updated By:   Ciprian Elies and Vang Thao
Last Update:  Jan. 2020
*******************************************************************************/

#ifndef OPTSCHED_ACO_H
#define OPTSCHED_ACO_H

#include "opt-sched/Scheduler/gen_sched.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/ArrayRef.h"
#include <memory>

namespace llvm {
namespace opt_sched {

typedef double pheromone_t;

struct Choice {
  SchedInstruction *inst;
  double heuristic; // range 0 to 1
};

class ACOScheduler : public ConstrainedScheduler {
public:
  ACOScheduler(DataDepGraph *dataDepGraph, MachineModel *machineModel,
               InstCount upperBound, SchedPriorities priorities,
               bool vrfySched);
  virtual ~ACOScheduler();
  FUNC_RESULT FindSchedule(InstSchedule *schedule, SchedRegion *region);
  inline void UpdtRdyLst_(InstCount cycleNum, int slotNum);
  // Set the initial schedule for ACO
  // Default is NULL if none are set.
  void setInitialSched(InstSchedule *Sched);

private:
  pheromone_t &Pheromone(SchedInstruction *from, SchedInstruction *to);
  pheromone_t &Pheromone(InstCount from, InstCount to);
  double Score(SchedInstruction *from, Choice choice);

  void PrintPheromone();

  SchedInstruction *
  SelectInstruction(const llvm::ArrayRef<Choice> &ready,
                    SchedInstruction *lastInst);
  void UpdatePheromone(InstSchedule *schedule);
  std::unique_ptr<InstSchedule> FindOneSchedule();
  llvm::SmallVector<pheromone_t, 0> pheromone_;
  pheromone_t initialValue_;
  bool use_fixed_bias;
  int count_;
  int heuristicImportance_;
  bool use_tournament;
  int fixed_bias;
  double bias_ratio;
  double local_decay;
  double decay_factor;
  int ants_per_iteration;
  bool print_aco_trace;
  std::unique_ptr<InstSchedule> InitialSchedule;
  bool VrfySched_;
};

} // namespace opt_sched
} // namespace llvm

#endif
