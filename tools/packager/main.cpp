#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "engine/asset/pipeline.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    const fs::path outDir = (argc > 1) ? fs::path(argv[1]) : fs::path("package");
    const fs::path gltfPath = (argc > 2) ? fs::path(argv[2]) : fs::path();
    fs::create_directories(outDir / "assets");

    luma::asset_pipeline::Manifest manifest;
    if (!gltfPath.empty() && fs::exists(gltfPath)) {
        manifest = luma::asset_pipeline::ingest_gltf_manifest(gltfPath, "imported_mesh");
    } else {
        manifest = luma::asset_pipeline::build_demo_manifest();
    }
    luma::asset_pipeline::sort_manifest(manifest);

    std::ofstream manifestFile(outDir / "manifest.json", std::ios::binary);
    manifestFile << "{\n  \"entry_scene\": \"" << manifest.entry_scene << "\",\n  \"assets\": [\n";
    for (size_t i = 0; i < manifest.assets.size(); ++i) {
        const auto& a = manifest.assets[i];
        manifestFile << "    {\"id\": \"" << a.id << "\", \"type\": " << static_cast<int>(a.type)
                     << ", \"deps\": []}";
        manifestFile << (i + 1 < manifest.assets.size() ? ",\n" : "\n");
    }
    manifestFile << "  ]\n}\n";

    // Write stub asset bins to ensure deterministic presence.
    for (const auto& a : manifest.assets) {
        if (!gltfPath.empty() && fs::exists(gltfPath) && a.id == "imported_mesh") {
            // Copy source glTF content as payload to keep deterministic hash.
            std::ifstream src(gltfPath, std::ios::binary);
            std::string data((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
            luma::asset_pipeline::write_stub_bin(outDir / "assets", a, data);
        } else {
            const std::string payload = "stub_" + a.id;
            luma::asset_pipeline::write_stub_bin(outDir / "assets", a, payload);
        }
    }

    std::cout << "Packaged stub assets to " << outDir << "\n";
    return 0;
}

