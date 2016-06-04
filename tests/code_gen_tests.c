#include <stdio.h>
#include <stdbool.h>

typedef long long int i64;
typedef unsigned long long int u64;

i64 f();
i64 f2(i64 a, i64 b);
i64 f3(i64 a, i64 b);

bool eq(i64 a, i64 b);
bool ne(i64 a, i64 b);
bool below(u64 a, u64 b);
bool gt(i64 a, i64 b);
bool le(i64 a, i64 b);
bool ge(i64 a, i64 b);

bool g(bool x);

u64 multiply(u64 a, u64 b);
i64 imultiply(i64 a, i64 b);

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

    TEST(eq(100, 100), true)
    TEST(eq(100, -100), false)
    TEST(ne(100, -100), true)
    TEST(ne(100, 100), false)
    TEST(below(5, 100), true)
    TEST(below(-5, 100), false)
    TEST(gt(-5, 100), false)
    TEST(gt(-5, -100), true)
    TEST(le(-100, 100), true)
    TEST(le(100, 100), true)
    TEST(le(100, -100), false)
    TEST(ge(-100, 100), false)
    TEST(ge(100, 100), true)
    TEST(ge(100, -100), true)

    TEST(g(true), false)
    TEST(g(false), true)
    TEST(g(12345), false)

    TEST(multiply(-5ull, 5ull), -5ull * 5ull)
    TEST(imultiply(-5, 5), -5 * 5)

    return 0;
}
