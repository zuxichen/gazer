// XFAIL: memory.arrays
// RUN: %theta "%s" | FileCheck "%s"

// CHECK: Verification SUCCESSFUL
int __VERIFIER_nondet_int(void);
void __VERIFIER_error(void) __attribute__((__noreturn__));

int main(void)
{
    int x[5];

    for (int i = 0; i < 5; ++i) {
        x[i] = i + 1;
    }

    if (x[0] == 0) {
        __VERIFIER_error();
    }

    return 0;
}