#pragma once
#include <stdint.h>
#include <stdatomic.h>
extern uint32_t test_log_primask;
#define __get_PRIMASK() (test_log_primask)
#define __disable_irq() (test_log_primask=1u)
#define __set_PRIMASK(value) (test_log_primask=(value))
#define __DMB() atomic_thread_fence(memory_order_seq_cst)
