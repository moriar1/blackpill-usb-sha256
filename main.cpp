#include <wolfssl/ssl.h>

#include "board/main.h"
#include "shell.hpp"

int main() {
    board_main();
    wolfSSL_Init();
    shell_run();
}
