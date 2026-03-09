#pragma once
#include "pico/platform/sections.h"

#undef __not_in_flash
#undef __not_in_flash_func
#undef __no_inline_not_in_flash_func

#define __not_in_flash(group) __attribute__((section(".boot3.critical")))
#define __not_in_flash_func(func_name) __not_in_flash(__STRING(func_name)) func_name
#define __no_inline_not_in_flash_func(func_name) __noinline __not_in_flash_func(func_name)
