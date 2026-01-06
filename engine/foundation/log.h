// Minimal logging facade placeholder; replace with spdlog in full build.
#pragma once

#include <string_view>

namespace luma {

void log_info(std::string_view message);

}  // namespace luma

