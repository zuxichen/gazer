//==-------------------------------------------------------------*- C++ -*--==//
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
#ifndef GAZER_MEMORY_MEMORYMODEL_H
#define GAZER_MEMORY_MEMORYMODEL_H

#include "gazer/LLVM/Memory/MemorySSA.h"

#include <llvm/Analysis/PtrUseVisitor.h>

namespace llvm
{
    class Loop;
} // end namespace llvm

namespace gazer
{

class Cfa;

namespace llvm2cfa
{

class VariableDeclExtensionPoint;
class GenerationStepExtensionPoint;
class AutomatonInterfaceExtensionPoint;

} // end namespace llvm2cfa

/// An interface for memory models.
class MemoryModel
{
public:
    struct CallParam
    {
        CallParam(MemoryObjectDef* formal, MemoryObjectDef* actual)
            : formal(formal), actual(actual)
        {}

        MemoryObjectDef* formal;
        MemoryObjectDef* actual;
    };

public:
    MemoryModel(
        GazerContext& context,
        LLVMFrontendSettings settings,
        const llvm::DataLayout& dl
    )
        : mContext(context),
        mTypes(*this, settings.ints),
        mSettings(settings),
        mDataLayout(dl)
    {}

    MemoryModel(const MemoryModel&) = delete;
    MemoryModel& operator=(const MemoryModel&) = delete;

    /// Initializes this memory model for a specific module.
    /// \param module The LLVM module to initialize this memory model for.
    /// \param getDomTree A callable which returns the dominator tree for each function.
    void initialize(llvm::Module& module);

    /// Declares all input/output/local variables that should be inserted into the CFA.
    virtual void declareProcedureVariables(llvm2cfa::VariableDeclExtensionPoint& extensionPoint) = 0;

    virtual ExprPtr handlePointerCast(const llvm::CastInst& cast, ExprPtr origPtr) = 0;

    /// Handles an arbitrary pointer value.
    /// The second operand allows function-specific lookups.
    virtual ExprPtr handlePointerValue(const llvm::Value* value, llvm::Function& parent) = 0;

    /// Translates a given memory object definition into an assignment.
    //virtual std::optional<VariableAssignment> handleMemoryObjectDef(const MemoryObjectDef* def) = 0;
    //virtual std::optional<VariableAssignment> handleMemoryObjectUse(const MemoryObjectUse* use) = 0;

    virtual ExprPtr handleLiveOnEntry(
        memory::LiveOnEntryDef* def,
        llvm2cfa::GenerationStepExtensionPoint& ep
    );

    virtual ExprPtr handleGlobalInitializer(
        memory::GlobalInitializerDef* def,
        ExprPtr pointer,
        llvm2cfa::GenerationStepExtensionPoint& ep
    ) = 0;
    
    /// Translates the given LoadInst into an assignable expression.
    virtual ExprPtr handleLoad(
        const llvm::LoadInst& load,
        ExprPtr pointer,
        llvm2cfa::GenerationStepExtensionPoint& ep
    ) = 0;

    /// Translates an alloca instruction into an assignable expression.
    virtual ExprPtr handleAlloca(
        const llvm::AllocaInst& alloc,
        llvm2cfa::GenerationStepExtensionPoint& ep
    ) = 0;

    virtual void handleCall(
        llvm::ImmutableCallSite call,
        llvm2cfa::GenerationStepExtensionPoint& callerEp,
        llvm2cfa::AutomatonInterfaceExtensionPoint& calleeEp,
        std::vector<VariableAssignment>& inputAssignments,
        std::vector<VariableAssignment>& outputAssignments,
        std::vector<VariableAssignment>& additionalAssignments
    ) = 0;

    virtual ExprPtr handleGetElementPtr(
        const llvm::GetElementPtrInst& gep,
        llvm::ArrayRef<ExprPtr> ops
    ) = 0;

    virtual void handleStore(
        const llvm::StoreInst& store,
        ExprPtr pointer,
        ExprPtr value,
        llvm2cfa::GenerationStepExtensionPoint& ep
    ) = 0;

    virtual void handleBlock(
        const llvm::BasicBlock& bb,
        llvm2cfa::GenerationStepExtensionPoint& ep
    ) = 0;

    virtual gazer::Type& handlePointerType(const llvm::PointerType* type) = 0;

    /// Translates type for constant arrays and initializers.
    virtual gazer::Type& handleArrayType(const llvm::ArrayType* type) = 0;

    virtual ExprPtr handleConstantDataArray(
        const llvm::ConstantDataArray* cda,
        llvm::ArrayRef<ExprRef<LiteralExpr>> elements
    ) = 0;

    GazerContext& getContext() { return mContext; }
    const llvm::DataLayout& getDataLayout() const { return mDataLayout; }

    gazer::Type& translateType(const llvm::Type* type) { return mTypes.get(type); }
    memory::MemorySSA* getFunctionMemorySSA(const llvm::Function& function) {
        return mFunctions[&function].get();
    }

    virtual void dump() const {};

    virtual ~MemoryModel() {}

protected:
    /// Fills \p builder with all memory objects, their (possible) definitons and uses.
    virtual void initializeFunction(llvm::Function& function, memory::MemorySSABuilder& builder) = 0;

protected:
    GazerContext& mContext;
    LLVMTypeTranslator mTypes;
    LLVMFrontendSettings mSettings;
    const llvm::DataLayout& mDataLayout;

private:
    std::unordered_map<const llvm::Function*, std::unique_ptr<memory::MemorySSA>> mFunctions;
};

class CollectMemoryDefsUsesVisitor : public llvm::PtrUseVisitor<CollectMemoryDefsUsesVisitor>
{
    friend class llvm::PtrUseVisitor<CollectMemoryDefsUsesVisitor>;
    friend class llvm::InstVisitor<CollectMemoryDefsUsesVisitor>;
public:
    explicit CollectMemoryDefsUsesVisitor(const llvm::DataLayout& dl, MemoryObject* obj, memory::MemorySSABuilder& builder)
        : PtrUseVisitor(dl), mObject(obj), mBuilder(builder)
    {}

private:
    void visitStoreInst(llvm::StoreInst& store);
    void visitLoadInst(llvm::LoadInst& load);

    void visitCallInst(llvm::CallInst& call);
    void visitPHINode(llvm::PHINode& phi);
    void visitSelectInst(llvm::SelectInst& select);
    void visitInstruction(llvm::Instruction& inst);

private:
    MemoryObject* mObject;
    memory::MemorySSABuilder& mBuilder;
};

//==-----------------------------------------------------------------------==//
/// DummyMemoryModel - A havoc memory model which does not create any memory
/// objects. oad operations return an unknown value and store instructions
/// have no effect. No MemoryObjectPhis are inserted.
std::unique_ptr<MemoryModel> CreateHavocMemoryModel(
    GazerContext& context,
    LLVMFrontendSettings& settings,
    const llvm::DataLayout& dl
);

//==-----------------------------------------------------------------------==//
// BasicMemoryModel - a simple memory model which handles local arrays,
// structs, and globals which do not have their address taken. This memory
// model returns undef for all heap operations.
std::unique_ptr<MemoryModel> CreateBasicMemoryModel(
    GazerContext& context,
    const LLVMFrontendSettings& settings,
    const llvm::DataLayout& dl
);

//==-----------------------------------------------------------------------==//
// FlatMemoryModel
std::unique_ptr<MemoryModel> CreateFlatMemoryModel(
    GazerContext& context,
    const LLVMFrontendSettings& settings,
    const llvm::DataLayout& dl
);

}
#endif //GAZER_MEMORY_MEMORYMODEL_H
