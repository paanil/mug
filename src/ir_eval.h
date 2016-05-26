#ifndef IR_EVAL_H
#define IR_EVAL_H

#include <cstdint>

#define VOID_VALUE 0xdeadf00dbaadf00d // a hack to detect void in tests

uint64_t eval(struct IR &ir);

#endif // IR_EVAL_H
