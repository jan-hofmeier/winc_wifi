cmake_minimum_required(VERSION 3.13)

# Import the Pico SDK
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(pico_winc_boot C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Add your executable
add_executable(winc_bootloader
    winc_bootloader.c
)

# Link to pico_stdlib (for gpio, time, stdio)
target_link_libraries(winc_bootloader 
    pico_stdlib
)

# Enable USB stdio for printf() debugging
pico_enable_stdio_usb(winc_bootloader 1)
pico_enable_stdio_uart(winc_bootloader 0)

# Create the .uf2 file
pico_add_extra_outputs(winc_bootloader)
