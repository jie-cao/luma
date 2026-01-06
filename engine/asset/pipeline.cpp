#include "pipeline.h"

#include <regex>

namespace luma::asset_pipeline {

std::uint64_t compute_hash(std::string_view data) {
    return std::hash<std::string_view>{}(data);
}

std::uint64_t compute_file_hash(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return 0;
    std::string buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return compute_hash(buf);
}

static std::vector<std::string> extract_names(const std::string& data, const std::string& key) {
    std::vector<std::string> names;
    std::regex rgx("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
    auto begin = std::sregex_iterator(data.begin(), data.end(), rgx);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        names.push_back((*it)[1].str());
    }
    return names;
}

Manifest ingest_gltf_manifest(const std::filesystem::path& path, const AssetID& id_hint) {
    Manifest manifest;
    manifest.entry_scene = "scene_main";

    std::ifstream f(path, std::ios::binary);
    std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    const auto meshId = id_hint.empty() ? path.stem().string() : id_hint;
    AssetRecord mesh{meshId, AssetType::Mesh, {}, path.string(), compute_hash(data)};

    std::vector<AssetID> matIds;

#ifdef LUMA_WITH_TINYGLTF
    auto parsed = parse_gltf_with_tinygltf(path, meshId);
    for (const auto& m : parsed.materials) {
        manifest.assets.push_back(m);
        matIds.push_back(m.id);
    }
    for (const auto& t : parsed.textures) manifest.assets.push_back(t);
    for (const auto& a : parsed.animations) manifest.assets.push_back(a);
    if (parsed.mesh) {
        mesh = *parsed.mesh;
    }
#else
    auto matNames = extract_names(data, "name");
    for (size_t i = 0; i < matNames.size(); ++i) {
        const auto id = meshId + "_mat_" + (matNames[i].empty() ? std::to_string(i) : matNames[i]);
        manifest.assets.push_back({id, AssetType::Material, {}, path.string(), compute_hash(matNames[i])});
        matIds.push_back(id);
    }

    auto texNames = extract_names(data, "uri");
    for (size_t i = 0; i < texNames.size(); ++i) {
        const auto id = meshId + "_tex_" + (texNames[i].empty() ? std::to_string(i) : texNames[i]);
        manifest.assets.push_back({id, AssetType::Texture, {}, path.string(), compute_hash(texNames[i])});
    }

    auto animNames = extract_names(data, "animations");
    for (size_t i = 0; i < animNames.size(); ++i) {
        const auto id = meshId + "_anim_" + (animNames[i].empty() ? std::to_string(i) : animNames[i]);
        manifest.assets.push_back({id, AssetType::AnimationClip, {}, path.string(), compute_hash(animNames[i])});
    }
#endif

    mesh.deps = matIds;
    manifest.assets.push_back(mesh);
    sort_manifest(manifest);
    return manifest;
}

GltfParsed parse_gltf_with_tinygltf(const std::filesystem::path& path, const AssetID& id_hint) {
    GltfParsed parsed;
#ifdef LUMA_WITH_TINYGLTF
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    if (!loader.LoadASCIIFromFile(&model, &err, &warn, path.string())) {
        return parsed;
    }
    const auto meshId = id_hint.empty() ? path.stem().string() : id_hint;
    parsed.mesh = AssetRecord{meshId, AssetType::Mesh, {}, path.string(), compute_file_hash(path)};

    for (size_t i = 0; i < model.materials.size(); ++i) {
        const auto& mat = model.materials[i];
        const auto id = meshId + "_mat_" + (mat.name.empty() ? std::to_string(i) : mat.name);
        parsed.materials.push_back({id, AssetType::Material, {}, path.string(), compute_hash(mat.name)});
    }
    for (size_t i = 0; i < model.textures.size(); ++i) {
        const auto& tex = model.textures[i];
        const auto id = meshId + "_tex_" + std::to_string(i);
        parsed.textures.push_back({id, AssetType::Texture, {}, path.string(), compute_hash(std::to_string(i))});
    }
    for (size_t i = 0; i < model.animations.size(); ++i) {
        const auto& anim = model.animations[i];
        const auto id = meshId + "_anim_" + (anim.name.empty() ? std::to_string(i) : anim.name);
        parsed.animations.push_back({id, AssetType::AnimationClip, {}, path.string(), compute_hash(anim.name)});
    }
#endif
    return parsed;
}

void write_stub_bin(const std::filesystem::path& out_dir, const AssetRecord& rec, std::string_view payload) {
    std::filesystem::create_directories(out_dir);
    std::ofstream bin(out_dir / (rec.id + ".bin"), std::ios::binary);
    bin.write(payload.data(), static_cast<std::streamsize>(payload.size()));
}

Manifest build_demo_manifest() {
    Manifest manifest;
    manifest.entry_scene = "scene_main";
    manifest.assets.push_back({"asset_mesh_hero", AssetType::Mesh, {}, "hero.gltf"});
    manifest.assets.push_back({"asset_camera_main", AssetType::Scene, {}, "camera"});
    manifest.assets.push_back({"look_default", AssetType::Look, {}, "look.json"});
    sort_manifest(manifest);
    return manifest;
}

}  // namespace luma::asset_pipeline

