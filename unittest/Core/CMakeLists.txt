SET(TEST_SOURCES
    TypeTest.cpp
    VariableTest.cpp
    ExprTest.cpp
    Expr/MatcherTest.cpp
    Expr/ExprPrinterTest.cpp
    Expr/ExprEvaluatorTest.cpp
    Expr/ExprWalkerTest.cpp
)

add_test(GazerCoreTest GazerCoreTest)
add_executable(GazerCoreTest ${TEST_SOURCES})
target_link_libraries(GazerCoreTest GazerCore gtest_main)