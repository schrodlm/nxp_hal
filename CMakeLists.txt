SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)

cmake_minimum_required(VERSION 3.30)

set(ARM_BINARIES "${CMAKE_CURRENT_SOURCE_DIR}/.dependencies/arm-gnu-toolchain/bin")
set(OPENOCD "${CMAKE_CURRENT_SOURCE_DIR}/.dependencies/openocd/bin/openocd")

# Specify the cross compiler before project()
SET(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_C_COMPILER "${ARM_BINARIES}/arm-none-eabi-gcc")
SET(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER "${ARM_BINARIES}/arm-none-eabi-g++")
set(AS "${ARM_BINARIES}/arm-none-eabi-as")
set(AR "${ARM_BINARIES}/arm-none-eabi-ar")
set(OBJCOPY "${ARM_BINARIES}/arm-none-eabi-objcopy")
set(OBJDUMP "${ARM_BINARIES}/arm-none-eabi-objdump")
set(SIZE "${ARM_BINARIES}/arm-none-eabi-size")

# MCU configuration
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/devices/S32K118/linker/gcc/S32K118_25_flash.ld)
set(MCPU cortex-m0plus)

# Compiler flags (must be set before project())
set(COMMON_FLAGS "-mcpu=${MCPU} -mthumb -ffunction-sections -fdata-sections -fmessage-length=0 -specs=nano.specs")

set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS} -Wall")
set(CMAKE_ASM_FLAGS_INIT "${COMMON_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,-gc-sections,--print-memory-usage -nostartfiles -T${LINKER_SCRIPT}")

project(ers_s32 C ASM)
set(CMAKE_C_STANDARD 23)

# Emit compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Common compiler definitions
add_compile_definitions(CPU_S32K118 START_FROM_FLASH)

# Debug/Release flags
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-O2 -g3")

# Include directories
include_directories(
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/devices
        ${CMAKE_SOURCE_DIR}/unity
)

# Common source files shared by all projects
set(SOURCES_CORE
        devices/startup.c
        devices/S32K118/startup/system_S32K118.c
        devices/S32K118/startup/gcc/startup_S32K118.S
        src/core/syscalls.c
        src/core/sysmem.c
        src/core/clock.c
        src/core/debug_uart.c
        src/core/systick.c
        src/core/system.c
)

# Doxygen documentation target
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
            WORKING_DIRECTORY ..
            COMMENT "Generating Doxygen documentation for src/core and src/lib"
            VERBATIM
    )
endif()

# Function to create a project target
# Usage: ers_s32_exercise(target_name SOURCES source1 source2... [DEFINITIONS def1 def2...])
function(ers_s32_exercise TARGET_NAME)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs SOURCES DEFINITIONS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Create executable with core sources + project-specific sources
    add_executable(${TARGET_NAME}.elf ${SOURCES_CORE} ${ARG_SOURCES} ${LINKER_SCRIPT})

    # Apply project-specific definitions if provided
    if(ARG_DEFINITIONS)
        target_compile_definitions(${TARGET_NAME}.elf PRIVATE ${ARG_DEFINITIONS})
    endif()

    # Linker flags
    set_target_properties(${TARGET_NAME}.elf PROPERTIES
            LINK_FLAGS "-Wl,-Map=${PROJECT_BINARY_DIR}/${TARGET_NAME}.map,--cref"
    )

    # Generate HEX file
    add_custom_command(TARGET ${TARGET_NAME}.elf POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${TARGET_NAME}.elf> ${PROJECT_BINARY_DIR}/${TARGET_NAME}.hex
            COMMENT "Building ${TARGET_NAME}.hex"
    )

    # Generate disassembly listing
    add_custom_command(TARGET ${TARGET_NAME}.elf POST_BUILD
            COMMAND ${CMAKE_OBJDUMP} -d -S -l $<TARGET_FILE:${TARGET_NAME}.elf> > ${PROJECT_BINARY_DIR}/${TARGET_NAME}.asm
            COMMENT "Generating ${TARGET_NAME} disassembly listing..."
    )

    # Create upload target
    add_custom_target(UP_${TARGET_NAME}.elf
            COMMAND ${OPENOCD} -f interface/cmsis-dap.cfg -c "transport select swd" -f target/s32k.cfg -c "init\; kinetis fopt 0x7f\; program ${TARGET_NAME}.hex verify reset exit"
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMENT "Uploading ${TARGET_NAME}.hex to target"
    )

    add_dependencies(UP_${TARGET_NAME}.elf ${TARGET_NAME}.elf)
endfunction()

# Function to create a test target
# Usage: ers_s32_test(target_name SOURCES source1 source2... [DEFINITIONS def1 def2...])
function(ers_s32_test TARGET_NAME)
    # Call ers_s32_exercise with TEST_ prefix and UNITY_INCLUDE_CONFIG_H definition
    ers_s32_exercise(TST_${TARGET_NAME} ${ARGN} SOURCES unity/unity.c DEFINITIONS UNITY_INCLUDE_CONFIG_H)
endfunction()



ers_s32_exercise(e00_starter
    SOURCES
        src/e00_starter/main.c
)

ers_s32_exercise(e01_led
    SOURCES
        src/lib/led.c
        src/e01_led/main.c
)

ers_s32_test(e01_led
    SOURCES
        src/lib/led.c
        src/e01_led/test.c
)

ers_s32_exercise(e02_btn
    SOURCES
        src/lib/led.c
        src/lib/btn.c
        src/e02_btn/main.c
)

ers_s32_test(e02_btn
    SOURCES
        src/lib/btn.c
        src/e02_btn/test.c
)

ers_s32_exercise(e03_btn_cnt
        SOURCES
        src/lib/led.c
        src/lib/btn.c
        src/lib/btn_cnt.c
        src/e03_btn_cnt/main.c
)

ers_s32_test(e03_btn_cnt
        SOURCES
        src/lib/btn.c
        src/lib/btn_cnt.c
        src/e03_btn_cnt/test.c
)

ers_s32_exercise(e04_kpad
        SOURCES
        src/lib/led.c
        src/lib/kpad.c
        src/e04_kpad/main.c
)

ers_s32_exercise(e05_enc
        SOURCES
        src/lib/led.c
        src/lib/enc.c
        src/e05_enc/main.c
)

ers_s32_exercise(e06_servo
        SOURCES
        src/lib/led.c
        src/lib/enc.c
        src/lib/servo.c
        src/e06_servo/main.c
)

# ers_s32_exercise(e07_lcd
#         SOURCES
#         src/lib/led.c
#         src/lib/spi.c
#         src/lib/lcd.c
#         src/lib/lcd_font.c
#         src/e07_lcd/main.c
# )

# ers_s32_exercise(e07_lcd-dma
#         SOURCES
#         src/lib/led.c
#         src/lib/spi.c
#         src/lib/lcd.c
#         src/lib/lcd_font.c
#         src/e07_lcd/main.c
#         DEFINITIONS
#         SPI_WITH_DMA
# )

# ers_s32_exercise(e08_eeprom
#         SOURCES
#         src/lib/led.c
#         src/lib/spi.c
#         src/lib/eeprom.c
#         src/e08_eeprom/main.c
# )

# ers_s32_test(e08_eeprom
#         SOURCES
#         src/lib/spi.c
#         src/lib/eeprom.c
#         src/e08_eeprom/test.c
# )


# ers_s32_exercise(e09_joy
#         SOURCES
#         src/lib/led.c
#         src/lib/joy.c
#         src/e09_joy/main.c
# )

# ers_s32_exercise(e10_sht
#         SOURCES
#         src/lib/led.c
#         src/lib/btn.c
#         src/lib/enc.c
#         src/lib/spi.c
#         src/lib/lcd.c
#         src/lib/lcd_font.c
#         src/lib/eeprom.c
#         src/lib/sht40.c
#         src/e10_sht/main.c
# )

# ers_s32_exercise(e11_can
#         SOURCES
#         src/lib/led.c
#         src/lib/btn.c
#         src/lib/enc.c
#         src/lib/joy.c
#         src/lib/spi.c
#         src/lib/lcd.c
#         src/lib/lcd_font.c
#         src/lib/eeprom.c
#         src/lib/sht40.c
#         src/lib/can.c
#         src/e11_can/main.c
# )

# # Add all the necessary source files for the punch press exercise

# ers_s32_exercise(e12_punchpress
#         SOURCES
#         src/e12_punchpress/main.c
# )
