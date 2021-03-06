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
#include "gazer/Core/Valuation.h"
#include "gazer/Core/LiteralExpr.h"

#include <llvm/Support/raw_ostream.h>

using namespace gazer;

void Valuation::print(llvm::raw_ostream& os)
{
    for (auto it = mMap.begin(); it != mMap.end(); ++it) {
        auto [variable, expr] = *it;

        os << variable->getName() << " = ";
        expr->print(os);
        os << "\n";
    }
}

ExprRef<LiteralExpr>& Valuation::operator[](const Variable& variable)
{
    return mMap[&variable];
}

ExprRef<AtomicExpr> Valuation::eval(const ExprPtr& expr)
{
    if (auto varRef = llvm::dyn_cast<VarRefExpr>(expr.get())) {
        auto lit = this->operator[](varRef->getVariable());
        return lit != nullptr ? boost::static_pointer_cast<AtomicExpr>(lit) : UndefExpr::Get(expr->getType());
    }
    
    if (expr->getKind() == Expr::Literal) {
        return llvm::cast<LiteralExpr>(expr);
    }

    return UndefExpr::Get(expr->getType());
}
