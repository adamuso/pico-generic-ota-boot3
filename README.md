# pico-generic-ota-boot3

A generic, protocol-agnostic **Stage 3 (boot3) Bootloader** for the Raspberry Pi Silicon family (currently only **RP2040**, but **RP2350** may be added in the future).

This project provides a robust framework for implementing Over-the-Air (OTA) firmware updates regardless of the communication medium. Whether your device is connected via Wi-Fi (CYW43439), UART, USB, CAN, SPI, or any other transport, this bootloader handles the critical application switching and update logic.

## Quick Start

### Prerequisites

- [pico-sdk](https://github.com/raspberrypi/pico-sdk) (included as a submodule in `lib/pico-sdk` or provided via `PICO_SDK_PATH`)
- CMake ≥ 3.22
- ARM GCC toolchain (`arm-none-eabi-gcc`)

### Adding boot3 to your project

**1. Add as a subdirectory** (recommended — either as a submodule or a copy):

```cmake
# In your project's CMakeLists.txt, before pico_sdk_init()
add_subdirectory(path/to/rp2040-ota-bootloader)

project(my_project C CXX ASM)
pico_sdk_init()

add_executable(my_firmware main.c)
target_link_libraries(my_firmware PRIVATE pico_stdlib)

# Attach boot3 to your target: injects linker script and post-build checksum step
pico_boot3_init(my_firmware)
```

`pico_boot3_init` does three things automatically:
- Switches the linker script to the generated `boot3_memmap.ld` (which reserves the boot3 section)
- Links your target against the `boot3` interface library (includes compiled boot3 objects)
- Adds a post-build command that computes the FNV1A-64 checksum of the binary and embeds it into `.boot3_state_checksum` in the ELF

### CMake options

| Option | Default | Description |
|---|---|---|
| `BOOT3_INJECT_MEMMAP` | `ON` | Generate an injected `memmap_default.ld` with the `.boot3` section. Disable only if managing the linker script manually. |
| `BOOT3_AUTO_UPDATE` | `ON` | Automatically update firmware from pending state if checksum is different. When disabled update is only tried to be installed when user request it. |
| `BOOT3_WS2812_ENABLE` | `OFF` | Compile WS2812 LED driver into boot3. LED feedback during update. |
| `BOOT3_STATUS_ENABLE` | `OFF` | Enable status/program LED GPIOs during boot. |
| `BOOT3_WS2812_GPIO` | *(empty)* | GPIO pin for WS2812 data. Falls back to `PICO_DEFAULT_WS2812_PIN`. |
| `BOOT3_STATUS_LED_GPIO` | *(empty)* | GPIO for status LED. Falls back to `PICO_DEFAULT_LED_PIN`. |
| `BOOT3_PROGRAM_LED_GPIO` | *(empty)* | GPIO for programming-in-progress LED. Falls back to `PICO_DEFAULT_LED_PIN`. |

## LED Feedback (optional)

When `BOOT3_STATUS_ENABLE` and `BOOT3_WS2812_ENABLE` are enabled:

| LED colour | Meaning |
|---|---|
| Green | boot3 initialised, checking state, usually visible only ehen something hangs, as it quickly switch to blue |
| Blue | boot3 checking state and calculating current and pending checksums |
| Red | boot3 programming (copying) pending state to current |
| Red blinking | blinks every one page (4096 bytes) is programmed into current state |

## Limitations

- Flash must be at least **2× the application size** (the flash is split in half).
- The entire firmware binary (including boot2 and boot3) must be provided in the OTA image so that the pending state can be applied correctly.
- Flash operations must be performed with interrupts disabled and while executing from RAM (the bootloader handles this internally for its own copy routine; your application must do the same when calling `boot3_flash_erase_pending_data` / `boot3_flash_program_pending_data`).
- The boot3 code + state region occupies the first **8 KiB** of flash; application code starts at offset `0x2000`.

## Roadmap

Note: The following items are not listed by priority.

- [x] Add support for triggering a bootloader update via a watchdog scratch register, allowing the user to explicitly control whether an update should occur after reboot (currently, an update is always performed if the checksum differs)
- [ ] Add support for RP2350 (some code is currently hardcoded for RP2040)
- [ ] Implement pending program signature verification to ensure only authorized firmware can be flashed to the current state
- [ ] Consider adding support for bootloader reprogramming
- [ ] Fix a known issue where the checksum and program size are not injected on the first build; currently, a second build is sometimes required for correct injection.
- [ ] What is the expected behavior if both the current and pending states are invalid?
- [ ] Optimize bootloader code to reduce its flash size (we are already using >80% of 4kB)

---

## How It Works

The bootloader splits flash into two equal halves:

| Region | Content |
|---|---|
| Flash start → Flash/2 | **Current** (running) program |
| Flash/2 → Flash end | **Pending** (staged) program |

At every boot, `boot3` runs before the main application. It checks whether a valid pending firmware exists in the second half of flash and, if an update is required, copies it over the current slot and launches it. If no update is needed, the bootloader transparently hands off execution to the existing application with no measurable overhead.

**Continuous Integrity Verification:** Unlike simpler bootloaders, boot3 verifies the application's FNV1A-64 checksum on every single boot. This ensures that the device will never attempt to execute corrupted firmware (e.g., due to partial flash failure or accidental overwrite).

### Boot sequence

```
Power-on / Reset
      │
      ▼
  boot2 (256 B) ── built-in ROM second-stage
      │
      ▼
  boot3 entry  ── initialises XOSC, copies critical code to SRAM
      │
      ▼
  Check pending state
    ├─ Invalid or no update needed ──► Launch current application
    └─ Valid + update required
           │
           ▼
         Verify pending checksum (FNV1A-64)
           ├─ Mismatch ──► Launch current application
           └─ Match
                  │
                  ▼
                Erase current slot, copy pending → current (with progress tracking)
                  │
                  ▼
                Copy state metadata, relaunch
```

### Power-loss resilience

Copy progress is recorded byte-by-byte in the 2 KiB progress region of the boot3 state (each byte represents one 4 KiB sector). If power is lost mid-copy, the bootloader detects the incomplete current state on the next boot (checksum mismatch) and resumes or retries the copy from the pending slot, which is never erased until the copy is verified complete.

## Flash Memory Layout

```
┌────────────────────────────────────────┐  0x00000000
│  boot2  (256 B)                        │
├────────────────────────────────────────┤  0x00000100
│  boot3 code  (up to ~3.75 KiB)        │
│  boot3 critical section (in RAM)       │
├────────────────────────────────────────┤  0x00001000
│  boot3 state  (4 KiB)                 │
│   ├─ checksum      (8 B)              │
│   ├─ program_size  (4 B)              │
│   ├─ magic         (4 B)              │
│   ├─ internal data (≤1 KiB)           │
│   ├─ user data     (1 KiB)            │
│   └─ copy progress (2 KiB)            │
├────────────────────────────────────────┤  0x00002000
│  Current application                   │
│  (Flash/2 - 8 KiB available)           │
├────────────────────────────────────────┤  Flash/2
│  Pending application (staged OTA)      │
│  (mirrored layout — also has state)    │
└────────────────────────────────────────┘  Flash end
```

> **Note:** `PICO_FLASH_SIZE_BYTES` is automatically halved by the CMake integration so the SDK and application code always address only the current slot.

## Project Structure

```
rp2040-ota-bootloader/
├── src/
│   ├── boot3.S              # Assembly entry point; provides vector table for boot3
│   ├── boot3.h              # Public API header
│   ├── boot3.c              # Public flash API (erase, program, validate)
│   ├── boot3_internal.c     # Core bootloader logic (state check, copy, launch)
│   ├── boot3_internal.h     # Internal macros and section attributes
│   ├── boot3_state.c        # State structure initialisation and checksum embedding
│   ├── boot3_flash.c        # Low-level flash erase/program (executes from RAM)
│   ├── boot3_ws2812.c/h     # Optional bit-banged WS2812 LED driver
│   └── CMakeLists.txt
├── tools/
│   └── fnv1a_checksum/      # Host tool: computes FNV1A-64 of a binary (post-build)
├── example/
│   ├── main.c               # USB serial OTA example
│   ├── ws2812.c/h           # WS2812 helper for the example
│   └── CMakeLists.txt
├── boot3.ld                 # Linker script fragment injected into pico SDK memmap
└── CMakeLists.txt           # Main build; exposes `boot3` interface library
```

## Public API

Include `boot3.h` in your application code.

### State access

```c
// Get a pointer to the current (running) boot3 state in flash
const struct Boot3State *boot3_get_current_state(void);

// Get a pointer to the pending (staged) boot3 state in flash
const struct Boot3State *boot3_get_pending_state(void);
```

### Flash operations (call with interrupts disabled of flash_safe_execute function)

```c
// Erase flash in the pending slot for a program of `len` bytes.
// Always erases at least two sectors (to clear the pending magic + checksum).
bool boot3_flash_erase_pending_data(size_t len);

// Program `len` bytes from `data` into the pending slot at `offset`.
// `len` must be a multiple of FLASH_PAGE_SIZE (256 bytes).
void boot3_flash_program_pending_data(size_t offset, const uint8_t *data, size_t len);
```

### Validation

```c
// Returns true if `state` has a valid magic, a sane program_size,
// and a checksum that matches the program data in flash.
bool boot3_validate_state(const struct Boot3State *state);
```

### Checksum

```c
// Compute a FNV1A-64 hash using the function pointer stored in the current state.
// The function is stored in the bootloader, so the user program cannot access it directly.
// In the future, if bootloader reprogramming is supported, the address may not be static.
uint64_t boot3_fnv1a_64(const uint8_t *data, size_t len);
```

### User callbacks (can be overriden in your application)

```c
// Weak function — override to customise when an update is applied.
// `checksum_mismatch` is true when the pending checksum differs from the current one.
//
// Return true to apply the pending update, false to skip it.
//
// Default behaviour (when not overridden): update whenever checksums differ.
bool boot3_should_update(bool checksum_mismatch);
```

**WARNING:** These functions run in the bootloader context. At this stage, user application code and RAM data are NOT loaded and MUST NOT be accessed.
Attempting to use user code or RAM data will result in a corrupted state and undefined behavior.

**Note:** These functions cannot be placed in RAM, as they will not be loaded there when the bootloader starts. If the function needs to use any peripherals, it must initialize them itself. Accessing GPIO, ADC, or other hardware is possible, but keep in mind that these peripherals are almost certainly uninitialized at this stage.

### User data

Place persistent data in the 1 KiB user data region of the boot3 state:

```c
struct MyConfig {
    uint32_t version;
    // ... up to 1020 more bytes
} in_boot3_user_data my_config = {
    .version = 1,
};
```

The `in_boot3_user_data` macro places the variable in `.boot3_state.user_data`. This data is embedded in the firmware image and therefore also exists in the pending slot, enabling the update callback to compare versions between slots:

```c
bool boot3_should_update(bool checksum_mismatch) {
    struct MyConfig *current = (struct MyConfig *)boot3_get_current_state()->user_data;
    struct MyConfig *pending  = (struct MyConfig *)boot3_get_pending_state()->user_data;
    return pending->version > current->version;
}
```

## Writing a Firmware Update (Transport-Agnostic)

Regardless of how your device receives the new firmware (UART, USB, BLE, Wi-Fi, …), the flash-side procedure is always the same:

```c
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "boot3.h"

// 1. Receive the complete firmware image into a RAM buffer (protocol-specific)
uint8_t *new_firmware = ...; // full binary including boot2+boot3
size_t   firmware_len  = ...; // total byte count

// 2. Erase the pending slot (interrupts must be disabled)
uint32_t ints = save_and_disable_interrupts();
boot3_flash_erase_pending_data(firmware_len);
restore_interrupts(ints);

// 3. Write the new firmware page by page
size_t pages = (firmware_len + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
ints = save_and_disable_interrupts();
boot3_flash_program_pending_data(0, new_firmware, pages * FLASH_PAGE_SIZE);
restore_interrupts(ints);

// 4. Validate before committing
if (boot3_validate_state(boot3_get_pending_state())) {
    // 5. Reboot — boot3 will detect the valid pending state and apply it
    watchdog_enable(1, true);
    for (;;) tight_loop_contents();
}
```

## Example: USB Serial OTA

The `example/` directory contains a complete reference implementation for the **Waveshare RP2040-Zero** board. It listens on USB CDC for a raw firmware binary, programs the pending slot, then reboots.

### Build

```bash
cd example
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Flash (first time, via UF2)

```bash
# Copy the UF2 to the board in BOOTSEL mode
cp example_serial_ota.uf2 /media/$USER/RPI-RP2/
```

### Send a firmware update

```bash
cat your_firmware.bin > /dev/ttyACM0
```

The device will:
1. Boot with a **blue** WS2812 LED
2. Wait up to **10 seconds** for incoming firmware data on USB serial (green LED)
3. Program and validate the pending slot (yellow LED)
4. Reboot (red LED briefly)
5. boot3 applies the update and launches the new firmware

## Boot3 State Structure

```c
struct Boot3State {
    struct Boot3StatePrelude {
        uint64_t checksum;      // FNV1A-64 of the application binary
        uint32_t program_size;  // size of the application binary in bytes
        uint32_t magic;         // 0x553007B0 — marks a valid state
    } prelude;

    struct Boot3StateInternalData {
        struct Boot3StateConfig {
            uint8_t *flash_binary_start;
            uint8_t *flash_binary_end;
            bool (*should_update)(bool checksum_mismatch);
            uint64_t (*fnv1a_64)(const uint8_t *data, size_t len);
        } config;
        char padding[...]; // padded to exactly 1 KiB
    } data;

    char user_data[1024];               // application-defined persistent data

    struct Boot3StateCopyProgress {
        char data[2048];                // one byte per 4 KiB sector, tracks copy
    } progress;
};
// Total: exactly 4096 bytes
```

## Tools

### `fnv1a_checksum` (host tool)

A small host-side utility used by the CMake post-build step to compute the FNV1A-64 checksum of a firmware binary and embed it into the ELF.

```
Usage: fnv1a_checksum <input.bin> <start_byte> [length|"calc"]

  start_byte   byte offset in the binary where hashing begins (e.g. 0x2000 to skip boot2+boot3)
  length       number of bytes to hash (default: remainder of file)
  calc         instead of computing a hash, output the remaining byte count as a 4-byte LE integer
               (used to populate boot3_state_program_size)
```
## License

This project is licensed under the **MIT License** — see [LICENSE](LICENSE) for details.

Portions of [`src/boot3_flash.c`](src/boot3_flash.c) are derived from the
[Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
(`hardware_flash/flash.c`), which is Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
and licensed under the **BSD 3-Clause License**. The full BSD-3-Clause text is
reproduced in the [LICENSE](LICENSE) file.
