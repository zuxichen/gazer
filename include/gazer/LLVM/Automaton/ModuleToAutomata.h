//==- ModuleToAutomata.h ----------------------------------------*- C++ -*--==//
//
// Copyright 2019 Contributors to the Gazer project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//===----------------------------------------------------------------------===//
#ifndef GAZER_MODULETOAUTOMATA_H
#define GAZER_MODULETOAUTOMATA_H

#include "gazer/Automaton/Cfa.h"
#include "gazer/LLVM/Memory/MemoryObject.h"
#include "gazer/LLVM/Memory/ValueOrMemoryObject.h"
#include "gazer/LLVM/LLVMFrontendSettings.h"
#include "gazer/LLVM/LLVMTraceBuilder.h"

#include <llvm/Pass.h>

#include <variant>

namespace llvm
{
    class Value;
    class Loop;
    class LoopInfo;
}

namespace gazer::llvm2cfa
{
    class GenerationContext;
} // end namespace gazer::llvm2cfa

namespace gazer
{

// Extension points
//==------------------------------------------------------------------------==//
// If a client (such as a memory model) wishes to hook into the CFA generation
// process, it may do so through extension points.
// Each hook will receive one of these extension point objects at the time
// they are called.

namespace llvm2cfa
{

class CfaGenInfo;

class ExtensionPoint
{
protected:
    ExtensionPoint(CfaGenInfo& genInfo)
        : mGenInfo(genInfo)
    {}

public:
    ExtensionPoint(const ExtensionPoint&) = default;
    ExtensionPoint& operator=(const ExtensionPoint&) = delete;

    const Cfa& getCfa() const;

    llvm::Loop* getSourceLoop() const;
    llvm::Function* getSourceFunction() const;

    llvm::Function* getParent() const;

protected:
    CfaGenInfo& mGenInfo;
};

/// This extension can be used to insert additional variables into the CFA at the
/// beginning of the generation process.
class VariableDeclExtensionPoint : public ExtensionPoint
{
public:
    VariableDeclExtensionPoint(CfaGenInfo& genInfo)
        : ExtensionPoint(genInfo)
    {}

    Variable* createInput(ValueOrMemoryObject val, Type& type, const std::string& suffix = "");
    Variable* createLocal(ValueOrMemoryObject val, Type& type, const std::string& suffix = "");

    /// \brief Creates an input variable which will be handled according to the
    /// transformation rules used for PHI nodes.
    Variable* createPhiInput(ValueOrMemoryObject val, Type& type, const std::string& suffix = "");

    /// Marks an already declared variable as output.
    void markOutput(ValueOrMemoryObject val, Variable* variable);
};

/// An extension point which may only query variables from the target automaton.
class AutomatonInterfaceExtensionPoint : public ExtensionPoint
{
public:
    explicit AutomatonInterfaceExtensionPoint(CfaGenInfo& genInfo)
        : ExtensionPoint(genInfo)
    {}

    Variable* getVariableFor(ValueOrMemoryObject val);
    Variable* getInputVariableFor(ValueOrMemoryObject val);
    Variable* getOutputVariableFor(ValueOrMemoryObject val);
};

class GenerationStepExtensionPoint : public AutomatonInterfaceExtensionPoint
{
public:
    using AutomatonInterfaceExtensionPoint::AutomatonInterfaceExtensionPoint;

    Variable* createAuxiliaryVariable(const std::string& name, Type& type);

    virtual ExprPtr getAsOperand(ValueOrMemoryObject val) = 0;

    /// Attempts to inline and eliminate a given variable from the CFA.
    virtual bool tryToEliminate(ValueOrMemoryObject val, Variable* variable, ExprPtr expr) = 0;

    virtual void insertAssignment(Variable* variable, ExprPtr value) = 0;
};

} // end namespace llvm2cfa

// Traceability
//==------------------------------------------------------------------------==//

/// Provides a mapping between CFA locations/variables and LLVM values.
class CfaToLLVMTrace
{
    friend class llvm2cfa::GenerationContext;
public:
    enum LocationKind { Location_Unknown, Location_Entry, Location_Exit };

    struct BlockToLocationInfo
    {
        const llvm::BasicBlock* block;
        LocationKind kind;
    };

    struct ValueMappingInfo
    {
        llvm::DenseMap<ValueOrMemoryObject, ExprPtr> values;
    };

    BlockToLocationInfo getBlockFromLocation(Location* loc) {
        return mLocationsToBlocks.lookup(loc);
    }

    ExprPtr getExpressionForValue(const Cfa* parent, const llvm::Value* value);
    Variable* getVariableForValue(const Cfa* parent, const llvm::Value* value);

private:
    llvm::DenseMap<const Location*, BlockToLocationInfo> mLocationsToBlocks;
    llvm::DenseMap<const Cfa*, ValueMappingInfo> mValueMaps;
};

// LLVM pass
//==------------------------------------------------------------------------==//

/// ModuleToAutomataPass - translates an LLVM IR module into an automata system.
class ModuleToAutomataPass : public llvm::ModulePass
{
public:
    static char ID;

    ModuleToAutomataPass(GazerContext& context, LLVMFrontendSettings settings)
        : ModulePass(ID), mContext(context), mSettings(settings)
    {}

    void getAnalysisUsage(llvm::AnalysisUsage& au) const override;

    bool runOnModule(llvm::Module& module) override;

    llvm::StringRef getPassName() const override {
        return "Module to automata transformation";
    }

    AutomataSystem& getSystem() { return *mSystem; }
    llvm::DenseMap<llvm::Value*, Variable*>& getVariableMap() { return mVariables; }
    CfaToLLVMTrace& getTraceInfo() { return mTraceInfo; }

private:
    std::unique_ptr<AutomataSystem> mSystem;
    llvm::DenseMap<llvm::Value*, Variable*> mVariables;
    CfaToLLVMTrace mTraceInfo;
    GazerContext& mContext;
    LLVMFrontendSettings mSettings;
};

std::unique_ptr<AutomataSystem> translateModuleToAutomata(
    llvm::Module& module,
    LLVMFrontendSettings settings,
    llvm::DenseMap<llvm::Function*, llvm::LoopInfo*>& loopInfos,
    GazerContext& context,
    MemoryModel& memoryModel,
    llvm::DenseMap<llvm::Value*, Variable*>& variables,
    CfaToLLVMTrace& blockEntries
);

llvm::Pass* createCfaPrinterPass();

llvm::Pass* createCfaViewerPass();

}

#endif //GAZER_MODULETOAUTOMATA_H
