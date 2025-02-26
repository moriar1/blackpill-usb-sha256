find_package(wolfssl REQUIRED)

add_library(usb-sha256 STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/sha256sum.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shell.cpp
)

target_link_libraries(usb-sha256 PUBLIC wolfssl::wolfssl)
target_compile_definitions(usb-sha256 PRIVATE STM32F411xE)
target_include_directories(usb-sha256 PRIVATE
    ${USBSHA256_INCLUDE_DIRECTORY}
    ${BOARD_INCLUDE_DIRECTORY}
    ${CMSIS_INCLUDE_DIRECTORIES}
    ${HAL_INCLUDE_DIRECTORIES}
    ${USBDEVLIB_INCLUDE_DIRECTORIES}
)
target_compile_options(usb-sha256 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    -ffunction-sections
    -fdata-sections
)
