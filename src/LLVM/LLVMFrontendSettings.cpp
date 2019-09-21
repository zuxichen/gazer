#include "gazer/LLVM/LLVMFrontendSettings.h"

#include <llvm/Support/CommandLine.h>

using namespace gazer;
using namespace llvm;

namespace
{
    cl::opt<ElimVarsLevel> ElimVarsLevelOpt("elim-vars", cl::desc("Level for variable elimination:"),
        cl::values(
            clEnumValN(ElimVarsLevel::Off, "off", "Do not eliminate variables"),
            clEnumValN(ElimVarsLevel::Normal, "normal", "Eliminate variables having only one use"),
            clEnumValN(ElimVarsLevel::Aggressive, "aggressive", "Eliminate all eligible variables")
        ),
        cl::init(ElimVarsLevel::Normal)
    );

    cl::opt<bool> ArithInts("math-int", cl::desc("Use mathematical unbounded integers instead of bitvectors."));
} // end anonymous namespace

namespace gazer
{
    // NoSimplifyExpr is a global setting, defined in BoundedModelChecker.cpp
    extern cl::opt<bool> NoSimplifyExpr;
} // end namespace gazer

LLVMFrontendSettings LLVMFrontendSettings::initFromCommandLine()
{
    LLVMFrontendSettings settings;
    settings.setElimVarsLevel(ElimVarsLevelOpt);
    settings.setSimplifyExpr(!NoSimplifyExpr);

    if (ArithInts) {
        settings.setIntRepresentation(IntRepresentation::Integers);
    } else {
        settings.setIntRepresentation(IntRepresentation::BitVectors);
    }

    return settings;
}

std::string LLVMFrontendSettings::toString() const
{
    std::string str;

    str += R"({"elim_vars": ")";
    switch (mElimVars) {
        case ElimVarsLevel::Off:         str += "off"; break;
        case ElimVarsLevel::Normal:      str += "normal"; break;
        case ElimVarsLevel::Aggressive:  str += "aggressive"; break;
    }
    str += R"(", "loop_representation": ")";

    switch (mLoops) {
        case LoopRepresentation::Recursion:  str += "recursion"; break;
        case LoopRepresentation::Cycle:      str += "cycle"; break;
    }

    str += R"(", "int_representation": ")";

    switch (mInts) {
        case IntRepresentation::BitVectors:     str += "bv"; break;
        case IntRepresentation::Integers:       str += "int"; break;
    }

    str += R"(", "float_representation": ")";

    switch (mFloats) {
        case FloatRepresentation::Fpa:     str += "fpa";   break;
        case FloatRepresentation::Real:    str += "real";  break;
        case FloatRepresentation::Undef:   str += "undef"; break;
    }

    str += "\"}";

    return str;
}