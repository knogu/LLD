#pragma once

#include "common.h"

void timer_init();
void handle_timer_1();
void handle_timer_3();
void timer_sleep(u32 ms);
u64 timer_get_ticks();

unsigned long HAL_GetTick(void);
void HAL_Delay(unsigned int ms);
