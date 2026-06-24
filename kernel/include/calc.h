/*
 * AcharyaOS - calc.h
 * ------------------
 * Phase 1, Feature 17: Calculator.
 *
 * The calculator is a small integer expression evaluator. It is designed
 * for shell use, not as a general-purpose scripting engine.
 */

#ifndef ACHARYAOS_CALC_H
#define ACHARYAOS_CALC_H

#include <stdint.h>

int calc_evaluate(const char *expression, int64_t *out_value);
void calc_get_help(void);

#endif /* ACHARYAOS_CALC_H */
