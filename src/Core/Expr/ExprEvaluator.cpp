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
#include "gazer/Core/Expr/ExprEvaluator.h"

using namespace gazer;
using llvm::cast;
using llvm::dyn_cast;

/// Checks for undefs among operands.

ExprRef<LiteralExpr> ExprEvaluatorBase::visitUndef(const ExprRef<UndefExpr>& expr)
{
    llvm_unreachable("Invalid undef expression");
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitExpr(const ExprPtr& expr)
{
    llvm_unreachable("Unhandled expression type in ExprEvaluatorBase");
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitLiteral(const ExprRef<LiteralExpr>& expr) {
    return expr;
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitVarRef(const ExprRef<VarRefExpr>& expr) {
    return this->getVariableValue(expr->getVariable());
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitZExt(const ExprRef<ZExtExpr>& expr)
{
    auto bvLit = dyn_cast<BvLiteralExpr>(getOperand(0));
    auto& type = llvm::cast<BvType>(expr->getType());

    return BvLiteralExpr::Get(type, bvLit->getValue().zext(expr->getExtendedWidth()));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitSExt(const ExprRef<SExtExpr>& expr)
{
    auto bvLit = dyn_cast<BvLiteralExpr>(getOperand(0));
    auto& type = llvm::cast<BvType>(expr->getType());

    return BvLiteralExpr::Get(type, bvLit->getValue().sext(expr->getExtendedWidth()));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitExtract(const ExprRef<ExtractExpr>& expr)
{
    auto bvLit = dyn_cast<BvLiteralExpr>(getOperand(0));

    return BvLiteralExpr::Get(
        cast<BvType>(expr->getType()),
        bvLit->getValue().extractBits(expr->getExtractedWidth(), expr->getOffset())
    );
}

static ExprRef<LiteralExpr> EvalBinaryArithmetic(
    Expr::ExprKind kind,
    const ExprRef<LiteralExpr>& lhs,
    const ExprRef<LiteralExpr>& rhs)
{
    assert(lhs->getType() == rhs->getType());

    if (lhs->getType().isBvType()) {
        auto left  = llvm::cast<BvLiteralExpr>(lhs)->getValue();
        auto right = llvm::cast<BvLiteralExpr>(rhs)->getValue();

        auto& type = llvm::cast<BvType>(lhs->getType());

        switch (kind) {
            case Expr::Add: return BvLiteralExpr::Get(type, left + right);
            case Expr::Sub: return BvLiteralExpr::Get(type, left - right);
            case Expr::Mul: return BvLiteralExpr::Get(type, left * right);
            case Expr::BvSDiv: return BvLiteralExpr::Get(type, left.sdiv(right));
            case Expr::BvUDiv: return BvLiteralExpr::Get(type, left.udiv(right));
            case Expr::BvSRem: return BvLiteralExpr::Get(type, left.srem(right));
            case Expr::BvURem: return BvLiteralExpr::Get(type, left.urem(right));
            case Expr::Shl: return BvLiteralExpr::Get(type, left.shl(right));
            case Expr::LShr: return BvLiteralExpr::Get(type, left.lshr(right.getLimitedValue()));
            case Expr::AShr: return BvLiteralExpr::Get(type, left.ashr(right.getLimitedValue()));
            case Expr::BvAnd: return BvLiteralExpr::Get(type, left & right);
            case Expr::BvOr: return BvLiteralExpr::Get(type, left | right);
            case Expr::BvXor: return BvLiteralExpr::Get(type, left ^ right);
            default:
                llvm_unreachable("Unknown binary expression kind!");
        }
    }
    
    if (lhs->getType().isIntType()) {
        auto left  = llvm::cast<IntLiteralExpr>(lhs)->getValue();
        auto right = llvm::cast<IntLiteralExpr>(rhs)->getValue();

        auto& intTy = llvm::cast<IntType>(lhs->getType());

        switch (kind) {
            case Expr::Add: return IntLiteralExpr::Get(intTy, left + right);
            case Expr::Sub: return IntLiteralExpr::Get(intTy, left - right);
            case Expr::Mul: return IntLiteralExpr::Get(intTy, left * right);
            case Expr::Div: return IntLiteralExpr::Get(intTy, left / right);
            default:
                break;
            /*
            case Expr::Mod: {
                return IntLiteralExpr::Get(intTy, left * right);
            }
            case Expr::Rem: {
                // Division remainder, for example:
                //  5 rem  3 = 2
                //  5 rem -3 = 2
                // -5 rem  3 = -2
                // -5 rem -3 = -2
                return IntLiteralExpr::Get(intTy, left % right);
            } */
        }
    }
    
    llvm_unreachable("Unknown binary arithmetic expression kind.");
}

// Binary
ExprRef<LiteralExpr> ExprEvaluatorBase::visitAdd(const ExprRef<AddExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitSub(const ExprRef<SubExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitMul(const ExprRef<MulExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitDiv(const ExprRef<DivExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitMod(const ExprRef<ModExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitRem(const ExprRef<RemExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvSDiv(const ExprRef<BvSDivExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvUDiv(const ExprRef<BvUDivExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvSRem(const ExprRef<BvSRemExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvURem(const ExprRef<BvURemExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitShl(const ExprRef<ShlExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitLShr(const ExprRef<LShrExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitAShr(const ExprRef<AShrExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvAnd(const ExprRef<BvAndExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvOr(const ExprRef<BvOrExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvXor(const ExprRef<BvXorExpr>& expr)
{
    return EvalBinaryArithmetic(expr->getKind(), getOperand(0), getOperand(1));
}

// Logic
//-----------------------------------------------------------------------------

ExprRef<LiteralExpr> ExprEvaluatorBase::visitNot(const ExprRef<NotExpr>& expr)
{
    auto boolLit = dyn_cast<BoolLiteralExpr>(getOperand(0).get());
    return BoolLiteralExpr::Get(cast<BoolType>(expr->getType()), !boolLit->getValue());
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitAnd(const ExprRef<AndExpr>& expr)
{
    bool result = true;
    for (size_t i = 0; i < expr->getNumOperands(); ++i) {
        result = result && cast<BoolLiteralExpr>(getOperand(i))->getValue();
    }

    return BoolLiteralExpr::Get(cast<BoolType>(expr->getType()), result);
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitOr(const ExprRef<OrExpr>& expr)
{
    bool result = false;
    for (size_t i = 0; i < expr->getNumOperands(); ++i) {
        result = result || cast<BoolLiteralExpr>(getOperand(i))->getValue();
    }

    return BoolLiteralExpr::Get(cast<BoolType>(expr->getType()), result);
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitXor(const ExprRef<XorExpr>& expr) {
    auto left = cast<BoolLiteralExpr>(getOperand(0))->getValue();
    auto right = cast<BoolLiteralExpr>(getOperand(1))->getValue();

    return BoolLiteralExpr::Get(cast<BoolType>(expr->getType()), left != right);
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitImply(const ExprRef<ImplyExpr>& expr) {
    auto left = cast<BoolLiteralExpr>(getOperand(0))->getValue();
    auto right = cast<BoolLiteralExpr>(getOperand(1))->getValue();

    return BoolLiteralExpr::Get(cast<BoolType>(expr->getType()), !left || right);
}

static ExprRef<LiteralExpr> EvalBvCompare(
    Expr::ExprKind kind,
    const ExprRef<LiteralExpr>& lhs,
    const ExprRef<LiteralExpr>& rhs)
{
    assert(lhs->getType().isBvType());

    auto left = cast<BvLiteralExpr>(lhs)->getValue();
    auto right = cast<BvLiteralExpr>(rhs)->getValue();

    BoolType& type = BoolType::Get(lhs->getContext());

    switch (kind) {
        case Expr::Eq: return BoolLiteralExpr::Get(type, left.eq(right));
        case Expr::NotEq: return BoolLiteralExpr::Get(type, left.ne(right));
        case Expr::BvSLt: return BoolLiteralExpr::Get(type, left.slt(right));
        case Expr::BvSLtEq: return BoolLiteralExpr::Get(type, left.sle(right));
        case Expr::BvSGt: return BoolLiteralExpr::Get(type, left.sgt(right));
        case Expr::BvSGtEq: return BoolLiteralExpr::Get(type, left.sge(right));
        case Expr::BvULt: return BoolLiteralExpr::Get(type, left.ult(right));
        case Expr::BvULtEq: return BoolLiteralExpr::Get(type, left.ule(right));
        case Expr::BvUGt: return BoolLiteralExpr::Get(type, left.ugt(right));
        case Expr::BvUGtEq: return BoolLiteralExpr::Get(type, left.uge(right));
        default:
            break;
    }

    llvm_unreachable("Unknown bit-vector comparison kind.");
}

static ExprRef<LiteralExpr> EvalIntCompare(
    Expr::ExprKind kind,
    const ExprRef<LiteralExpr>& lhs,
    const ExprRef<LiteralExpr>& rhs)
{
    assert(lhs->getType().isIntType());
    assert(rhs->getType().isIntType());

    auto left = llvm::cast<IntLiteralExpr>(lhs)->getValue();
    auto right = llvm::cast<IntLiteralExpr>(rhs)->getValue();

    BoolType& type = BoolType::Get(lhs->getContext());

    switch (kind) {
        case Expr::Eq: return BoolLiteralExpr::Get(type, left == right);
        case Expr::NotEq: return BoolLiteralExpr::Get(type, left != right);
        case Expr::Lt: return BoolLiteralExpr::Get(type, left < right);
        case Expr::LtEq: return BoolLiteralExpr::Get(type, left <= right);
        case Expr::Gt: return BoolLiteralExpr::Get(type, left > right);
        case Expr::GtEq: return BoolLiteralExpr::Get(type, left >= right);
        default:
            break;
    }

    llvm_unreachable("Unknown integer comparison kind.");
}

static ExprRef<LiteralExpr> EvalBinaryCompare(
    Expr::ExprKind kind, const ExprRef<LiteralExpr>& left, const ExprRef<LiteralExpr>& right)
{
    Type& opTy = left->getType();
    assert(left->getType() == right->getType());
    assert(!opTy.isFloatType() && "Float types must be compared using FEqExpr!");

    switch (opTy.getTypeID()) {
        case Type::BvTypeID:
            return EvalBvCompare(kind, left, right);
        case Type::BoolTypeID: {
            BoolType& boolTy = BoolType::Get(opTy.getContext());
            assert(kind == Expr::Eq || kind == Expr::NotEq);

            return BoolLiteralExpr::Get(
                boolTy,
                cast<BoolLiteralExpr>(left)->getValue() == cast<BoolLiteralExpr>(right)->getValue()
            );
        }
        case Type::IntTypeID:
            return EvalIntCompare(kind, left, right);
        default:
            break;
    }

    llvm_unreachable("Invalid operand type in a comparison expression!");
}

// Compare
ExprRef<LiteralExpr> ExprEvaluatorBase::visitEq(const ExprRef<EqExpr>& expr)
{
    return EvalBinaryCompare(Expr::Eq, getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitNotEq(const ExprRef<NotEqExpr>& expr)
{
    return EvalBinaryCompare(Expr::NotEq, getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitLt(const ExprRef<LtExpr>& expr) {
    return EvalIntCompare(Expr::Lt, getOperand(0), getOperand(1));
}
ExprRef<LiteralExpr> ExprEvaluatorBase::visitLtEq(const ExprRef<LtEqExpr>& expr) {
    return EvalIntCompare(Expr::LtEq, getOperand(0), getOperand(1));
}
ExprRef<LiteralExpr> ExprEvaluatorBase::visitGt(const ExprRef<GtExpr>& expr) {
    return EvalIntCompare(Expr::Gt, getOperand(0), getOperand(1));
}
ExprRef<LiteralExpr> ExprEvaluatorBase::visitGtEq(const ExprRef<GtEqExpr>& expr) {
    return EvalIntCompare(Expr::GtEq, getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvSLt(const ExprRef<BvSLtExpr>& expr)
{
    return EvalBvCompare(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvSLtEq(const ExprRef<BvSLtEqExpr>& expr)
{
    return EvalBvCompare(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvSGt(const ExprRef<BvSGtExpr>& expr)
{
    return EvalBvCompare(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvSGtEq(const ExprRef<BvSGtEqExpr>& expr)
{
    return EvalBvCompare(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvULt(const ExprRef<BvULtExpr>& expr)
{
    return EvalBvCompare(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvULtEq(const ExprRef<BvULtEqExpr>& expr)
{
    return EvalBvCompare(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvUGt(const ExprRef<BvUGtExpr>& expr)
{
    return EvalBvCompare(expr->getKind(), getOperand(0), getOperand(1));
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitBvUGtEq(const ExprRef<BvUGtEqExpr>& expr)
{
    return EvalBvCompare(expr->getKind(), getOperand(0), getOperand(1));
}

// Floating-point queries
ExprRef<LiteralExpr> ExprEvaluatorBase::visitFIsNan(const ExprRef<FIsNanExpr>& expr) {
    return this->visitNonNullary(expr);
}
ExprRef<LiteralExpr> ExprEvaluatorBase::visitFIsInf(const ExprRef<FIsInfExpr>& expr) {
    return this->visitNonNullary(expr);
}

// Floating-point arithmetic
ExprRef<LiteralExpr> ExprEvaluatorBase::visitFAdd(const ExprRef<FAddExpr>& expr) {
    return this->visitNonNullary(expr);
}
ExprRef<LiteralExpr> ExprEvaluatorBase::visitFSub(const ExprRef<FSubExpr>& expr) {
    return this->visitNonNullary(expr);
}
ExprRef<LiteralExpr> ExprEvaluatorBase::visitFMul(const ExprRef<FMulExpr>& expr) {
    return this->visitNonNullary(expr);
}
ExprRef<LiteralExpr> ExprEvaluatorBase::visitFDiv(const ExprRef<FDivExpr>& expr) {
    return this->visitNonNullary(expr);
}

// Floating-point compare
ExprRef<LiteralExpr> ExprEvaluatorBase::visitFEq(const ExprRef<FEqExpr>& expr) {
    return this->visitNonNullary(expr);
}
ExprRef<LiteralExpr> ExprEvaluatorBase::visitFGt(const ExprRef<FGtExpr>& expr) {
    return this->visitNonNullary(expr);
}
ExprRef<LiteralExpr> ExprEvaluatorBase::visitFGtEq(const ExprRef<FGtEqExpr>& expr) {
    return this->visitNonNullary(expr);
}
ExprRef<LiteralExpr> ExprEvaluatorBase::visitFLt(const ExprRef<FLtExpr>& expr) {
    return this->visitNonNullary(expr);
}
ExprRef<LiteralExpr> ExprEvaluatorBase::visitFLtEq(const ExprRef<FLtEqExpr>& expr) {
    return this->visitNonNullary(expr);
}

template<class Type, class ExprTy>
ExprRef<LiteralExpr> EvalSelect(
    const ExprRef<BoolLiteralExpr>& cond,
    const ExprRef<LiteralExpr>& then,
    const ExprRef<LiteralExpr>& elze
) {
    return ExprTy::Get(
        *cast<Type>(&then->getType()),
        cond->getValue() ? cast<ExprTy>(then)->getValue() : cast<ExprTy>(elze)->getValue()
    );
}

// Ternary
ExprRef<LiteralExpr> ExprEvaluatorBase::visitSelect(const ExprRef<SelectExpr>& expr)
{
    auto cond = cast<BoolLiteralExpr>(getOperand(0));
    auto then = getOperand(1);
    auto elze = getOperand(2);

    switch (expr->getType().getTypeID()) {
        case Type::BoolTypeID:
            return EvalSelect<BoolType, BoolLiteralExpr>(cond, then, elze);
        case Type::BvTypeID:
            return EvalSelect<BvType, BvLiteralExpr>(cond, then, elze);
        case Type::IntTypeID:
            return EvalSelect<IntType, IntLiteralExpr>(cond, then, elze);
        case Type::FloatTypeID:
            return EvalSelect<FloatType, FloatLiteralExpr>(cond, then, elze);
        case Type::RealTypeID:
            return EvalSelect<RealType, RealLiteralExpr>(cond, then, elze);
    }
    
    llvm_unreachable("Invalid SelectExpr type!");
}

// Arrays
ExprRef<LiteralExpr> ExprEvaluatorBase::visitArrayRead(const ExprRef<ArrayReadExpr>& expr) {
    return this->visitNonNullary(expr);
}

ExprRef<LiteralExpr> ExprEvaluatorBase::visitArrayWrite(const ExprRef<ArrayWriteExpr>& expr) {
    return this->visitNonNullary(expr);
}