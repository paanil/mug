#ifndef CODE_GEN_H
#define CODE_GEN_H

#include <cstdio>

/**
 * Generates assembly from the given intermediate code and
 * writes the assembly code into the given file.
 */
void gen_code(struct IR ir, FILE *f);

#endif // CODE_GEN_H
