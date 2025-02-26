cmake_minimum_required(VERSION 3.20)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(usb-sha256 C CXX ASM)

# paths

set(BOARD_INCLUDE_DIRECTORY ${PROJECT_SOURCE_DIR}/board/include)
set(USBSHA256_INCLUDE_DIRECTORY ${PROJECT_SOURCE_DIR}/usb-sha256/include)
include(${PROJECT_SOURCE_DIR}/cmake/stm32cubef4.cmake)

# libraries

add_subdirectory(${PROJECT_SOURCE_DIR}/startup)
add_subdirectory(${PROJECT_SOURCE_DIR}/board)
add_subdirectory(${PROJECT_SOURCE_DIR}/usb-sha256)

# main ELF file

add_executable(main
    ${PROJECT_SOURCE_DIR}/main.c
    ${PROJECT_SOURCE_DIR}/syscalls.c
    ${PROJECT_SOURCE_DIR}/sysmem.c
)
target_compile_definitions(main PRIVATE STM32F411xE)
target_include_directories(main PRIVATE
    ${BOARD_INCLUDE_DIRECTORY}
    ${USBSHA256_INCLUDE_DIRECTORY}
    ${CMSIS_INCLUDE_DIRECTORIES}
    ${HAL_INCLUDE_DIRECTORIES}
    ${USBDEVLIB_INCLUDE_DIRECTORIES}
)
target_compile_options(main PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -ffunction-sections
    -fdata-sections
)
set_target_properties(main PROPERTIES OUTPUT_NAME main.elf)
target_link_libraries(main PRIVATE startup board usb-sha256)
target_link_options(main PRIVATE
    -Wl,-T ${PROJECT_SOURCE_DIR}/linker/STM32F411CEUX_FLASH.ld
    -Wl,--fatal-warnings
    -Wl,--gc-sections
    -nostartfiles
)

# strip ELF file

find_program(CMAKE_STRIP arm-none-eabi-strip REQUIRED)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/stripped.elf
    DEPENDS main
    COMMAND ${CMAKE_STRIP} main.elf -o stripped.elf
)
add_custom_target(stripped DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/stripped.elf)

# make firmware file

find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy REQUIRED)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/main.bin
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/stripped.elf
    COMMAND ${CMAKE_OBJCOPY} -O binary stripped.elf main.bin
)
add_custom_target(main_bin ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/main.bin)

# flash device

message(CHECK_START "Check whether st-flash is installed")
find_program(CMAKE_ST_FLASH st-flash)
if (CMAKE_ST_FLASH-NOTFOUND)
    message(CHECK_FAIL "not found")
else()
    message(CHECK_PASS "found")
    add_custom_target(
        flash
        COMMAND ${CMAKE_ST_FLASH} --connect-under-reset write main.bin 0x08000000
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/main.bin
    )
endif()
