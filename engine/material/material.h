// Simple material store with parameter/value and variant index.
#pragma once

#include <string>
#include <unordered_map>

namespace luma {

struct MaterialData {
    std::unordered_map<std::string, std::string> parameters;
    int variant{0};
};

}  // namespace luma

