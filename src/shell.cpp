#include <string>
#include <string_view>
#include <cstddef>

#include "usbd_cdc_if.h"
#include "shell.h"

namespace shell {

std::string current_input {};

void respond(const std::string_view response) {
    std::string copy { response };
    auto ptr = reinterpret_cast<unsigned char*>(copy.data());
    CDC_Transmit_FS(ptr, response.size());
}

void process_characters(const char* const data, const std::size_t size) {
    (void)data;
    if (size > 500) {
    }
    if (current_input.size() + size > 500) {
        current_input.clear();
        respond("\r\nerror: input is larger than 500 symbols\r\n");
    }
    current_input.append(data, size);
}

}

extern "C" void process_characters(const uint8_t* const data, const uint32_t size) {
    shell::process_characters(reinterpret_cast<const char*>(data), size);
}
