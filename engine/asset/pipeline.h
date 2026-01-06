// Asset pipeline stub: manifest definition and deterministic helpers, with hashing.
#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <optional>

#include "engine/asset/asset.h"
#include "engine/foundation/types.h"

namespace luma::asset_pipeline {

struct AssetRecord {
    AssetID id;
    AssetType type;
    std::vector<AssetID> deps;
    std::string source;  // original path (import-time only)
    std::uint64_t version{0}; // content hash
};

struct Manifest {
    std::string entry_scene;
    std::vector<AssetRecord> assets;
};

// Deterministic ordering by AssetID.
inline void sort_manifest(Manifest& manifest) {
    std::sort(manifest.assets.begin(), manifest.assets.end(),
              [](const AssetRecord& a, const AssetRecord& b) { return a.id < b.id; });
}

// Placeholder hash (use real content hash in production).
std::uint64_t compute_hash(std::string_view data);
std::uint64_t compute_file_hash(const std::filesystem::path& path);

Manifest build_demo_manifest();
Manifest ingest_gltf_manifest(const std::filesystem::path& path, const AssetID& id_hint);
void write_stub_bin(const std::filesystem::path& out_dir, const AssetRecord& rec, std::string_view payload);

// TinyGLTF (optional) helpers
struct GltfParsed {
    std::vector<AssetRecord> materials;
    std::vector<AssetRecord> textures;
    std::vector<AssetRecord> animations;
    std::optional<AssetRecord> mesh;
};

GltfParsed parse_gltf_with_tinygltf(const std::filesystem::path& path, const AssetID& id_hint);

}  // namespace luma::asset_pipeline

