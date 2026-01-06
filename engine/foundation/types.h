// Basic shared type definitions for LUMA v1 skeleton.
#pragma once

#include <cstdint>
#include <string>

namespace luma {

// Asset identifiers are stable strings (UUID or content hash).
using AssetID = std::string;

// Action identifiers can be used for tracing/playback if needed.
using ActionID = std::uint64_t;

}  // namespace luma

