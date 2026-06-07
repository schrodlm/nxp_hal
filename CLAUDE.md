# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

A course assignment scaffold for implementing a Hardware Abstraction Layer (HAL) on the **NXP S32K118** (ARM Cortex-M0+) microcontroller. NXP provides no vendor HAL — only CMSIS-style register definitions. The student writes `src/lib/*.c` files against fully specified `src/lib/*.h` headers (Doxygen contracts: pin mapping, ISR rules, listener semantics) so that the provided `src/eXX_*/main.c` exercise programs behave correctly on the board.

## Toolchain & build

Cross-compilation only — never try to run binaries on the host.

- **Toolchain:** ARM GNU toolchain pinned to `.dependencies/arm-gnu-toolchain/bin/arm-none-eabi-gcc`. Install it with `./setup-toolchain.sh` (downloads version 15.2.rel1 into `.dependencies/`). `CMakeLists.txt` hardcodes this path, so the toolchain must live there — a system-wide `arm-none-eabi-gcc` will not be picked up.
- **Build:** Out-of-source CMake build. Typical flow:
  ```
  cmake -S . -B build -G Ninja
  cmake --build build --target e01_led.elf
  ```
- **Flashing:** Each exercise gets an auto-generated `UP_<target>.elf` custom target that invokes OpenOCD over CMSIS-DAP/SWD. **Build** the `UP_*` target (do not "run" it):
  ```
  cmake --build build --target UP_e01_led.elf
  ```
  OpenOCD ≥ 0.12.0+dev is required (older distro packages lack S32K support).
- **Artifacts per target:** `.elf` (with debug syms), `.hex` (for OpenOCD), `.map`, and `.asm` (full disassembly — useful for diagnosing subtle HAL bugs).
- **Doxygen target:** `cmake --build build --target docs` if Doxygen is installed.

## Adding a new exercise/module

Don't hand-edit per-target boilerplate. Use the `ers_s32_exercise(<name> SOURCES … [DEFINITIONS …])` and `ers_s32_test(<name> SOURCES …)` helpers in `CMakeLists.txt`. The test helper auto-prefixes `TST_`, links Unity, and defines `UNITY_INCLUDE_CONFIG_H`. Both helpers automatically create the matching `UP_*` flashing target.

The DMA variant of an exercise is just a second `ers_s32_exercise(...)` call with `DEFINITIONS SPI_WITH_DMA` (see `e07_lcd-dma`).

## Architecture

```
devices/        NXP CMSIS register headers, S32K118 startup, GCC linker script (S32K118_25_flash.ld)
src/core/       Provided — clock (40 MHz), 1 ms SysTick, debug UART, syscalls. system_init() bootstraps all three.
src/lib/        Headers fully specify the HAL. The student implements the .c files. lcd_font.{c,h} is provided.
src/eXX_*/      Per-exercise main.c (+ optional test.c). Never modify; never add new mains.
unity/          Unity test framework, linked into every TST_* target.
docs/           S32K reference manual, Cortex-M0+ docs, cookbook PDF.
ers-board-v1.2/ KiCad sources & schematic for the carrier board (jumper config, pin assignments).
```

Two patterns recur across the HAL and must be respected when implementing modules:

- **SysTick listener chain** (`src/core/systick.h`): modules that need periodic work (e.g. `btn_cnt`) register a *statically-allocated* `systick_listener_t` node via `systick_register_listener()`. Callbacks run in ISR context, in reverse registration order. There is no unregister.
- **SPI callback contract** (`src/lib/spi.h`): the DMA path delivers TX and RX completion via **two independent callbacks**, not one combined event. The LCD driver chains DC/CS toggles inside those callbacks rather than inline after `spi_dma_txrx()`, because nothing has been clocked out yet at the call site. DMA buffers must be 4-byte aligned; tail bytes (`count & 3`) are handled by a scatter-gather TCD on TX and by the LPSPI frame-complete IRQ on RX (since RX DMA only fires on full 32-bit words). `LPSPI_TCR_BYSW=1` is required because LPSPI shifts MSB-first while the core is little-endian.

## Serial output

That to **LPUART1 at 1 000 000 baud, 8N1** on **PTC7 (TX) / PTC6 (RX)**. Configure the terminal accordingly. Constraints when writing code that prints:

- Always terminate messages with `\n` (or `fflush(stdout)`) — `stdout` is line-buffered and unterminated messages may never reach the UART.
- Do not call `printf` from ISRs; LPUART1 blocks when its TX FIFO is full. Use `debug_uart_putchar()` for single chars, or set a flag and print from the main loop.
- Do not use `%f`. Use integer / fixed-point formatting.

## Reference manual workflow

The intended development loop for each module:

1. Read the corresponding `src/lib/<mod>.h` end to end — pin mapping, ISR rules, preconditions are all there.
2. Look up the peripheral chapter in `docs/S32K-RM.pdf`. Register macro names in `devices/S32K118/include/S32K118.h` match the manual exactly. The manual's **PDF attachments** (open in Foxit/Acrobat, not the browser) contain the pin-mux and DMAMUX-source tables.
3. Implement the `.c` under `src/lib/`. No CMake changes needed — `CMakeLists.txt` already lists expected filenames.
4. Build `UP_eXX_*.elf` to flash and observe behavior.
5. If a `TST_eXX_*` target exists, flashing it via `UP_TST_eXX_*.elf` is **mandatory** — a module is only considered correct when its Unity test reports all `PASS` on the UART. Required wiring (jumper wires) is documented at the top of each `test.c`.

## AI-assistance ground rules (from README §4.1)

This is an academic assignment. The course explicitly permits AI assistance but requires that:
- **Every register write must be manually cross-checked against `docs/S32K-RM.pdf`.** AI models routinely produce plausible-looking but subtly wrong configurations for `PORT->PCR` MUX fields, `PCC` clock gates, peripheral enable bits, DMAMUX source numbers, and NVIC settings.
- **The student must be able to explain every bit of every register assignment** during evaluation. Prefer code that mirrors the manual's structure (one register write per logical step, named bitfield macros over magic numbers) so it can be defended line by line.

When generating HAL code here, lean toward verbose, manual-traceable register writes rather than clever abstractions, and call out which RM section a configuration came from in conversation (not as code comments).
