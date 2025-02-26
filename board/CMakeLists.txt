add_library(board STATIC
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
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_cdc_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_conf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_desc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usb_device.c
    ${USBDEVLIB_CDC_SRC_DIR}/usbd_cdc.c
    ${USBDEVLIB_SRC_DIR}/usbd_core.c
    ${USBDEVLIB_SRC_DIR}/usbd_ctlreq.c
    ${USBDEVLIB_SRC_DIR}/usbd_ioreq.c
)
target_include_directories(board PRIVATE
    ${BOARD_INCLUDE_DIRECTORY}
    ${USBSHA256_INCLUDE_DIRECTORY}
    ${CMSIS_INCLUDE_DIRECTORIES}
    ${HAL_INCLUDE_DIRECTORIES}
    ${USBDEVLIB_INCLUDE_DIRECTORIES}
)
target_compile_definitions(board PRIVATE STM32F411xE)
target_compile_options(board PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -ffunction-sections
    -fdata-sections
)
