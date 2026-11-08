cmake_minimum_required(VERSION 3.16)

# Command-line builds can opt out to use their own SDK/toolchain paths.
option(PENDANT_USE_PICO_VSCODE "Use the Pico extension's installed tool versions" ON)
# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode "${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake")
if(PENDANT_USE_PICO_VSCODE AND EXISTS "${picoVscode}")
    include("${picoVscode}")
endif()
# ====================================================================================

# The extension configures Ninja without a build type on first open.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
set(PICO_BOARD pico CACHE STRING "RP2040 target; touch-board pins are in DEV_Config.h")
set(PICO_FLASH_SIZE_BYTES 4194304)
include(pico_sdk_import.cmake)
project(flip_rp2040 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
pico_sdk_init()
set(PENDANT_SYS_KHZ 133000 CACHE STRING "System frequency, default rated 133 MHz")
set(PENDANT_SPI_HZ 66500000 CACHE STRING "Requested LCD SPI baud; actual divider reported over USB")
set(FLUID_RENDER_SCALE 4 CACHE STRING "Pixel scale: 4 = 60px pixel art; 2 = 120px; 1 = native 240px")
set(PENDANT_IMU_SWAP 1 CACHE STRING "Swap screen gravity axes")
set(PENDANT_IMU_X_SIGN 1 CACHE STRING "Screen X gravity sign")
set(PENDANT_IMU_Y_SIGN -1 CACHE STRING "Screen Y gravity sign")
add_subdirectory(lib/Config)
add_subdirectory(lib/LCD)
add_subdirectory(lib/sensor_i2c)
add_subdirectory(lib/qmi8658)
add_subdirectory(lib/cst816)
add_executable(flip_rp2040
    src/main.c src/input.c src/display.c src/console.c src/benchmark.c src/telemetry.c
    src/fluid.c src/render.c src/fixed_math.c)
target_include_directories(flip_rp2040 PRIVATE include lib/Config lib/LCD)
target_compile_options(flip_rp2040 PRIVATE $<IF:$<CONFIG:Debug>,-Og,-O3> -Wall -Wextra)
target_compile_definitions(flip_rp2040 PRIVATE
    PENDANT_SYS_KHZ=${PENDANT_SYS_KHZ}
    PENDANT_SPI_HZ=${PENDANT_SPI_HZ}
    PICO_STDIO_USB_STDOUT_TIMEOUT_US=1000
    FLUID_RENDER_SCALE=${FLUID_RENDER_SCALE}
    PENDANT_IMU_SWAP=${PENDANT_IMU_SWAP}
    PENDANT_IMU_X_SIGN=${PENDANT_IMU_X_SIGN}
    PENDANT_IMU_Y_SIGN=${PENDANT_IMU_Y_SIGN})
target_link_libraries(flip_rp2040 LCD Config qmi8658 cst816 pico_stdlib pico_multicore pico_util hardware_clocks hardware_watchdog)
pico_enable_stdio_usb(flip_rp2040 1)
pico_enable_stdio_uart(flip_rp2040 0)
pico_add_extra_outputs(flip_rp2040)

# Use the selected SDK's register description without a versioned path in launch.json.
configure_file("${PICO_SDK_PATH}/src/rp2040/hardware_regs/RP2040.svd"
               "${CMAKE_BINARY_DIR}/RP2040.svd" COPYONLY)
