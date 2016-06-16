#ifndef IR_EVAL_H
#define IR_EVAL_H

#include <cstdint>

#define VOID_VALUE 0xdeadf00dbaadf00d // a hack to detect void in tests

/**
 * Evaluates given ir and returns the last value for testing purposes.
 */
uint64_t eval(struct IR &ir);

#endif // IR_EVAL_H
