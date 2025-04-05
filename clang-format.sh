#!/bin/env bash

clang-format -i \
    usb-sha256/src/*.cpp \
    usb-sha256/include/usb-sha256/*.hpp \
    board/include/board/*.h \
    board/src/*.c
