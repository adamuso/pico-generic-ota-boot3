.syntax unified
.cpu cortex-m0plus
.thumb

#include "hardware/regs/m0plus.h"

// Place the entry in a dedicated .boot3.entry subsection so the linker
// injection can force it to the very start of the .boot3 output section.
.section .boot3.entry
.global __boot3_vectors
.global boot3_entry

__boot3_vectors:
    .word __StackTop
    .word boot3_entry

.type boot3_entry,%function
.thumb_func
boot3_entry:
    // Branch to the C entry point; preserve LR in case C wants to return
    push {lr}
    bl boot3_main
    pop {r0}
    // If boot3_main returns, loop here forever
1:  b 1b
