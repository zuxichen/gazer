//==- Cfa.h - Control flow automaton interface ------------------*- C++ -*--==//
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
///
/// \file This file defines classes to represent control flow automata, their
/// locations and transitions.
///
//===----------------------------------------------------------------------===//
#ifndef GAZER_AUTOMATON_CFA_H
#define GAZER_AUTOMATON_CFA_H

#include "gazer/Core/Expr.h"

#include <llvm/ADT/GraphTraits.h>
#include <llvm/ADT/DenseMap.h>
#include <boost/iterator/indirect_iterator.hpp>

namespace gazer
{

class Cfa;
class Transition;

class Location
{
    friend class Cfa;
    using EdgeVectorTy = std::vector<Transition*>;
public:
    enum LocationKind
    {
        State,
        Error
    };

private:
    explicit Location(unsigned id, Cfa* parent, LocationKind kind = State)
        : mID(id), mCfa(parent), mKind(kind)
    {}
    
public:
    Location(const Location&) = delete;
    Location& operator=(const Location&) = delete;

    unsigned getId() const { return mID; }

    bool isError() const { return mKind == Error; }

    size_t getNumIncoming() const { return mIncoming.size(); }
    size_t getNumOutgoing() const { return mOutgoing.size(); }

    Cfa* getAutomaton() const { return mCfa; }

    //-------------------------- Iterator support ---------------------------//
    using edge_iterator = EdgeVectorTy::iterator;
    using const_edge_iterator = EdgeVectorTy::const_iterator;

    edge_iterator incoming_begin() { return mIncoming.begin(); }
    edge_iterator incoming_end() { return mIncoming.end(); }
    llvm::iterator_range<edge_iterator> incoming() {
        return llvm::make_range(incoming_begin(), incoming_end());
    }

    edge_iterator outgoing_begin() { return mOutgoing.begin(); }
    edge_iterator outgoing_end() { return mOutgoing.end(); }
    llvm::iterator_range<edge_iterator> outgoing() {
        return llvm::make_range(outgoing_begin(), outgoing_end());
    }

    const_edge_iterator incoming_begin() const { return mIncoming.begin(); }
    const_edge_iterator incoming_end() const { return mIncoming.end(); }
    llvm::iterator_range<const_edge_iterator> incoming() const {
        return llvm::make_range(incoming_begin(), incoming_end());
    }

    const_edge_iterator outgoing_begin() const { return mOutgoing.begin(); }
    const_edge_iterator outgoing_end() const { return mOutgoing.end(); }
    llvm::iterator_range<const_edge_iterator> outgoing() const {
        return llvm::make_range(outgoing_begin(), outgoing_end());
    }

private:
    void addIncoming(Transition* edge);
    void addOutgoing(Transition* edge);

    void removeIncoming(Transition* edge);
    void removeOutgoing(Transition* edge);

private:
    unsigned mID;
    Cfa* mCfa;
    LocationKind mKind;
    EdgeVectorTy mIncoming;
    EdgeVectorTy mOutgoing;
};

/// A simple transition with a guard or summary expression.
class Transition
{
    friend class Cfa;
public:
    enum EdgeKind
    {
        Edge_Assign,    ///< Variable assignment.
        Edge_Call,      ///< Call into another procedure.
    };

protected:
    Transition(Location* source, Location* target, ExprPtr expr, EdgeKind kind)
        : mSource(source), mTarget(target), mExpr(expr), mEdgeKind(kind)
    {
        assert(source != nullptr && "Transition source location must not be null!");
        assert(target != nullptr && "Transition target location must not be null!");
        assert(expr != nullptr && "Transition guard expression must not be null!");
        assert(expr->getType().isBoolType()
            && "Transition guards can only be booleans!");
    }
public:
    Transition(Transition&) = delete;
    Transition& operator=(Transition&) = delete;

    Location* getSource() const { return mSource; }
    Location* getTarget() const { return mTarget; }

    ExprPtr getGuard() const { return mExpr; }
    EdgeKind getKind() const { return mEdgeKind; }

    bool isAssign() const { return mEdgeKind == Edge_Assign; }
    bool isCall() const { return mEdgeKind == Edge_Call; }

    void print(llvm::raw_ostream& os) const;

    virtual ~Transition() = default;

private:
    Location* mSource;
    Location* mTarget;
    ExprPtr mExpr;
    EdgeKind mEdgeKind;
};

/// Represents a (potentially guared) transition with variable assignments.
class AssignTransition final : public Transition
{
    friend class Cfa;
protected:
    AssignTransition(Location* source, Location* target, ExprPtr guard, std::vector<VariableAssignment> assignments);

public:
    using iterator = std::vector<VariableAssignment>::const_iterator;
    iterator begin() const { return mAssignments.begin(); }
    iterator end() const { return mAssignments.end(); }

    size_t getNumAssignments() const { return mAssignments.size(); }
    void addAssignment(VariableAssignment assignment) {
        mAssignments.push_back(assignment);
    }

    static bool classof(const Transition* edge) {
        return edge->getKind() == Edge_Assign;
    }

private:
    std::vector<VariableAssignment> mAssignments;
};

/// Represents a (potentially guarded) transition with a procedure call.
class CallTransition final : public Transition
{
    friend class Cfa;
protected:
    CallTransition(
        Location* source, Location* target,
        ExprPtr guard,
        Cfa* callee,
        std::vector<VariableAssignment> inputArgs,
        std::vector<VariableAssignment> outputArgs
    );

public:
    Cfa* getCalledAutomaton() const { return mCallee; }

    //-------------------------- Iterator support ---------------------------//
    using input_arg_iterator  = std::vector<VariableAssignment>::const_iterator;
    using output_arg_iterator = std::vector<VariableAssignment>::const_iterator;

    input_arg_iterator input_begin() const { return mInputArgs.begin(); }
    input_arg_iterator input_end() const { return mInputArgs.end(); }
    llvm::iterator_range<input_arg_iterator> inputs() const {
        return llvm::make_range(input_begin(), input_end());
    }
    size_t getNumInputs() const { return mInputArgs.size(); }

    output_arg_iterator output_begin() const { return mOutputArgs.begin(); }
    output_arg_iterator output_end() const { return mOutputArgs.end(); }
    llvm::iterator_range<output_arg_iterator> outputs() const {
        return llvm::make_range(output_begin(), output_end());
    }
    size_t getNumOutputs() const { return mOutputArgs.size(); }

    std::optional<VariableAssignment> getInputArgument(Variable& input) const;
    std::optional<VariableAssignment> getOutputArgument(Variable& variable) const;

    static bool classof(const Transition* edge) {
        return edge->getKind() == Edge_Call;
    }

private:
    Cfa* mCallee;
    std::vector<VariableAssignment> mInputArgs;
    std::vector<VariableAssignment> mOutputArgs;
};

class AutomataSystem;

/// Represents a control flow automaton.
class Cfa final
{
    friend class AutomataSystem;
private:
    Cfa(GazerContext& context, std::string name, AutomataSystem& parent);

public:
    Cfa(const Cfa&) = delete;
    Cfa& operator=(const Cfa&) = delete;

public:
    //===----------------------------------------------------------------------===//
    // Locations and edges
    Location* createLocation();
    Location* createErrorLocation();

    AssignTransition* createAssignTransition(
        Location* source, Location* target, ExprPtr guard = nullptr, std::vector<VariableAssignment> assignments = {}
    );

    AssignTransition* createAssignTransition(
        Location* source, Location* target, std::vector<VariableAssignment> assignments
    ) {
        return createAssignTransition(source, target, nullptr, assignments);
    }

    CallTransition* createCallTransition(
        Location* source, Location* target, const ExprPtr& guard,
        Cfa* callee, std::vector<VariableAssignment> inputArgs, std::vector<VariableAssignment> outputArgs
    );

    CallTransition* createCallTransition(
        Location* source, Location* target,
        Cfa* callee, std::vector<VariableAssignment> inputArgs, std::vector<VariableAssignment> outputArgs
    );

    //===----------------------------------------------------------------------===//
    // Variable handling

    Variable* createInput(const std::string& name, Type& type);
    Variable* createLocal(const std::string& name, Type& type);

    /// Marks an already existing variable as an output.
    void addOutput(Variable* variable);

    void addErrorCode(Location* location, ExprPtr errorCodeExpr);
    ExprPtr getErrorFieldExpr(Location* location);

    //===----------------------------------------------------------------------===//
    // Iterator support
    using node_iterator = std::vector<std::unique_ptr<Location>>::iterator;
    using const_node_iterator = std::vector<std::unique_ptr<Location>>::const_iterator;

    node_iterator node_begin() { return mLocations.begin(); }
    node_iterator node_end() { return mLocations.end(); }

    const_node_iterator node_begin() const { return mLocations.begin(); }
    const_node_iterator node_end() const { return mLocations.end(); }
    llvm::iterator_range<node_iterator> nodes() {
        return llvm::make_range(node_begin(), node_end());
    }
    llvm::iterator_range<const_node_iterator> nodes() const {
        return llvm::make_range(node_begin(), node_end());
    }

    // Iterator for error locations
    using error_iterator = llvm::SmallDenseMap<Location*, ExprPtr, 1>::const_iterator;

    error_iterator error_begin() const { return mErrorFieldExprs.begin(); }
    error_iterator error_end() const { return mErrorFieldExprs.end(); }
    llvm::iterator_range<error_iterator> errors() const {
        return llvm::make_range(error_begin(), error_end());
    }

    size_t getNumErrors() const { return mErrorFieldExprs.size(); }

    // Transition (edge) iterators...
    using edge_iterator = std::vector<std::unique_ptr<Transition>>::iterator;
    using const_edge_iterator = std::vector<std::unique_ptr<Transition>>::const_iterator;

    edge_iterator edge_begin() { return mTransitions.begin(); }
    edge_iterator edge_end() { return mTransitions.end(); }

    const_edge_iterator edge_begin() const { return mTransitions.begin(); }
    const_edge_iterator edge_end() const { return mTransitions.end(); }
    llvm::iterator_range<edge_iterator> edges() {
        return llvm::make_range(edge_begin(), edge_end());
    }
    llvm::iterator_range<const_edge_iterator> edges() const {
        return llvm::make_range(edge_begin(), edge_end());
    }

    // Variable iterators
    using var_iterator = boost::indirect_iterator<std::vector<Variable*>::iterator>;

    var_iterator input_begin() { return mInputs.begin(); }
    var_iterator input_end() { return mInputs.end(); }
    llvm::iterator_range<var_iterator> inputs() {
        return llvm::make_range(input_begin(), input_end());
    }

    var_iterator output_begin() { return mOutputs.begin(); }
    var_iterator output_end() { return mOutputs.end(); }
    llvm::iterator_range<var_iterator> outputs() {
        return llvm::make_range(output_begin(), output_end());
    }

    var_iterator local_begin() { return mLocals.begin(); }
    var_iterator local_end() { return mLocals.end(); }
    llvm::iterator_range<var_iterator> locals() {
        return llvm::make_range(local_begin(), local_end());
    }

    //===----------------------------------------------------------------------===//
    // Others
    llvm::StringRef getName() const { return mName; }
    Location* getEntry() const { return mEntry; }
    Location* getExit() const { return mExit; }

    AutomataSystem& getParent() const { return mParent; }

    size_t getNumLocations() const { return mLocations.size(); }
    size_t getNumTransitions() const { return mTransitions.size(); }

    size_t getNumInputs() const { return mInputs.size(); }
    size_t getNumOutputs() const { return mOutputs.size(); }
    size_t getNumLocals() const { return mLocals.size(); }

    /// Returns the index of a given input variable in the input list of this automaton.

    size_t getInputNumber(Variable* variable) const;
    size_t getOutputNumber(Variable* variable) const;

    Variable* findInputByName(llvm::StringRef name) const;
    Variable* findLocalByName(llvm::StringRef name) const;
    Variable* findOutputByName(llvm::StringRef name) const;

    Variable* getInput(size_t i) const { return mInputs[i]; }
    Variable* getOutput(size_t i) const { return mOutputs[i]; }

    Location* findLocationById(unsigned id);

    bool isOutput(Variable* variable) const;

    /// View the graph representation of this CFA with the
    /// system's default GraphViz viewer.
    void view() const;

    void print(llvm::raw_ostream& os) const;
    void printDeclaration(llvm::raw_ostream& os) const;

    //------------------------------ Deletion -------------------------------//
    void removeUnreachableLocations();

    void disconnectLocation(Location* location);
    void disconnectEdge(Transition* edge);

    void clearDisconnectedElements();

    template<class Predicate>
    void removeLocalsIf(Predicate p) {
        mLocals.erase(
            std::remove_if(mLocals.begin(), mLocals.end(), p),
            mLocals.end()
        );
    }

    ~Cfa();

private:
    Variable* createMemberVariable(const std::string& name, Type& type);
    Variable* findVariableByName(const std::vector<Variable*>& vec, llvm::StringRef name) const;

private:
    std::string mName;
    GazerContext& mContext;
    AutomataSystem& mParent;

    std::vector<std::unique_ptr<Location>> mLocations;
    std::vector<std::unique_ptr<Transition>> mTransitions;

    llvm::SmallVector<Location*, 1> mErrorLocations;
    llvm::SmallDenseMap<Location*, ExprPtr, 1> mErrorFieldExprs;

    std::vector<Variable*> mInputs;
    std::vector<Variable*> mOutputs;
    std::vector<Variable*> mLocals;

    Location* mEntry;
    Location* mExit;

    llvm::DenseMap<Variable*, std::string> mSymbolNames;
    llvm::DenseMap<unsigned, Location*> mLocationNumbers;

    Cfa* mParentAutomaton = nullptr;

    unsigned mLocationIdx = 0;
    unsigned mTmp = 0;
};

/// A system of CFA instances.
class AutomataSystem final
{
public:
    explicit AutomataSystem(GazerContext& context);

    AutomataSystem(const AutomataSystem&) = delete;
    AutomataSystem& operator=(const AutomataSystem&) = delete;

public:
    Cfa* createCfa(std::string name);

    using iterator = boost::indirect_iterator<std::vector<std::unique_ptr<Cfa>>::iterator>;
    using const_iterator = boost::indirect_iterator<std::vector<std::unique_ptr<Cfa>>::const_iterator>;

    iterator begin() { return mAutomata.begin(); }
    iterator end() { return mAutomata.end(); }

    const_iterator begin() const { return mAutomata.begin(); }
    const_iterator end() const { return mAutomata.end(); }

    GazerContext& getContext() { return mContext; }

    size_t getNumAutomata() const { return mAutomata.size(); }
    Cfa* getAutomatonByName(llvm::StringRef name) const;

    Cfa* getMainAutomaton() const { return mMainAutomaton; }
    void setMainAutomaton(Cfa* cfa);

    void print(llvm::raw_ostream& os) const;

private:
    GazerContext& mContext;
    std::vector<std::unique_ptr<Cfa>> mAutomata;
    Cfa* mMainAutomaton;
};

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const Transition& transition)
{
    transition.print(os);
    return os;
}

} // end namespace gazer


// GraphTraits specialization for automata
//-------------------------------------------------------------------------
namespace llvm
{

template<>
struct GraphTraits<gazer::Cfa>
{
    using NodeRef = gazer::Location*;
    using EdgeRef = gazer::Transition*;

    static constexpr auto GetEdgeTarget = [](const gazer::Transition* edge) -> NodeRef  {
        return edge->getTarget();
    };
    static constexpr auto GetLocationFromPtr = [](const std::unique_ptr<gazer::Location>& loc)
        -> gazer::Location*
    {
        return loc.get();
    };

    // Child traversal
    using ChildIteratorType = llvm::mapped_iterator<
        gazer::Location::edge_iterator,
        decltype(GetEdgeTarget),
        NodeRef
    >;

    static ChildIteratorType child_begin(NodeRef loc) {
        return ChildIteratorType(loc->outgoing_begin(), GetEdgeTarget);
    }
    static ChildIteratorType child_end(NodeRef loc) {
        return ChildIteratorType(loc->outgoing_end(), GetEdgeTarget);
    }

    using nodes_iterator = llvm::mapped_iterator<
        gazer::Cfa::const_node_iterator, decltype(GetLocationFromPtr)
    >;

    static nodes_iterator nodes_begin(const gazer::Cfa& cfa) {
        return nodes_iterator(cfa.node_begin(), GetLocationFromPtr);
    }
    static nodes_iterator nodes_end(const gazer::Cfa& cfa) {
        return nodes_iterator(cfa.node_end(), GetLocationFromPtr);
    }

    static NodeRef getEntryNode(const gazer::Cfa& cfa) {
        return cfa.getEntry();
    }

    // Edge traversal
    using ChildEdgeIteratorType = gazer::Location::edge_iterator;
    static ChildEdgeIteratorType child_edge_begin(NodeRef loc) {
        return loc->outgoing_begin();
    }
    static ChildEdgeIteratorType child_edge_end(NodeRef loc) {
        return loc->outgoing_end();
    }
    static NodeRef edge_dest(EdgeRef edge) {
        return edge->getTarget();
    }

    static unsigned size(gazer::Cfa& cfa) {
        return cfa.getNumLocations();
    }
};

template<>
struct GraphTraits<Inverse<gazer::Cfa>> : public GraphTraits<gazer::Cfa>
{
    using NodeRef = gazer::Location*;

    static constexpr auto GetEdgeSource = [](const gazer::Transition* edge) -> NodeRef  {
        return edge->getSource();
    };

    using ChildIteratorType = llvm::mapped_iterator<gazer::Location::edge_iterator, decltype(GetEdgeSource)>;

    static ChildIteratorType child_begin(NodeRef loc) {
        return ChildIteratorType(loc->outgoing_begin(), GetEdgeSource);
    }
    static ChildIteratorType child_end(NodeRef loc) {
        return ChildIteratorType(loc->outgoing_end(), GetEdgeSource);
    }

    static NodeRef getEntryNode(Inverse<gazer::Cfa>& cfa) { return cfa.Graph.getExit(); }
};

template<>
struct GraphTraits<gazer::Location*>
{
    using NodeRef = gazer::Location*;

    static constexpr auto GetEdgeTarget = [](gazer::Transition* edge) -> gazer::Location*  {
        return edge->getTarget();
    };

    // Child traversal
    using ChildIteratorType = llvm::mapped_iterator<
        gazer::Location::edge_iterator,
        decltype(GetEdgeTarget),
        NodeRef
    >;

    static ChildIteratorType child_begin(NodeRef loc) {
        return ChildIteratorType(loc->outgoing_begin(), GetEdgeTarget);
    }
    static ChildIteratorType child_end(NodeRef loc) {
        return ChildIteratorType(loc->outgoing_end(), GetEdgeTarget);
    }

    static NodeRef getEntryNode(gazer::Location* loc) { return loc; }
};

template<>
struct GraphTraits<Inverse<gazer::Location*>> : public GraphTraits<gazer::Location*>
{
    using NodeRef = gazer::Location*;

    using ChildIteratorType = llvm::mapped_iterator<
        gazer::Location::edge_iterator,
        decltype(GraphTraits<Inverse<gazer::Cfa>>::GetEdgeSource)
    >;

    static ChildIteratorType child_begin(NodeRef cp) {
        return ChildIteratorType(cp->incoming_begin(), GraphTraits<Inverse<gazer::Cfa>>::GetEdgeSource);
    }
    static ChildIteratorType child_end(NodeRef cp) {
        return ChildIteratorType(cp->incoming_end(), GraphTraits<Inverse<gazer::Cfa>>::GetEdgeSource);
    }

    static NodeRef getEntryNode(Inverse<gazer::Location*> loc) { return loc.Graph; }
};

} // end namespace llvm

#endif

