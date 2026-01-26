// Screenshot and Export System
// Supports PNG/JPG capture with custom resolution and transparency
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace luma {

// ===== Screenshot Settings =====
struct ScreenshotSettings {
    // Output format
    enum class Format { PNG, JPG };
    Format format = Format::PNG;
    
    // Resolution
    uint32_t width = 0;   // 0 = use viewport size
    uint32_t height = 0;
    bool maintainAspectRatio = true;
    
    // Quality
    int jpgQuality = 95;  // 1-100 for JPG
    
    // Options
    bool transparentBackground = false;
    bool includeUI = false;
    bool antialiasing = true;
    int supersampling = 1;  // 1, 2, or 4 for SSAA
    
    // File naming
    std::string outputPath;
    bool autoIncrement = true;  // Add _001, _002, etc.
    
    // Preset resolutions
    static ScreenshotSettings HD() {
        ScreenshotSettings s;
        s.width = 1280;
        s.height = 720;
        return s;
    }
    
    static ScreenshotSettings FullHD() {
        ScreenshotSettings s;
        s.width = 1920;
        s.height = 1080;
        return s;
    }
    
    static ScreenshotSettings UHD4K() {
        ScreenshotSettings s;
        s.width = 3840;
        s.height = 2160;
        return s;
    }
    
    static ScreenshotSettings Square1K() {
        ScreenshotSettings s;
        s.width = 1024;
        s.height = 1024;
        return s;
    }
    
    static ScreenshotSettings Square2K() {
        ScreenshotSettings s;
        s.width = 2048;
        s.height = 2048;
        return s;
    }
};

// ===== Animation Export Settings =====
struct AnimationExportSettings {
    // Frame range
    float startTime = 0.0f;
    float endTime = 0.0f;
    float fps = 30.0f;
    
    // Output
    std::string outputDirectory;
    std::string filenamePrefix = "frame_";
    ScreenshotSettings frameSettings;
    
    // Video encoding (optional)
    bool encodeVideo = false;
    std::string videoCodec = "h264";
    int videoBitrate = 8000000;  // 8 Mbps
};

// ===== Screenshot Exporter =====
class ScreenshotExporter {
public:
    // Callback types
    using PixelReadCallback = std::function<bool(uint32_t width, uint32_t height, 
                                                  std::vector<uint8_t>& outPixels)>;
    using ProgressCallback = std::function<void(int current, int total)>;
    using CompletionCallback = std::function<void(bool success, const std::string& path)>;
    
    // Set pixel read callback (platform-specific implementation)
    void setPixelReader(PixelReadCallback callback) { pixelReader_ = callback; }
    
    // Take a single screenshot
    bool captureScreenshot(const ScreenshotSettings& settings);
    
    // Export animation sequence
    bool exportAnimationSequence(const AnimationExportSettings& settings,
                                  ProgressCallback progressCallback = nullptr);
    
    // Get last error message
    const std::string& getLastError() const { return lastError_; }
    
    // Get generated file path from last capture
    const std::string& getLastFilePath() const { return lastFilePath_; }
    
    // Generate unique filename
    static std::string generateFilename(const std::string& basePath, 
                                         const std::string& extension,
                                         bool autoIncrement);
    
private:
    bool saveAsPNG(const std::string& path, uint32_t width, uint32_t height,
                    const std::vector<uint8_t>& pixels, bool hasAlpha);
    bool saveAsJPG(const std::string& path, uint32_t width, uint32_t height,
                    const std::vector<uint8_t>& pixels, int quality);
    
    PixelReadCallback pixelReader_;
    std::string lastError_;
    std::string lastFilePath_;
};

// ===== Implementation =====

inline std::string ScreenshotExporter::generateFilename(const std::string& basePath,
                                                          const std::string& extension,
                                                          bool autoIncrement) {
    if (!autoIncrement) {
        return basePath + "." + extension;
    }
    
    // Try to find a unique filename
    for (int i = 1; i < 10000; ++i) {
        char suffix[16];
        snprintf(suffix, sizeof(suffix), "_%04d", i);
        std::string path = basePath + suffix + "." + extension;
        
        // Check if file exists (simple check using fopen)
        FILE* f = fopen(path.c_str(), "r");
        if (!f) {
            return path;
        }
        fclose(f);
    }
    
    return basePath + "_9999." + extension;
}

inline bool ScreenshotExporter::captureScreenshot(const ScreenshotSettings& settings) {
    if (!pixelReader_) {
        lastError_ = "No pixel reader callback set";
        return false;
    }
    
    // Determine actual size
    uint32_t width = settings.width;
    uint32_t height = settings.height;
    
    // Read pixels from render target
    std::vector<uint8_t> pixels;
    if (!pixelReader_(width, height, pixels)) {
        lastError_ = "Failed to read pixels from render target";
        return false;
    }
    
    if (width == 0 || height == 0) {
        lastError_ = "Invalid resolution";
        return false;
    }
    
    // Determine output path
    std::string extension = (settings.format == ScreenshotSettings::Format::PNG) ? "png" : "jpg";
    std::string outputPath = settings.outputPath;
    
    if (outputPath.empty()) {
        outputPath = "screenshot";
    }
    
    // Remove extension if present
    size_t extPos = outputPath.rfind('.');
    if (extPos != std::string::npos) {
        std::string ext = outputPath.substr(extPos + 1);
        if (ext == "png" || ext == "jpg" || ext == "jpeg") {
            outputPath = outputPath.substr(0, extPos);
        }
    }
    
    lastFilePath_ = generateFilename(outputPath, extension, settings.autoIncrement);
    
    // Save the image
    bool success = false;
    if (settings.format == ScreenshotSettings::Format::PNG) {
        success = saveAsPNG(lastFilePath_, width, height, pixels, settings.transparentBackground);
    } else {
        success = saveAsJPG(lastFilePath_, width, height, pixels, settings.jpgQuality);
    }
    
    return success;
}

inline bool ScreenshotExporter::saveAsPNG(const std::string& path, uint32_t width, uint32_t height,
                                           const std::vector<uint8_t>& pixels, bool hasAlpha) {
    // Use stb_image_write for PNG encoding
    // Note: This requires stb_image_write to be included in the project
    
    #ifdef STB_IMAGE_WRITE_IMPLEMENTATION
    int channels = hasAlpha ? 4 : 3;
    int result = stbi_write_png(path.c_str(), width, height, channels, pixels.data(), width * channels);
    if (result == 0) {
        lastError_ = "Failed to write PNG file";
        return false;
    }
    return true;
    #else
    // Fallback: Write raw PPM format (very basic)
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) {
        lastError_ = "Failed to open file for writing: " + path;
        return false;
    }
    
    // PPM header
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    
    // Write RGB data (skip alpha if present)
    for (size_t i = 0; i < width * height * 4; i += 4) {
        fputc(pixels[i], f);
        fputc(pixels[i + 1], f);
        fputc(pixels[i + 2], f);
    }
    
    fclose(f);
    
    // Note: This is actually a .ppm file, not PNG
    lastError_ = "STB_IMAGE_WRITE not available, saved as PPM format";
    return true;
    #endif
}

inline bool ScreenshotExporter::saveAsJPG(const std::string& path, uint32_t width, uint32_t height,
                                           const std::vector<uint8_t>& pixels, int quality) {
    #ifdef STB_IMAGE_WRITE_IMPLEMENTATION
    // Convert RGBA to RGB
    std::vector<uint8_t> rgb(width * height * 3);
    for (size_t i = 0, j = 0; i < pixels.size(); i += 4, j += 3) {
        rgb[j] = pixels[i];
        rgb[j + 1] = pixels[i + 1];
        rgb[j + 2] = pixels[i + 2];
    }
    
    int result = stbi_write_jpg(path.c_str(), width, height, 3, rgb.data(), quality);
    if (result == 0) {
        lastError_ = "Failed to write JPG file";
        return false;
    }
    return true;
    #else
    lastError_ = "STB_IMAGE_WRITE not available for JPG export";
    return false;
    #endif
}

inline bool ScreenshotExporter::exportAnimationSequence(const AnimationExportSettings& settings,
                                                          ProgressCallback progressCallback) {
    if (!pixelReader_) {
        lastError_ = "No pixel reader callback set";
        return false;
    }
    
    if (settings.endTime <= settings.startTime) {
        lastError_ = "Invalid time range";
        return false;
    }
    
    float duration = settings.endTime - settings.startTime;
    int totalFrames = static_cast<int>(duration * settings.fps);
    
    for (int frame = 0; frame < totalFrames; ++frame) {
        // Report progress
        if (progressCallback) {
            progressCallback(frame + 1, totalFrames);
        }
        
        // Generate filename
        char frameSuffix[32];
        snprintf(frameSuffix, sizeof(frameSuffix), "%s%05d", 
                 settings.filenamePrefix.c_str(), frame);
        
        ScreenshotSettings frameSettings = settings.frameSettings;
        frameSettings.outputPath = settings.outputDirectory + "/" + frameSuffix;
        frameSettings.autoIncrement = false;
        
        if (!captureScreenshot(frameSettings)) {
            return false;
        }
    }
    
    return true;
}

// ===== Global accessor =====
inline ScreenshotExporter& getScreenshotExporter() {
    static ScreenshotExporter exporter;
    return exporter;
}

}  // namespace luma
