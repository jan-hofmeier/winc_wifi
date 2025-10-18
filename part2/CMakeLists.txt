# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

option(USE_USB_MSC "Enable USB Mass Storage support" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(winc_wifi C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable with common sources
add_executable(winc_wifi winc_pico_part2.c winc_wifi.c winc_sock.c winc_flash.c)

# Pass the build option to the C preprocessor
target_compile_definitions(winc_wifi PRIVATE USE_USB_MSC=${USE_USB_MSC})

pico_set_program_name(winc_wifi "winc_wifi")
pico_set_program_version(winc_wifi "0.1")

pico_enable_stdio_uart(winc_wifi 0)

# Add common libraries
target_link_libraries(winc_wifi pico_stdlib hardware_spi)

if(USE_USB_MSC)
    message(STATUS "Building with USB Mass Storage support")
    # Add MSC-specific sources
    target_sources(winc_wifi PRIVATE msc_disk.c usb_descriptors.c)

    # Link against TinyUSB libraries
    target_link_libraries(winc_wifi tinyusb_board tinyusb_device)

    # Add include paths for TinyUSB
    target_include_directories(winc_wifi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(winc_wifi PRIVATE ${PICO_SDK_PATH}/lib/tinyusb/hw)

    # Add definitions for TinyUSB classes
    add_definitions(-D CFG_TUD_MSC -D CFG_TUD_CDC -D CFG_TUD_MSC_EP_BUFSIZE=512)
else()
    message(STATUS "Building in Serial-only mode")
    # Use standard Pico SDK USB serial
    pico_enable_stdio_usb(winc_wifi 1)
endif()

pico_add_extra_outputs(winc_wifi)

