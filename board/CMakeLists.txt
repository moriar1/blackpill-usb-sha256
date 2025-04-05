set(BOARD_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BOARD_SRC_DIR ${BOARD_DIR}/src)
set(BOARD_INC_DIR ${BOARD_DIR}/include/board)

include(${BOARD_DIR}/stm32cubef4.cmake)

add_library(board STATIC
    ${BOARD_SRC_DIR}/main.c
    ${BOARD_SRC_DIR}/stm32f4xx_hal_msp.c
    ${BOARD_SRC_DIR}/stm32f4xx_it.c
    ${BOARD_SRC_DIR}/usb_device.c
    ${BOARD_SRC_DIR}/usbd_cdc_if.c
    ${BOARD_SRC_DIR}/usbd_conf.c
    ${BOARD_SRC_DIR}/usbd_desc.c
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
    ${USBDEVLIB_CDC_SRC_DIR}/usbd_cdc.c
    ${USBDEVLIB_SRC_DIR}/usbd_core.c
    ${USBDEVLIB_SRC_DIR}/usbd_ctlreq.c
    ${USBDEVLIB_SRC_DIR}/usbd_ioreq.c
)

target_include_directories(board PUBLIC
    ${BOARD_INC_DIR}
    ${CMSIS_INCLUDE_DIRECTORIES}
    ${HAL_INCLUDE_DIRECTORIES}
    ${USBDEVLIB_INCLUDE_DIRECTORIES}
)
target_include_directories(board INTERFACE "${BOARD_DIR}/include")

target_compile_definitions(board PUBLIC STM32F411xE)
target_compile_options(board PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -ffunction-sections
    -fdata-sections
)

add_subdirectory(${BOARD_DIR}/startup)
target_link_libraries(board INTERFACE startup)
