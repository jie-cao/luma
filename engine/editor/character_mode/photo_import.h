// LUMA Photo Import Module
// Multi-photo import with real image loading via stb_image.
// Supports front, left profile, and right profile views (MetaHuman-style).
#pragma once

#include "engine/character/ai/face_reconstruction.h"
#include "engine/character/character_face.h"
#include "engine/character/character.h"
#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <functional>
#include <cstdio>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

// stb_image functions (implemented in model_loader.cpp)
extern "C" {
    extern unsigned char* stbi_load(char const* filename, int* x, int* y,
                                     int* channels_in_file, int desired_channels);
    extern unsigned char* stbi_load_from_memory(unsigned char const* buffer, int len,
                                                 int* x, int* y, int* channels_in_file,
                                                 int desired_channels);
    extern void stbi_image_free(void* retval_from_stbi_load);
}

namespace luma {
namespace editor {

// ============================================================================
// Photo Slot (one per view: front, left, right)
// ============================================================================

struct PhotoSlot {
    enum ViewType { Front = 0, LeftProfile = 1, RightProfile = 2 };

    std::string filePath;
    ViewType viewType = Front;

    // Loaded image data (real pixels via stb_image)
    std::vector<uint8_t> imageData;
    int width = 0;
    int height = 0;
    int channels = 0;

    // Processing state
    bool loaded = false;
    bool processed = false;
    float processingProgress = 0.0f;

    // Results from processing this photo
    FaceDetection detection;
    FaceLandmarks landmarks;
    Face3DMMResult coefficients;
    Vec3 skinColor;
    float confidence = 0.0f;

    // Thumbnail for UI display (64x64 RGBA)
    std::vector<uint8_t> thumbnail;
    int thumbWidth = 0;
    int thumbHeight = 0;

    bool isEmpty() const { return filePath.empty(); }

    void clear() {
        filePath.clear();
        imageData.clear();
        width = height = channels = 0;
        loaded = processed = false;
        processingProgress = 0.0f;
        confidence = 0.0f;
        thumbnail.clear();
        thumbWidth = thumbHeight = 0;
    }
};

// ============================================================================
// Photo Import Result
// ============================================================================

struct PhotoImportResult {
    bool success = false;
    std::string errorMessage;
    std::string summary;

    // Final face parameters (fused from all views)
    FaceShapeParams faceParams;
    Vec3 skinColor;

    // Detected landmarks (for direct mesh deformation)
    FaceLandmarks landmarks;

    // Texture extracted from front photo
    std::vector<uint8_t> textureData;
    int textureWidth = 0;
    int textureHeight = 0;

    // Confidence score
    float confidence = 0.0f;

    // Legacy compatibility
    PhotoFaceResult toLegacyResult() const {
        PhotoFaceResult r;
        r.success = success;
        r.errorMessage = errorMessage;
        r.overallConfidence = confidence;
        r.textureData = textureData;
        r.textureWidth = textureWidth;
        r.textureHeight = textureHeight;
        return r;
    }
};

// ============================================================================
// Photo Import Module
// ============================================================================

class PhotoImportModule {
public:
    PhotoImportModule() = default;

    // === File Dialog ===

    static std::string openPhotoFileDialog(void* hwnd = nullptr) {
#ifdef _WIN32
        char path[MAX_PATH] = {};
        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = static_cast<HWND>(hwnd);
        ofn.lpstrFilter =
            "Image Files\0*.jpg;*.jpeg;*.png;*.bmp;*.tga\0"
            "JPEG Files (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0"
            "PNG Files (*.png)\0*.png\0"
            "All Files (*.*)\0*.*\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        ofn.lpstrTitle = "Select Photo for Face Generation";

        if (GetOpenFileNameA(&ofn)) {
            return std::string(path);
        }
#else
        (void)hwnd;
#endif
        return "";
    }

    // === Real Image Loading via stb_image ===

    static bool loadImage(const std::string& path,
                          std::vector<uint8_t>& outData,
                          int& outWidth, int& outHeight, int& outChannels) {

        // Decode image using stb_image (supports JPEG, PNG, BMP, TGA, etc.)
        int w = 0, h = 0, ch = 0;
        unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &ch, 3);

        if (pixels && w > 0 && h > 0) {
            outWidth = w;
            outHeight = h;
            outChannels = 3; // We requested 3 channels (RGB)
            outData.assign(pixels, pixels + w * h * 3);
            stbi_image_free(pixels);

            printf("[PhotoImport] Loaded image (%dx%d, %dch): %s\n",
                   outWidth, outHeight, ch, path.c_str());
            return true;
        }

        if (pixels) stbi_image_free(pixels);

        // Fallback: try loading from memory buffer for paths stbi_load can't handle
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) {
            printf("[PhotoImport] Cannot open file: %s\n", path.c_str());
            return false;
        }
        fseek(f, 0, SEEK_END);
        long fileSize = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (fileSize <= 0 || fileSize > 100 * 1024 * 1024) {
            fclose(f);
            printf("[PhotoImport] File too large or empty: %ld bytes\n", fileSize);
            return false;
        }

        std::vector<uint8_t> fileData(fileSize);
        size_t bytesRead = fread(fileData.data(), 1, fileSize, f);
        fclose(f);
        if (bytesRead != (size_t)fileSize) return false;

        pixels = stbi_load_from_memory(fileData.data(), (int)fileSize, &w, &h, &ch, 3);
        if (pixels && w > 0 && h > 0) {
            outWidth = w;
            outHeight = h;
            outChannels = 3;
            outData.assign(pixels, pixels + w * h * 3);
            stbi_image_free(pixels);

            printf("[PhotoImport] Loaded image from memory (%dx%d): %s\n",
                   outWidth, outHeight, path.c_str());
            return true;
        }

        if (pixels) stbi_image_free(pixels);
        printf("[PhotoImport] Failed to decode image: %s\n", path.c_str());
        return false;
    }

    // === Multi-Photo Support ===

    static bool loadPhotoSlot(PhotoSlot& slot) {
        slot.loaded = false;
        slot.processed = false;

        if (slot.filePath.empty()) return false;

        bool ok = loadImage(slot.filePath, slot.imageData,
                           slot.width, slot.height, slot.channels);
        if (ok) {
            slot.loaded = true;
            generateThumbnail(slot);
        }
        return ok;
    }

    // Process a single photo slot with the pipeline
    static bool processPhotoSlot(PhotoSlot& slot, PhotoToFacePipeline& pipeline) {
        if (!slot.loaded) return false;

        slot.processingProgress = 0.1f;

        PhotoFacePipelineResult result = pipeline.process(
            slot.imageData.data(), slot.width, slot.height, slot.channels);

        slot.processingProgress = 0.8f;

        if (result.success) {
            slot.detection = result.detection;
            slot.landmarks = result.landmarks;
            slot.coefficients = result.coefficients;
            slot.skinColor = result.skinColor;
            slot.confidence = result.confidence;
            slot.processed = true;
        }

        slot.processingProgress = 1.0f;
        return result.success;
    }

    // Process all photo slots and fuse results
    static PhotoImportResult processAllPhotos(
        PhotoSlot* slots, int numSlots, PhotoToFacePipeline& pipeline) {

        PhotoImportResult result;

        // Process each slot individually
        int processedCount = 0;
        for (int i = 0; i < numSlots; i++) {
            if (!slots[i].loaded) continue;
            if (processPhotoSlot(slots[i], pipeline)) {
                processedCount++;
            }
        }

        if (processedCount == 0) {
            result.errorMessage = "No photos could be processed";
            return result;
        }

        // Build multi-view fusion
        std::vector<MultiViewFusion::ViewResult> views;
        for (int i = 0; i < numSlots; i++) {
            if (!slots[i].processed) continue;

            MultiViewFusion::ViewResult view;
            view.coefficients = slots[i].coefficients;
            view.landmarks = slots[i].landmarks;
            view.confidence = slots[i].confidence;

            switch (slots[i].viewType) {
                case PhotoSlot::Front:        view.viewType = MultiViewFusion::ViewResult::Front; break;
                case PhotoSlot::LeftProfile:  view.viewType = MultiViewFusion::ViewResult::LeftProfile; break;
                case PhotoSlot::RightProfile: view.viewType = MultiViewFusion::ViewResult::RightProfile; break;
            }
            views.push_back(view);
        }

        // Fuse all views
        MultiViewFusion::fuse(views, result.faceParams);

        // Use front photo for skin color, texture, and landmarks
        for (int i = 0; i < numSlots; i++) {
            if (slots[i].viewType == PhotoSlot::Front && slots[i].processed) {
                result.skinColor = slots[i].skinColor;
                
                // Store landmarks for direct mesh deformation
                result.landmarks = slots[i].landmarks;

                // Extract texture from front photo
                if (slots[i].loaded && slots[i].landmarks.numPoints >= 68) {
                    FaceTextureExtractor::extractTexture(
                        slots[i].imageData.data(), slots[i].width, slots[i].height, slots[i].channels,
                        slots[i].landmarks, result.textureData, 512);
                    result.textureWidth = 512;
                    result.textureHeight = 512;
                }
                break;
            }
        }

        result.confidence = views.empty() ? 0.0f : views[0].confidence;
        result.success = true;

        char buf[256];
        snprintf(buf, sizeof(buf), "Processed %d photos, confidence: %.0f%%",
                 processedCount, result.confidence * 100.0f);
        result.summary = buf;

        printf("[PhotoImport] %s\n", result.summary.c_str());
        return result;
    }

    // === Legacy compatibility ===

    static uint32_t computeImageHash(const std::string& path) {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) return 42;
        uint32_t hash = 5381;
        uint8_t buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
            for (size_t i = 0; i < n; i++) {
                hash = ((hash << 5) + hash) + buf[i];
            }
        }
        fclose(f);
        return hash;
    }

    static Vec3 extractDominantColor(const std::string& path) {
        // Load real pixels via stb_image for accurate color extraction
        int w = 0, h = 0, ch = 0;
        unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &ch, 3);
        if (!pixels || w <= 0 || h <= 0) {
            if (pixels) stbi_image_free(pixels);
            return Vec3(0.85f, 0.65f, 0.5f);
        }

        // Sample from the center region (more likely to contain face/skin)
        float rSum = 0, gSum = 0, bSum = 0;
        int count = 0;
        int x0 = w / 4, x1 = w * 3 / 4;
        int y0 = h / 4, y1 = h * 3 / 4;
        int step = std::max(1, (x1 - x0) * (y1 - y0) / 2000);

        for (int i = 0; i < (x1 - x0) * (y1 - y0); i += step) {
            int lx = x0 + (i % (x1 - x0));
            int ly = y0 + (i / (x1 - x0));
            if (ly >= y1) break;
            int idx = (ly * w + lx) * 3;
            uint8_t r = pixels[idx], g = pixels[idx+1], b = pixels[idx+2];

            // Skin-like color filter
            if (r > 50 && g > 30 && b > 20 && r < 250 && g < 240) {
                rSum += r; gSum += g; bSum += b; count++;
            }
        }
        stbi_image_free(pixels);

        if (count < 10) return Vec3(0.85f, 0.65f, 0.5f);
        return Vec3(
            std::clamp(rSum / count / 255.0f, 0.3f, 0.95f),
            std::clamp(gSum / count / 255.0f, 0.2f, 0.85f),
            std::clamp(bSum / count / 255.0f, 0.15f, 0.75f)
        );
    }

private:
    // Generate thumbnail for UI display
    static void generateThumbnail(PhotoSlot& slot) {
        int tw = 64, th = 64;
        slot.thumbnail.resize(tw * th * 4);
        slot.thumbWidth = tw;
        slot.thumbHeight = th;

        float scaleX = (float)slot.width / tw;
        float scaleY = (float)slot.height / th;

        for (int y = 0; y < th; y++) {
            for (int x = 0; x < tw; x++) {
                int srcX = std::min((int)(x * scaleX), slot.width - 1);
                int srcY = std::min((int)(y * scaleY), slot.height - 1);
                int srcIdx = (srcY * slot.width + srcX) * slot.channels;
                int dstIdx = (y * tw + x) * 4;

                slot.thumbnail[dstIdx + 0] = slot.imageData[srcIdx + 0];
                slot.thumbnail[dstIdx + 1] = (slot.channels > 1) ? slot.imageData[srcIdx + 1] : slot.imageData[srcIdx];
                slot.thumbnail[dstIdx + 2] = (slot.channels > 2) ? slot.imageData[srcIdx + 2] : slot.imageData[srcIdx];
                slot.thumbnail[dstIdx + 3] = 255;
            }
        }
    }

};

} // namespace editor
} // namespace luma
