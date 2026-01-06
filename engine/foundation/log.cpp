#include "log.h"

#include <iostream>

namespace luma {

void log_info(std::string_view message) {
    std::cout << "[luma] " << message << std::endl;
}

}  // namespace luma

