/*******************************************************************************
Description:  Defines a machine model class that abstracts machine description
              information to be used by an instruction scheduler. All properties
              of the machine model are read-only.
Author:       Ghassan Shobaki
Created:      Jun. 2002
Last Update:  Mar. 2011
*******************************************************************************/

#ifndef OPTSCHED_BASIC_MACHINE_MODEL_H
#define OPTSCHED_BASIC_MACHINE_MODEL_H

// For class ostream.
#include <iostream>
// For class string.
#include <string>
// For class vector.
#include "opt-sched/Scheduler/defines.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include <array>
#include <memory>
#include <vector>

namespace llvm {
namespace opt_sched {
// The possible types of dependence between two machine instructions.
enum DependenceType {
  // A true data dependence (read after write).
  DEP_DATA,
  // A register anti-dependence (write after read).
  DEP_ANTI,
  // A register output-dependence (write after write).
  DEP_OUTPUT,
  // Any other ordering dependence.
  DEP_OTHER
};

// Instruction type. An index into the machine's instructions list.
typedef int16_t InstType;
// Machine issue type. An index into the machine's issue type list. All
// instructions of the same issue type use the same pipeline.
typedef int16_t IssueType;

// The code returned when the user tries to parse an unknown instruction type.
const InstType INVALID_INST_TYPE = -1;
// The code returned when the user tries to parse an unknown issue type.
const IssueType INVALID_ISSUE_TYPE = -1;
// The issue type for NOP instructions inserted by the scheduler.
const IssueType ISSU_STALL = -2;
// The maximum allowed number of issue types. Not used by the machine model
// code itself, but some of the legacy users of this class use it.
const int MAX_ISSUTYPE_CNT = 20;

// A description of an instruction type.
struct InstTypeInfo {
  // The name of the instruction type.
  std::string Name;
  // Whether instructions of this type can be scheduled only in a particular
  // context.
  bool IsContextDependent;
  // The issue type used by instructions of this type.
  llvm::opt_sched::IssueType IssueType;
  // The latency of this instructions, i.e. the number of cycles this
  // instruction takes before instructions that depend on it (true
  // dependence) can be scheduled.
  int16_t Latency;
  // Whether instructions of this type are pipelined.
  bool Pipelined;
  // Whether instructions of this type are supported.
  bool Supported;
  // Whether instructions of this type block the cycle, such that no other
  // instructions can be scheduled in the same cycle.
  bool BlocksCycle;
};

// A description of a issue type/FU.
struct IssueTypeInfo {
  // The name of the issue type.
  std::string Name;
  // How many slots of this issue type the machine has per cycle.
  int SlotsCount;
};

// A description of a register type.
struct RegTypeInfo {
  // The name of the register.
  std::string Name;
  // How many register of this type the machine has.
  int Count;
};

struct MachineModel {
  // The name of the machine model.
  std::string ModelName;
  // The machine's issue rate. I.e. the total number of issue slots for all
  // issue types.
  int IssueRate;
  // The latencies for different dependence types.
  std::array<int16_t, 4> DependenceLatencies;
  // Whether the machine model includes unpipelined instructions.
  bool IncludesUnpipelined = false;

  // A vector of instruction type descriptions.
  llvm::SmallVector<InstTypeInfo, 10> InstTypes;
  // A vector of register types with their names and counts.
  llvm::SmallVector<RegTypeInfo, 10> RegisterTypes;
  // A vector of issue types with their names and slot counts.
  llvm::SmallVector<IssueTypeInfo, 10> IssueTypes;
};

std::shared_ptr<MachineModel> parseMachineModel(const std::string &ModelFile);

InstType instTypeByName(const MachineModel &, llvm::StringRef typeName,
                        llvm::StringRef prevName = "");

int16_t regTypeByName(const MachineModel &, llvm::StringRef regTypeName);

IssueType issueTypeByName(const MachineModel &, llvm::StringRef issuTypeName);

int16_t latency(const MachineModel &, InstType instTypeCode,
                DependenceType depType);

bool isRealInst(const MachineModel &, InstType instTypeCode);
bool isBranch(const MachineModel &, InstType instTypeCode);
bool isArtificial(const MachineModel &, InstType instTypeCode);
bool isCall(const MachineModel &, InstType instTypeCode);
bool isFloat(const MachineModel &, InstType instTypeCode);

InstType defaultInstType(const MachineModel &);
InstType defaultIssueType(const MachineModel &);

bool isSimple(const MachineModel &);

llvm::SmallVector<int, MAX_ISSUTYPE_CNT> allSlotsPerCycle(const MachineModel &);

} // namespace opt_sched
} // namespace llvm

#endif
