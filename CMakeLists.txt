cmake_minimum_required(VERSION 3.20)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(usb C CXX ASM)

set(CMAKE_ASM_FLAGS_DEBUG "")
set(CMAKE_ASM_FLAGS_RELEASE "")
set(CMAKE_ASM_FLAGS_MINSIZEREL "")
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "")

set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INC_DIR ${PROJECT_SOURCE_DIR}/include)

set(STM43CUBEF4_DIR ${PROJECT_SOURCE_DIR}/../STM32CubeF4)
set(CMSIS_DIR ${STM43CUBEF4_DIR}/Drivers/CMSIS)
set(CMSIS_INCLUDE_DIRECTORIES
    ${CMSIS_DIR}/Include
    ${CMSIS_DIR}/Device/ST/STM32F4xx/Include
)
set(CMSIS_TEMPLATES_DIR ${CMSIS_DIR}/Device/ST/STM32F4xx/Source/Templates)
set(HAL_DIR ${STM43CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver)
set(HAL_INCLUDE_DIRECTORIES ${HAL_DIR}/Inc)
set(HAL_SRC_DIR ${HAL_DIR}/Src)
set(USBDEVLIB_DIR ${STM43CUBEF4_DIR}/Middlewares/ST/STM32_USB_Device_Library)
set(USBDEVLIB_INCLUDE_DIRECTORIES
    ${USBDEVLIB_DIR}/Core/Inc
    ${USBDEVLIB_DIR}/Class/CDC/Inc
)
set(USBDEVLIB_SRC_DIR ${USBDEVLIB_DIR}/Core/Src)
set(USBDEVLIB_CDC_SRC_DIR ${USBDEVLIB_DIR}/Class/CDC/Src)

set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/linker/STM32F411CEUX_FLASH.ld)

add_executable(main
    ${CMSIS_TEMPLATES_DIR}/system_stm32f4xx.c
    ${HAL_SRC_DIR}/stm32f4xx_hal.c
    ${HAL_SRC_DIR}/stm32f4xx_hal_cortex.c
    ${HAL_SRC_DIR}/stm32f4xx_hal_gpio.c
    ${HAL_SRC_DIR}/stm32f4xx_hal_pcd.c
    ${HAL_SRC_DIR}/stm32f4xx_hal_pcd_ex.c
    ${HAL_SRC_DIR}/stm32f4xx_hal_rcc.c
    ${HAL_SRC_DIR}/stm32f4xx_hal_tim.c
    ${HAL_SRC_DIR}/stm32f4xx_hal_tim_ex.c
    ${HAL_SRC_DIR}/stm32f4xx_ll_usb.c
    ${SRC_DIR}/main.c
    ${SRC_DIR}/startup_stm32f411ceux.s
    ${SRC_DIR}/stm32f4xx_hal_msp.c
    ${SRC_DIR}/stm32f4xx_it.c
    ${SRC_DIR}/syscalls.c
    ${SRC_DIR}/sysmem.c
    ${SRC_DIR}/usbd_cdc_if.c
    ${SRC_DIR}/usbd_conf.c
    ${SRC_DIR}/usbd_desc.c
    ${SRC_DIR}/usb_device.c
    ${USBDEVLIB_CDC_SRC_DIR}/usbd_cdc.c
    ${USBDEVLIB_SRC_DIR}/usbd_core.c
    ${USBDEVLIB_SRC_DIR}/usbd_ctlreq.c
    ${USBDEVLIB_SRC_DIR}/usbd_ioreq.c
)
set_target_properties(main PROPERTIES OUTPUT_NAME main.elf)
target_include_directories(main PRIVATE
    ${INC_DIR}
    ${CMSIS_INCLUDE_DIRECTORIES}
    ${HAL_INCLUDE_DIRECTORIES}
    ${USBDEVLIB_INCLUDE_DIRECTORIES}
)
target_link_options(main PRIVATE
    -Wl,-T ${LINKER_SCRIPT}
    -Wl,--fatal-warnings
    -Wl,--gc-sections
    -nostartfiles
)
target_compile_definitions(main PRIVATE
    $<$<COMPILE_LANGUAGE:CXX,C>:STM32F411xE>
)
target_compile_options(main PRIVATE
    $<$<COMPILE_LANGUAGE:CXX,C>:-O0>
    $<$<COMPILE_LANGUAGE:CXX,C>:-g>
    $<$<COMPILE_LANGUAGE:CXX,C>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX,C>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX,C>:-Wpedantic>
    $<$<COMPILE_LANGUAGE:CXX,C>:-Werror>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX,C>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:CXX,C>:-fdata-sections>
)

find_program(CMAKE_STRIP arm-none-eabi-strip REQUIRED)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/stripped.elf
    DEPENDS main
    COMMAND ${CMAKE_STRIP} main.elf -o stripped.elf
)
add_custom_target(stripped DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/stripped.elf)

find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy REQUIRED)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/main.bin
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/stripped.elf
    COMMAND ${CMAKE_OBJCOPY} -O binary stripped.elf main.bin
)
add_custom_target(main_bin ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/main.bin)

find_program(CMAKE_ST_FLASH st-flash REQUIRED)
add_custom_target(
    flash
    COMMAND ${CMAKE_ST_FLASH} --connect-under-reset write main.bin 0x08000000
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/main.bin
)
