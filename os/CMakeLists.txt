set(OS_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(OS_SRC_DIR "${OS_DIR}/src")

add_library(os STATIC
    ${OS_SRC_DIR}/syscalls.c
    ${OS_SRC_DIR}/sysmem.c
)

target_compile_options(os PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -ffunction-sections
    -fdata-sections
)
