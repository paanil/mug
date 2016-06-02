#include <stdio.h>

typedef long long int i64;
typedef unsigned long long int u64;

i64 f();
i64 f2(i64 a, i64 b);
i64 f3(i64 a, i64 b);

i64 eq(i64 a, i64 b);
i64 ne(i64 a, i64 b);
i64 below(u64 a, u64 b);
i64 gt(i64 a, i64 b);
i64 le(i64 a, i64 b);
i64 ge(i64 a, i64 b);

#define TEST(x, y) { i64 z = x; printf(#x " -> %lld \t\t%s\n", z, (z == y) ? "OK" : "ERROR"); }

int main()
{
    TEST(f(), 42)

    TEST(f2(25, 40), 65)
    TEST(f2(25, -40), -15)
    TEST(f2(-25, 40), 15)
    TEST(f2(-25, -40), -65)

    TEST(f3(10, 20), 5)
    TEST(f3(10, -20), 10)
    TEST(f3(-10, 20), 5)
    TEST(f3(-10, -20), 10)
    TEST(f3(20, 10), 10)
    TEST(f3(20, -10), 10)
    TEST(f3(-20, 10), 5)
    TEST(f3(-20, -10), 5)

    TEST(eq(100, 100), 1)
    TEST(eq(100, -100), 0)
    TEST(ne(100, -100), 1)
    TEST(ne(100, 100), 0)
    TEST(below(5, 100), 1)
    TEST(below(-5, 100), 0)
    TEST(gt(-5, 100), 0)
    TEST(gt(-5, -100), 1)
    TEST(le(-100, 100), 1)
    TEST(le(100, 100), 1)
    TEST(le(100, -100), 0)
    TEST(ge(-100, 100), 0)
    TEST(ge(100, 100), 1)
    TEST(ge(100, -100), 1)
    return 0;
}
