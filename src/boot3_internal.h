#pragma once 

#include <stdint.h>
#include <stddef.h>

#define in_boot3_section __attribute__((section(".boot3")))
#define in_boot3_data __attribute__((section(".boot3.data")))
#define in_boot3_bss __attribute__((section(".boot3.bss")))
#define in_boot3_critical_section __attribute__((section(".boot3.critical")))

extern char __boot3_critical;
extern char __boot3_critical_end;
extern char __boot3_ram_start;
extern char __boot3_bss;
extern char __boot3_bss_end;
