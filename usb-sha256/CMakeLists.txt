set(USBSHA256_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(USBSHA256_SRC_DIR "${USBSHA256_DIR}/src")
set(USBSHA256_INC_DIR "${USBSHA256_DIR}/include/usb-sha256")

find_package(wolfssl REQUIRED)

add_library(usb-sha256 STATIC
    "${USBSHA256_SRC_DIR}/sha256sum.cpp"
    "${USBSHA256_SRC_DIR}/shell.cpp"
)

target_include_directories(usb-sha256 PUBLIC "${USBSHA256_INC_DIR}")
target_include_directories(usb-sha256 INTERFACE "${USBSHA256_DIR}/include")

target_compile_options(usb-sha256 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -ffunction-sections
    -fno-exceptions
    -fdata-sections
)

target_link_libraries(usb-sha256 PUBLIC wolfssl::wolfssl board)
