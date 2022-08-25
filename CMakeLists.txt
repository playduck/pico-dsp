# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

include(pico_extras_import.cmake)

project(pico-dsp C CXX ASM)
set(CMAKE_CXX_FLAGS "-O3 -march=armv6-m -mcpu=cortex-m0plus -mthumb -ffunction-sections -fdata-sections -fno-exceptions -Wall -Wextra")

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

include_directories(src)
add_executable(pico-dsp
        dsp.cpp
)

target_sources(pico-dsp PRIVATE
        # I2S-client.h
        # I2S-client.cpp
        src/I2S.cpp
        src/I2S.h
        src/AudioRingBuffer.cpp
        src/AudioRingBuffer.h
        src/iir.cpp
        src/iir.h
        # src/pio_i2s.pio.h
        # src/pio_i2s.pio
        src/compatability.h
)

pico_generate_pio_header(pico-dsp ${CMAKE_CURRENT_LIST_DIR}/src/pio_i2s.pio)

pico_set_program_name(pico-dsp "dsp")
pico_set_program_version(pico-dsp "0.1")

pico_enable_stdio_uart(pico-dsp 1)
pico_enable_stdio_usb(pico-dsp 0)

# Add the standard library to the build
target_link_libraries(pico-dsp pico_stdlib)

# Add any user requested libraries
target_link_libraries(pico-dsp
        hardware_pio
        hardware_dma
        pico_audio_i2s
        pico_multicore
)

pico_add_extra_outputs(pico-dsp)
# pico_set_binary_type(pico-dsp no_flash)
# pico_set_binary_type(pico-dsp copy_to_ram)
