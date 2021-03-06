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
#include "gazer/Core/Expr/ExprBuilder.h"
#include "gazer/Core/ExprTypes.h"
#include "gazer/Core/LiteralExpr.h"
#include "gazer/Core/Expr/Matcher.h"
#include "gazer/Core/Expr/ConstantFolder.h"

#include <llvm/ADT/DenseMap.h>

using namespace gazer;
using namespace gazer::PatternMatch;

using llvm::dyn_cast;

namespace
{

class FoldingExprBuilder : public ExprBuilder
{
public:
    FoldingExprBuilder(GazerContext& context)
        : ExprBuilder(context)
    {}

    ExprPtr Not(const ExprPtr& op) override
    {
        // Not(Not(X)) --> X
        if (op->getKind() == Expr::Not) {
            return llvm::cast<NotExpr>(op.get())->getOperand();
        }

        ExprPtr e1 = nullptr, e2 = nullptr;

        // Not(Eq(E1, E2)) --> NotEq(E1, E2)
        if (match(op, m_Eq(m_Expr(e1), m_Expr(e2)))) {
            return ConstantFolder::NotEq(e1, e2);
        }

        // Not(NotEq(E1, E2)) --> Eq(E1, E2)
        if (match(op, m_NotEq(m_Expr(e1), m_Expr(e2)))) {
            return ConstantFolder::Eq(e1, e2);
        }

        // Not(LESSTHAN(E1, E2)) --> GREATERTHANEQ(E1, E2)
        if (match(op, m_BvULt(m_Expr(e1), m_Expr(e2)))) {
            return ConstantFolder::BvUGtEq(e1, e2);
        }

        if (match(op, m_BvSLt(m_Expr(e1), m_Expr(e2)))) {
            return ConstantFolder::BvSGtEq(e1, e2);
        }

        if (match(op, m_Lt(m_Expr(e1), m_Expr(e2)))) {
            return this->GtEq(e1, e2);
        }

        return ConstantFolder::Not(op);
    }

    ExprPtr ZExt(const ExprPtr& op, BvType& type) override {
        return ConstantFolder::ZExt(op, type);
    }
    
    ExprPtr SExt(const ExprPtr& op, BvType& type) override {
        return ConstantFolder::SExt(op, type);
    }

    ExprPtr Trunc(const ExprPtr& op, BvType& type) override {
        return this->Extract(op, 0, type.getWidth());
    }

    ExprPtr Extract(const ExprPtr& op, unsigned offset, unsigned width) override
    {
        ExprRef<> x1, x2;

        if (offset == 0) {
            // Extract(SRem(SExt(X1), SExt(X2)), 0, w) --> SRem(X1, X2) if width(X1) == width(X2) == w
            if (match(op, m_BvSRem(m_SExt(m_Expr(x1)), m_SExt(m_Expr(x2))))) {
                assert(x1->getType().isBvType() && x2->getType().isBvType());
                auto& bvTy = *llvm::cast<BvType>(&x1->getType());

                if (bvTy.getWidth() == width) {
                    return ConstantFolder::BvSRem(x1, x2);
                }
            }
        }

        return ConstantFolder::Extract(op, offset, width);
    }

    ExprPtr Add(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::Add(left, right);
    }

    ExprPtr Sub(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::Sub(left, right);
    }

    ExprPtr Mul(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::Mul(left, right);
    }

    ExprPtr BvSDiv(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::BvSDiv(left, right);
    }

    ExprPtr BvUDiv(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::BvUDiv(left, right);
    }

    ExprPtr BvSRem(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::BvSRem(left, right);
    }

    ExprPtr BvURem(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::BvURem(left, right);
    }

    ExprPtr Shl(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::Shl(left, right);
    }

    ExprPtr LShr(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::LShr(left, right);
    }

    ExprPtr AShr(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::AShr(left, right);
    }

    ExprPtr BvAnd(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::BvAnd(left, right);
    }

    ExprPtr BvOr(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::BvOr(left, right);
    }

    ExprPtr BvXor(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::BvXor(left, right);
    }

    ExprPtr And(const ExprVector& vector) override
    {
        ExprVector newOps;

        for (const ExprPtr& op : vector) {
            if (op->getKind() == Expr::Literal) {
                auto lit = llvm::dyn_cast<BoolLiteralExpr>(op.get());

                assert(lit != nullptr && "Operands for ANDs should be booleans!");
                if (lit->getValue() == false) { //NOLINT
                    return this->False();
                } else {
                    // We are not adding unnecessary true literals
                }
            } else if (op->getKind() == Expr::And) {
                // For AndExpr operands, we flatten the expression
                auto andExpr = llvm::dyn_cast<AndExpr>(op.get());    
                newOps.insert(newOps.end(), andExpr->op_begin(), andExpr->op_end());
            } else {
                newOps.push_back(op);
            }
        }

        if (newOps.size() == 0) {
            // If we eliminated all operands
            return this->True();
        } else if (newOps.size() == 1) {
            return *newOps.begin();
        }

        ExprRef<> x1;
        ExprRef<> e1, e2, e3;

        // And(Eq(E1, E2), NotEq(E1, E2)) --> False
        if (unord_match(newOps, m_Eq(m_Expr(e1), m_Expr(e2)), m_NotEq(m_Specific(e1), m_Specific(e2)))) {
            return this->False();
        }

        if (newOps.size() == 2) {
            ExprPtr lhs = newOps[0];
            ExprPtr rhs = newOps[1];

            // And(Not(X), X) --> False
            if (unord_match(lhs, rhs, m_Not(m_Expr(e1)), m_Specific(e1))) {
                return this->False();
            }

            // And(Or(E1, E2), Or(E1, E3)) --> And(E1, Or(E2, E3))
            if (unord_match(lhs, rhs, m_Or(m_Expr(e1), m_Expr(e2)), m_Or(m_Specific(e1), m_Expr(e3)))) {
                newOps[0] = e1;
                newOps[1] = this->Or({e2, e3});
            }
        }

        return AndExpr::Create(newOps);
    }

    ExprPtr Or(const ExprVector& vector) override
    {
        ExprVector newOps;

        for (const ExprPtr& op : vector) {
            if (op->getKind() == Expr::Literal) {
                auto lit = llvm::dyn_cast<BoolLiteralExpr>(op.get());
                if (lit->getValue() == true) {
                    return this->True();
                } else {
                    // We are not adding unnecessary false literals
                }
            } else if (op->getKind() == Expr::Or) {
                // For OrExpr operands, we try to flatten the expression
                auto orExpr = llvm::dyn_cast<OrExpr>(op.get());    
                newOps.insert(newOps.end(), orExpr->op_begin(), orExpr->op_end());
            } else {
                newOps.push_back(op);
            }
        }

        if (newOps.size() == 0) {
            // If we eliminated all operands
            return this->False();
        } else if (newOps.size() == 1) {
            return *newOps.begin();
        }

        ExprRef<VarRefExpr> x1;
        ExprRef<> e1, e2, e3;

        // Try some optimizations for the binary case
        if (newOps.size() == 2) {
            ExprPtr lhs = newOps[0];
            ExprPtr rhs = newOps[1];

            // Or(Not(X), X) --> True
            if (unord_match(lhs, rhs, m_Not(m_Expr(e1)), m_Specific(e1))) {
                return this->True();
            }

            // Or(And(E1, E2), And(E1, E3)) --> And(E1, Or(E2, E3))
            if (unord_match(lhs, rhs, m_And(m_Expr(e1), m_Expr(e2)), m_And(m_Specific(e1), m_Expr(e3)))) {
                newOps[0] = e1;
                newOps[1] = this->Or({e2, e3});
            }
        }

        return OrExpr::Create(newOps);
    }

    ExprPtr Xor(const ExprPtr& left, const ExprPtr& right) override
    {
        // Xor(True, E1) --> Not(E1)
        // Xor(False, E1) --> E1
        if (left == this->True()) {
            return this->Not(right);
        } else if (right == this->True()) {
            return this->Not(left);
        } else if (left == this->False()) {
            return right;
        } else if (right == this->False()) {
            return left;
        }

        return XorExpr::Create(left, right);
    }

    ExprPtr Imply(const ExprPtr& left, const ExprPtr& right) override
    {
        return ImplyExpr::Create(left, right);
    }

    ExprPtr Eq(const ExprPtr& left, const ExprPtr& right) override
    {
        if (left == right) {
            return this->True();
        }

        ExprRef<BoolLiteralExpr> b1 = nullptr;
        ExprPtr c1 = nullptr;
        ExprPtr e1 = nullptr, e2 = nullptr;

        // Eq(True, X) --> X
        // Eq(False, X) --> Not(X)
        if (unord_match(left, right, m_BoolLit(b1), m_Expr(e1))) {
            if (b1->isTrue()) {
                return e1;
            }

            return this->Not(e1);
        }

        // Eq(Select(C1, E1, E2), E1) --> C1
        if (unord_match(left, right, m_Select(m_Expr(c1), m_Expr(e1), m_Expr(e2)), m_Specific(e1))) {
            return c1;
        }

        // Eq(Select(C1, E1, E2), E2) --> Not(C1)
        if (unord_match(left, right, m_Select(m_Expr(c1), m_Expr(e1), m_Expr(e2)), m_Specific(e2))) {
            return this->Not(c1);
        }

        llvm::APInt i1, i2;

        // Eq(ZExt.W(E1), C1) --> Eq(E1, C1) if width(E1) >= width(c1)
        if (unord_match(left, right, m_ZExt(m_Expr(e1)), m_Bv(&i1))) {
            auto& bvTy = llvm::cast<BvType>(e1->getType());

            if (i1.getActiveBits() <= bvTy.getWidth()) {
                return ConstantFolder::Eq(e1, this->BvLit(i1.zextOrTrunc(bvTy.getWidth())));
            }
        }

        return ConstantFolder::Eq(left, right);
    }

    ExprPtr NotEq(const ExprPtr& left, const ExprPtr& right) override
    {
        if (left == right) {
            return this->False();
        }

        ExprRef<BoolLiteralExpr> b1 = nullptr;
        ExprPtr e1 = nullptr, e2 = nullptr, e3 = nullptr;

        // NotEq(True, X) --> Not(X)
        // NotEq(False, X) --> X
        if (unord_match(left, right, m_BoolLit(b1), m_Expr(e1))) {
            if (b1->isTrue()) {
                return this->Not(e1);
            }

            return e1;
        }

        ExprRef<> x1 = nullptr, x2 = nullptr;

        // NotEq(Select(NotEq(X1, X2), E1, E2), E1) --> Eq(X)
        // NotEq(Select(NotEq(X1, X2), E1, E2), E2) --> NotEq(X)
        if (unord_match(left, right,
            m_Select(
                m_NotEq(m_Expr(x1), m_Expr(x2)),
                m_Expr(e1),
                m_Expr(e2)
            ),
            m_Expr(e3))
        ) {
            if (e3 == e1) {
                return ConstantFolder::Eq(x1, x2);
            } else if (e3 == e2) {
                return ConstantFolder::NotEq(x1, x2);
            }
        }

        llvm::APInt l1;

        // NotEq(ZExt(X1), 0) --> NotEq(X1, 0)
        if (unord_match(left, right, m_ZExt(m_Expr(x1)), m_Bv(&l1)) && l1 == 0) {
            auto& bvTy = *llvm::cast<BvType>(&x1->getType());
            return ConstantFolder::NotEq(x1, this->BvLit(0, bvTy.getWidth()));
        }

        return ConstantFolder::NotEq(left, right);
    }

    ExprPtr Lt(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::Lt(left, right);
    }

    ExprPtr LtEq(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::LtEq(left, right);
    }

    ExprPtr Gt(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::Gt(left, right);
    }

    ExprPtr GtEq(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::GtEq(left, right);
    }

    // Define a little helper for comparison operators.
    // Note that this simplification does not work for unsigned bitvector operations.
    // As an example, a + b u> c --> a u> a - b is not a valid transformation
    // if (a - b) underflows.
    #define COMPARE_ARITHMETIC_SIMPLIFY(OPCODE)                                     \
        ExprRef<> x;                                                                \
        llvm::APInt c1, c2;                                                         \
                                                                                    \
        /* CMP(Add(X, C1), C2) --> CMP(X, C2 - C1) */                               \
        if (unord_match(left, right, m_Add(m_Bv(&c1), m_Expr(x)), m_Bv(&c2))) {     \
            return ConstantFolder::OPCODE(x, this->BvLit(c2 - c1));                 \
        }                                                                           \

    ExprPtr BvSLt(const ExprPtr& left, const ExprPtr& right) override
    {
        COMPARE_ARITHMETIC_SIMPLIFY(BvSLt)

        return ConstantFolder::BvSLt(left, right);
    }

    ExprPtr BvSLtEq(const ExprPtr& left, const ExprPtr& right) override
    {
        COMPARE_ARITHMETIC_SIMPLIFY(BvSLtEq)

        return ConstantFolder::BvSLtEq(left, right);
    }

    ExprPtr BvSGt(const ExprPtr& left, const ExprPtr& right) override
    {
        COMPARE_ARITHMETIC_SIMPLIFY(BvSGt)
    
        return ConstantFolder::BvSGt(left, right);
    }

    ExprPtr BvSGtEq(const ExprPtr& left, const ExprPtr& right) override
    {
        COMPARE_ARITHMETIC_SIMPLIFY(BvSGtEq)

        return ConstantFolder::BvSGtEq(left, right);
    }

    ExprPtr BvULt(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::BvULt(left, right);
    }

    ExprPtr BvULtEq(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::BvULtEq(left, right);
    }

    ExprPtr BvUGt(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::BvUGt(left, right);
    }

    ExprPtr BvUGtEq(const ExprPtr& left, const ExprPtr& right) override
    {
        return ConstantFolder::BvUGtEq(left, right);
    }

    #undef COMPARE_ARITHMETIC_SIMPLIFY

    ExprPtr FIsNan(const ExprPtr& op) override {
        return ConstantFolder::FIsNan(op);
    }

    ExprPtr FIsInf(const ExprPtr& op) override {
        return ConstantFolder::FIsInf(op);
    }
    
    ExprPtr FCast(const ExprPtr& op, FloatType& type, llvm::APFloat::roundingMode rm) override {
        return FCastExpr::Create(op, type, rm);
    }

    ExprPtr SignedToFp(const ExprPtr& op, FloatType& type, llvm::APFloat::roundingMode rm) override {
        return SignedToFpExpr::Create(op, type, rm);
    }

    ExprPtr UnsignedToFp(const ExprPtr& op, FloatType& type, llvm::APFloat::roundingMode rm) override {
        return UnsignedToFpExpr::Create(op, type, rm);
    }

    ExprPtr FpToSigned(const ExprPtr& op, BvType& type, llvm::APFloat::roundingMode rm) override {
        return FpToSignedExpr::Create(op, type, rm);
    }

    ExprPtr FpToUnsigned(const ExprPtr& op, BvType& type, llvm::APFloat::roundingMode rm) override {
        return FpToUnsignedExpr::Create(op, type, rm);
    }
    
    ExprPtr FAdd(const ExprPtr& left, const ExprPtr& right, llvm::APFloat::roundingMode rm) override {
        return ConstantFolder::FAdd(left, right, rm);
    }
    
    ExprPtr FSub(const ExprPtr& left, const ExprPtr& right, llvm::APFloat::roundingMode rm) override {
        return ConstantFolder::FSub(left, right, rm);
    }

    ExprPtr FMul(const ExprPtr& left, const ExprPtr& right, llvm::APFloat::roundingMode rm) override {
        return ConstantFolder::FMul(left, right, rm);
    }

    ExprPtr FDiv(const ExprPtr& left, const ExprPtr& right, llvm::APFloat::roundingMode rm) override {
        return ConstantFolder::FDiv(left, right, rm);
    }
    
    ExprPtr FEq(const ExprPtr& left, const ExprPtr& right) override {
        return ConstantFolder::FEq(left, right);
    }
    ExprPtr FGt(const ExprPtr& left, const ExprPtr& right) override {
        return ConstantFolder::FGt(left, right);
    }
    ExprPtr FGtEq(const ExprPtr& left, const ExprPtr& right) override {
        return ConstantFolder::FGtEq(left, right);
    }
    ExprPtr FLt(const ExprPtr& left, const ExprPtr& right) override {
        return ConstantFolder::FLt(left, right);
    }
    ExprPtr FLtEq(const ExprPtr& left, const ExprPtr& right) override {
        return ConstantFolder::FLtEq(left, right);
    }

    ExprPtr Select(const ExprPtr& condition, const ExprPtr& then, const ExprPtr& elze) override
    {
        // Select(True, E1, E2) --> E1
        // Select(False, E1, E2) --> E2
        if (auto condLit = llvm::dyn_cast<BoolLiteralExpr>(condition.get())) {
            return condLit->isTrue() ? then : elze;
        }

        ExprPtr c1 = nullptr, c2 = nullptr;
        ExprPtr e1 = nullptr, e2 = nullptr;

        // Select(C, E, E) --> E
        if (then == elze) {
            return then;
        }

        // Select(C, E, False) --> And(C, E)
        if (elze == this->False()) {
            return this->And({ condition, then });
        }

        // Select(C, E, True) --> Or(not C, E)
        if (elze == this->True()) {
            return this->Or({ this->Not(condition), then });
        }

        // Select(C, True, E) --> Or(C, E)
        if (then == this->True()) {
            return this->Or({ condition, elze });
        }

        // Select(C, False, E) --> And(not C, E)
        if (then == this->False()) {
            return this->And({ this->Not(condition), elze });
        }

        // Select(not C, E1, E2) --> Select(C, E2, E1)
        if (match(condition, then, elze, m_Not(m_Expr(c1)), m_Expr(e1), m_Expr(e2))) {
            return ConstantFolder::Select(condition, elze, then);
        }

        // Select(C1, Select(C1, E1, E'), E2) --> Select(C1, E1, E2)
        if (match(condition, then, elze, m_Expr(c1), m_Select(m_Specific(c1), m_Expr(e1), m_Expr()), m_Expr(e2))) {
            return ConstantFolder::Select(c1, e1, e2);
        }

        // Select(C1, E1, Select(C1, E', E2)) --> Select(C1, E1, E2)
        if (match(condition, then, elze, m_Expr(c1), m_Expr(e1), m_Select(m_Specific(c1), m_Expr(), m_Expr(e2)))) {
            return ConstantFolder::Select(c1, e1, e2);
        }

        // Select(C1, Select(C2, E1, E2), E1) --> Select(C1 and not C2, E2, E1)
        if (match(condition, then, elze, m_Expr(c1), m_Select(m_Expr(c2), m_Expr(e1), m_Expr(e2)), m_Specific(e1))) {
            return ConstantFolder::Select(this->And({c1, this->Not(c2)}), e1, e2);
        }
    
        // Select(C1, Select(C2, E1, E2), E2) --> Select(C1 and C2, E1, E2)
        if (match(condition, then, elze, m_Expr(c1), m_Select(m_Expr(c2), m_Expr(e1), m_Expr(e2)), m_Specific(e2))) {
            return ConstantFolder::Select(ConstantFolder::And({c1, c2}), e1, e2);
        }

        // Select(C1, E1, Select(C2, E1, E2)) --> Select(C1 or C2, E1, E2)
        if (match(condition, then, elze, m_Expr(c1), m_Expr(e1), m_Select(m_Expr(c2), m_Specific(e1), m_Expr(e2)))) {
            return ConstantFolder::Select(ConstantFolder::Or({c1, c2}), e1, e2);
        }

        return ConstantFolder::Select(condition, then, elze);
    }
};

} // end anonymous namespace

std::unique_ptr<ExprBuilder> gazer::CreateFoldingExprBuilder(GazerContext& context) {
    return std::unique_ptr<ExprBuilder>(new FoldingExprBuilder(context));
}