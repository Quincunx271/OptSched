#include "opt-sched/Scheduler/machine_model.h"
// For atoi().
#include <cstdlib>
// for setiosflags(), setprecision().
#include "opt-sched/Scheduler/buffers.h"
#include "opt-sched/Scheduler/logger.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <iomanip>
#include <string>

using namespace llvm::opt_sched;

std::shared_ptr<MachineModel>
llvm::opt_sched::parseMachineModel(const std::string &ModelFile) {
  auto Result = std::make_shared<MachineModel>();

  SpecsBuffer buf;
  char buffer[MAX_NAMESIZE];

  buf.Load(ModelFile.c_str());

  buf.ReadSpec("MODEL_NAME:", buffer);
  Result->ModelName = buffer;

  Result->IssueRate = buf.ReadIntSpec("ISSUE_RATE:");

  Result->IssueTypes.resize(buf.ReadIntSpec("ISSUE_TYPE_COUNT:"));
  for (IssueTypeInfo &Info : Result->IssueTypes) {
    int pieceCnt;
    char *strngs[INBUF_MAX_PIECES_PERLINE];
    int lngths[INBUF_MAX_PIECES_PERLINE];
    buf.GetNxtVldLine(pieceCnt, strngs, lngths);

    if (pieceCnt != 2)
      llvm::report_fatal_error("Invalid issue type spec", false);

    Info.Name = strngs[0];
    Info.SlotsCount = atoi(strngs[1]);
  }

  Result->DependenceLatencies[DEP_DATA] = 1; // Shouldn't be used!
  Result->DependenceLatencies[DEP_ANTI] =
      (int16_t)buf.ReadIntSpec("DEP_LATENCY_ANTI:");
  Result->DependenceLatencies[DEP_OUTPUT] =
      (int16_t)buf.ReadIntSpec("DEP_LATENCY_OUTPUT:");
  Result->DependenceLatencies[DEP_OTHER] =
      (int16_t)buf.ReadIntSpec("DEP_LATENCY_OTHER:");

  Result->RegisterTypes.resize(buf.ReadIntSpec("REG_TYPE_COUNT:"));
  for (RegTypeInfo &Info : Result->RegisterTypes) {
    int pieceCnt;
    char *strngs[INBUF_MAX_PIECES_PERLINE];
    int lngths[INBUF_MAX_PIECES_PERLINE];
    buf.GetNxtVldLine(pieceCnt, strngs, lngths);

    if (pieceCnt != 2) {
      llvm::report_fatal_error("Invalid register type spec", false);
    }

    Info.Name = strngs[0];
    Info.Count = atoi(strngs[1]);
  }

  // Read instruction types.
  Result->InstTypes.resize(buf.ReadIntSpec("INST_TYPE_COUNT:"));
  for (InstTypeInfo &Info : Result->InstTypes) {
    buf.ReadSpec("INST_TYPE:", buffer);
    Info.Name = buffer;
    Info.IsContextDependent = (Info.Name.find("_after_") != string::npos);

    buf.ReadSpec("ISSUE_TYPE:", buffer);
    IssueType issuType = issueTypeByName(*Result, buffer);

    if (issuType == INVALID_ISSUE_TYPE) {
      llvm::report_fatal_error(std::string("Invalid issue type ") + buffer +
                                   " for inst. type " + Info.Name,
                               false);
    }

    Info.IssueType = issuType;
    Info.Latency = (int16_t)buf.ReadIntSpec("LATENCY:");
    Info.Pipelined = buf.ReadFlagSpec("PIPELINED:", true);
    Info.BlocksCycle = buf.ReadFlagSpec("BLOCKS_CYCLE:", false);
    Info.Supported = buf.ReadFlagSpec("SUPPORTED:", true);
  }

  return Result;
}

InstType llvm::opt_sched::instTypeByName(const MachineModel &Model,
                                         llvm::StringRef typeName,
                                         llvm::StringRef prevName) {
  std::string composite =
      prevName.size() ? typeName + std::string("_after_") + prevName : "";
  auto InstTypes = llvm::enumerate(Model.InstTypes);
  auto It = llvm::find_if(InstTypes, [&composite, typeName](const auto &Info) {
    return Info.value().IsContextDependent && Info.value().Name == composite ||
           !Info.value().IsContextDependent && Info.value().Name == typeName;
  });
  if (It != InstTypes.end()) {
    return It->value();
  }
  return INVALID_INST_TYPE;
}

int16_t llvm::opt_sched::regTypeByName(const MachineModel &Model,
                                       llvm::StringRef regTypeName) {
  auto RegTypes = llvm::enumerate(Model.RegisterTypes);
  auto It = llvm::find_if(RegTypes, [regTypeName](const auto &Info) {
    return Info.value().Name == regTypeName;
  });
  assert(It != RegTypes.end() &&
         "No register type with that name in machine model");
  return It->index();
}

IssueType llvm::opt_sched::issueTypeByName(const MachineModel &Model,
                                           llvm::StringRef issuTypeName) {
  auto IssueTypes = llvm::enumerate(Model.IssueTypes);
  auto It = llvm::find_if(IssueTypes, [issuTypeName](const auto &Info) {
    return Info.value().Name == issuTypeName;
  });
  if (It != IssueTypes.end()) {
    return It.index();
  }

  return INVALID_ISSUE_TYPE;
}

bool llvm::opt_sched::isRealInst(const MachineModel &Model,
                                 InstType instTypeCode) {
  return Model.IssueTypes[instTypeCode].Name != "NULL";
}

int16_t llvm::opt_sched::latency(const MachineModel &Model,
                                 InstType instTypeCode,
                                 DependenceType depType) {
  if (depType == DEP_DATA && instTypeCode != INVALID_INST_TYPE) {
    return Model.InstTypes[instTypeCode].Latency;
  } else {
    return Model.DependenceLatencies[depType];
  }
}

bool llvm::opt_sched::isBranch(const MachineModel &Model,
                               InstType instTypeCode) {
  return Model.InstTypes[instTypeCode].Name == "br";
}

bool llvm::opt_sched::isArtificial(const MachineModel &Model,
                                   InstType instTypeCode) {
  return Model.InstTypes[instTypeCode].Name == "artificial";
}

bool llvm::opt_sched::isCall(const MachineModel &Model, InstType instTypeCode) {
  return Model.InstTypes[instTypeCode].Name == "call";
}

bool llvm::opt_sched::isFloat(const MachineModel &Model,
                              InstType instTypeCode) {
  return Model.InstTypes[instTypeCode].Name[0] == 'f';
}

InstType llvm::opt_sched::defaultInstType(const MachineModel &Model) {
  return instTypeByName(Model, "Default");
}

InstType llvm::opt_sched::defaultIssueType(const MachineModel &Model) {
  return issueTypeByName(Model, "Default");
}

bool llvm::opt_sched::isSimple(const MachineModel &Model) {
  return Model.IssueRate == 1 && Model.IssueTypes.size() == 1 &&
         Model.IssueTypes[0].SlotsCount == 1 && !Model.IncludesUnpipelined;
}
