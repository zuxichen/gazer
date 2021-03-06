// RUN: %bmc -bound 1 "%s" | FileCheck "%s"

// CHECK: Verification FAILED
extern void __VERIFIER_error(void);
extern float __VERIFIER_nondet_float(void);
extern double __VERIFIER_nondet_double(void);

int main(void)
{
    float x = __VERIFIER_nondet_float();
    float y = __VERIFIER_nondet_float();

    float mul1 = x * x * y;
    float mul2 = x * (x * y);

    if (mul1 != mul2) {
        __VERIFIER_error();
    }

    return 0;
}
