#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void shell_run();
void shell_receive_callback(unsigned char *const data, const uint32_t size);

#ifdef __cplusplus
}
#endif
