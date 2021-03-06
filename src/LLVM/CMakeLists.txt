set(SOURCE_FILES
    Transform/InlineGlobalVariablesPass.cpp
    Transform/UndefToNondet.cpp
    Transform/LiftErrorsPass.cpp
    Transform/NormalizeVerifierCalls.cpp
    Transform/BackwardSlicer.cpp
    Transform/Inline.cpp
    Transform/TransformUtils.cpp
    Instrumentation/MarkFunctionEntries.cpp
    Instrumentation/Check.cpp
    Instrumentation/DefaultChecks.cpp
    Instrumentation/Intrinsics.cpp
    Trace/TestHarnessGenerator.cpp
    Automaton/ModuleToAutomata.cpp
    Automaton/InstToExpr.cpp
    LLVMTraceBuilder.cpp
    TypeTranslator.cpp
    LLVMFrontendSettings.cpp
    LLVMFrontend.cpp
    ClangFrontend.cpp
    Memory/MemoryObject.cpp
    Memory/FlatMemoryModel.cpp
    Memory/DummyMemoryModel.cpp
    Memory/MemoryModel.cpp
    Memory/MemorySSA.cpp
    Memory/MemoryUtils.cpp
    Automaton/AutomatonPasses.cpp
    Automaton/ExtensionPoints.cpp
    Automaton/ValueOrMemoryObject.cpp
    Analysis/PDG.cpp
)

llvm_map_components_to_libnames(GAZER_LLVM_LIBS core irreader transformutils scalaropts ipo)
message(STATUS "Using LLVM libraries: ${GAZER_LLVM_LIBS}")

add_library(GazerLLVM SHARED ${SOURCE_FILES})
target_link_libraries(GazerLLVM ${GAZER_LLVM_LIBS} GazerCore GazerTrace GazerZ3Solver GazerAutomaton GazerVerifier)
