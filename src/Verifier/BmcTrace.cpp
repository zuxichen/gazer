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
#include "BoundedModelCheckerImpl.h"

#include <llvm/Support/raw_ostream.h>

using namespace gazer;

void bmc::cex_iterator::advance()
{
    auto pred = mCex.mPredecessors.get(mState.getLocation());
    if (!pred) {
        // No predcessor information is available, this was the end of the counterexample trace.
        mState = { nullptr, nullptr };
        return;
    }

    Location* current = mState.getLocation();
    //auto predLit = llvm::dyn_cast_or_null<BvLiteralExpr>(
    //    mCex.mEval.walk(pred->second).get()
    //);
    auto predLit = llvm::dyn_cast_or_null<IntLiteralExpr>(
        mCex.mEval.walk(pred->second).get()
    );
                            
    assert((predLit != nullptr) && "Pred values should be evaluatable as integer literals!");
    
    size_t predId = predLit->getValue();//.getLimitedValue();
    Location* source = mCex.mCfa.findLocationById(predId);

    assert(source != nullptr && "Locations should be findable by their id!");

    auto edge = std::find_if(
        current->incoming_begin(),
        current->incoming_end(),
        [source](Transition* e) { return e->getSource() == source; }
    );

    assert(edge != current->incoming_end()
        && "There must be an edge between a location and its direct predecessor!");

    mState = { source, *edge };
}

// FIXME: Move this to BoundedModelChecker.cpp?
std::unique_ptr<VerificationResult> BoundedModelCheckerImpl::createFailResult()
{               
    auto model = mSolver->getModel();
    ExprEvaluator eval{model};

    if (mSettings.dumpSolverModel) {
        model.print(llvm::errs());
    }

    std::unique_ptr<Trace> trace;
    if (mSettings.trace) {
        std::vector<Location*> states;
        std::vector<std::vector<VariableAssignment>> actions;

        bmc::BmcCex cex{mError, *mRoot, eval, mPredecessors};
        for (auto state : cex) {
            Location* loc = state.getLocation();
            Transition* edge = state.getOutgoingTransition();

            Location* origLoc = mInlinedLocations.lookup(loc);

            states.push_back(origLoc != nullptr ? origLoc : loc);

            if (edge == nullptr) {
                continue;
            }

            auto assignEdge = llvm::dyn_cast<AssignTransition>(edge);
            assert(assignEdge != nullptr && "BMC traces must contain only assign transitions!");

            std::vector<VariableAssignment> traceAction;
            for (const VariableAssignment& assignment : *assignEdge) {
                Variable* variable = assignment.getVariable();
                Variable* origVariable = mInlinedVariables.lookup(assignment.getVariable());
                if (origVariable == nullptr) {
                    // This variable was not inlined, just use the original one.
                    origVariable = variable;
                }

                ExprRef<AtomicExpr> value;
                if (model.find(assignment.getVariable()) == model.end()) {
                    value = UndefExpr::Get(variable->getType());
                } else {
                    value = eval.walk(assignment.getVariable()->getRefExpr());
                }

                traceAction.emplace_back(origVariable, value);
            }

            actions.push_back(traceAction);
        }

        std::reverse(states.begin(), states.end());
        std::reverse(actions.begin(), actions.end());

        trace = mTraceBuilder.build(states, actions);
    } else {
        trace = std::make_unique<Trace>(std::vector<std::unique_ptr<TraceEvent>>());
    }

    ExprRef<LiteralExpr> errorExpr = eval.walk(mErrorFieldVariable->getRefExpr());
    assert(errorExpr != nullptr && "The error field must be present in the model as a literal expression!");

    switch (errorExpr->getType().getTypeID()) {
        case Type::BvTypeID:
            return VerificationResult::CreateFail(llvm::cast<BvLiteralExpr>(errorExpr)->getValue().getLimitedValue(), std::move(trace));
        case Type::IntTypeID:
            return VerificationResult::CreateFail(llvm::cast<IntLiteralExpr>(errorExpr)->getValue(), std::move(trace));
        default:
            llvm_unreachable("Invalid error field type!");
    }
}
