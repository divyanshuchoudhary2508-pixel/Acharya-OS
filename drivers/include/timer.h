/*
 * AcharyaOS - timer.h
 * -------------------
 * Phase 1, Feature 8: PIT timer interrupts. The timer gives the kernel a
 * regular heartbeat, which the scheduler will use in the next subsystem.
 */

#ifndef ACHARYAOS_TIMER_H
#define ACHARYAOS_TIMER_H

#include <stdint.h>

void timer_init(uint32_t frequency_hz);
uint64_t timer_get_ticks(void);
uint32_t timer_get_frequency(void);

#endif /* ACHARYAOS_TIMER_H */
