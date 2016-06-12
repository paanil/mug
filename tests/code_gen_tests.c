#include <stdio.h>
#include <stdbool.h>

typedef long long int i64;
typedef unsigned long long int u64;

i64 f();
i64 f2(i64 a, i64 b);
i64 f3(i64 a, i64 b);
i64 f4(i64 a, i64 b);

bool eq(i64 a, i64 b);
bool ne(i64 a, i64 b);
bool below(u64 a, u64 b);
bool gt(i64 a, i64 b);
bool le(i64 a, i64 b);
bool ge(i64 a, i64 b);

bool g(bool x);

u64 multiply(u64 a, u64 b);
i64 imultiply(i64 a, i64 b);

u64 divide(u64 a, u64 b);
i64 idivide(i64 a, i64 b);

i64 negate(i64 x);

bool and_(bool a, bool b);
bool or_(bool a, bool b);

i64 h(bool x);

i64 spill_test();

i64 many_params_test(i64 x, u64 y, bool z, i64 w, i64 a, u64 b, bool c);

i64 call_test();

#define TEST(x, y) { i64 z = x; printf(#x " -> %lld \t\t%s\n", z, (z == y) ? "OK" : "ERROR"); }

int main()
{
    TEST(f(), 42)

    // a + b
    TEST(f2(25, 40), 65)
    TEST(f2(25, -40), -15)
    TEST(f2(-25, 40), 15)
    TEST(f2(-25, -40), -65)

    // (a < b) ? 5 : 10
    TEST(f3(10, 20), 5)
    TEST(f3(10, -20), 10)
    TEST(f3(-10, 20), 5)
    TEST(f3(-10, -20), 10)
    TEST(f3(20, 10), 10)
    TEST(f3(20, -10), 10)
    TEST(f3(-20, 10), 5)
    TEST(f3(-20, -10), 5)

    // a - b
    TEST(f4(25, 40), -15)
    TEST(f4(25, -40), 65)
    TEST(f4(-25, 40), -65)
    TEST(f4(-25, -40), 15)

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

    TEST(divide(10ull, 5ull), 2ull)
    TEST(idivide(-10, 5), -2)

    TEST(negate(-10), 10)
    TEST(negate(5), -5)

    TEST(and_(false, false), false)
    TEST(and_(true, false), false)
    TEST(and_(false, true), false)
    TEST(and_(true, true), true)

    TEST(or_(false, false), false)
    TEST(or_(true, false), true)
    TEST(or_(false, true), true)
    TEST(or_(true, true), true)

    TEST(h(true), 5);
    TEST(h(false), 10);

    TEST(spill_test(), 54)

    TEST(many_params_test(10, 20, true, 30, 40, 50, true), 10);
    TEST(many_params_test(10, 20, false, 30, 40, 50, true), 30);
    TEST(many_params_test(10, 200, false, 30, 40, 50, true), 40);
    TEST(many_params_test(10, 200, false, 30, 40, 5, true), -22);
    TEST(many_params_test(10, 200, true, 30, 40, 5, false), -22);

    TEST(call_test(), 42);

    return 0;
}
