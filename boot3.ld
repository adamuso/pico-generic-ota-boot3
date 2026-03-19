    
    .boot3 : {
        __boot3_start__ = .;
        KEEP(*(.boot3.entry))
        KEEP(*(.boot3))
    } > FLASH

    PROVIDE(__boot3_critical = .);

    .boot3_critical : {
        PROVIDE(__boot3_ram_start = .);
        KEEP(*(.boot3.data))
        KEEP(*(.boot3.critical))
    } > RAM AT> FLASH

    .boot3_bss . (NOLOAD) : {
        . = ALIGN(4);

        PROVIDE(__boot3_bss = .);
        KEEP(*(.boot3.bss))
        . = ALIGN(4);
        PROVIDE(__boot3_bss_end = .);
    } > RAM
    
    __boot3_end__ = LOADADDR(.boot3_critical) + SIZEOF(.boot3_critical);
    . = __boot3_end__;
    . = ALIGN(4);

    PROVIDE(__boot3_critical_end = .);

    ASSERT(__boot3_end__ - __boot3_start__ <= 4096 - 256, "boot2 + boot3 section exceeds 4KiB")
    . = __boot3_start__ + 4096 - 256;

    .boot3_state_checksum . : {
        PROVIDE(__boot3_state = .);
        KEEP(*(.boot3_state.checksum))
    }

    .boot3_state_program_size . : {
        KEEP(*(.boot3_state.program_size))
    }

    .boot3_state . : {
        __boot3_internal_state_start__ = .;
        KEEP(*(.boot3_state.magic))
        KEEP(*(.boot3_state.data))
        __boot3_internal_state_end__ = .;
        
        ASSERT(__boot3_internal_state_end__ - __boot3_internal_state_start__ <= 1024, "boot3 internal state exceeds 1KiB");
        . = ALIGN(1024);

        __boot3_user_data_start__ = .;
        KEEP(*(.boot3_state.user_data))
        __boot3_user_data_end__ = .;

        ASSERT(__boot3_user_data_end__ - __boot3_user_data_start__ <= 1024, "boot3 user data exceeds 1KiB");
        . = __boot3_user_data_start__ + 1024;

        __boot3_progress_start__ = .;
        KEEP(*(.boot3_state.progress))
        __boot3_progress_end__ = .;

        ASSERT(__boot3_progress_end__ - __boot3_progress_start__ <= 2048, "boot3 copy flash progress exceeds 2KiB");
    } > FLASH

    .boot3_slot_state : {
        KEEP(*(.boot3_slot_state))
    } > FLASH_SLOT

    . = __boot3_start__ + 8192 - 256;
    PROVIDE(__boot3_end = .);

    .text . : {
