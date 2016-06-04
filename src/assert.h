#ifndef ASSERT_H
#define ASSERT_H

#include <cassert>

#define InvalidCodePath assert(0 && "invalid code path!")
#define InvalidDefaultCase default: assert(0 && "invalid default case!"); break

#endif // ASSERT_H
