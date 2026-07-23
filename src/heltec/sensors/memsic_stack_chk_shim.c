/**
 * libMemsicAlgo.a (cortex-m4) is built with -fstack-protector and references
 * __stack_chk_guard / __stack_chk_fail. When the firmware is not compiled with
 * stack protection, the linker has no definitions — add this shim.
 */
#if ENV_INCLUDE_COMPASS
#include <stdint.h>

/* Non-zero canary; value is only meaningful if the app also uses stack cookies. */
uintptr_t __stack_chk_guard = 0xdeadbeefu;

void __attribute__((noreturn)) __stack_chk_fail(void)
{
  while (1) { }
}

#endif // ENV_INCLUDE_COMPASS
