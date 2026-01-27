// Video Export System
// Frame capture, encoding, and video file output
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <cstdint>
#include <fstream>

namespace luma {

// ===== Video Format =====
enum class VideoFormat {
    MP4_H264,
    MP4_H265,
    WebM_VP9,
    AVI_MJPEG,
    GIF,
    ImageSequence_PNG,
    ImageSequence_JPG,
    ImageSequence_TGA
};

// ===== Video Quality =====
enum class VideoQuality {
    Low,        // Fast encoding, smaller files
    Medium,     // Balanced
    High,       // Better quality
    Lossless    // Maximum quality
};

// ===== Frame Data =====
struct FrameData {
    std::vector<uint8_t> pixels;   // RGBA or RGB
    int width = 0;
    int height = 0;
    int channels = 4;              // 3 = RGB, 4 = RGBA
    uint64_t frameNumber = 0;
    double timestamp = 0.0;        // In seconds
    
    size_t getSize() const { return width * height * channels; }
    
    // Convert RGBA to RGB (for encoding)
    void convertToRGB() {
        if (channels != 4) return;
        
        std::vector<uint8_t> rgb;
        rgb.reserve(width * height * 3);
        
        for (int i = 0; i < width * height; i++) {
            rgb.push_back(pixels[i * 4 + 0]);
            rgb.push_back(pixels[i * 4 + 1]);
            rgb.push_back(pixels[i * 4 + 2]);
        }
        
        pixels = std::move(rgb);
        channels = 3;
    }
    
    // Flip vertically (OpenGL has origin at bottom-left)
    void flipVertical() {
        int rowSize = width * channels;
        std::vector<uint8_t> temp(rowSize);
        
        for (int y = 0; y < height / 2; y++) {
            int topRow = y * rowSize;
            int bottomRow = (height - 1 - y) * rowSize;
            
            memcpy(temp.data(), &pixels[topRow], rowSize);
            memcpy(&pixels[topRow], &pixels[bottomRow], rowSize);
            memcpy(&pixels[bottomRow], temp.data(), rowSize);
        }
    }
};

// ===== Video Export Settings =====
struct VideoExportSettings {
    // Output
    std::string outputPath = "output.mp4";
    VideoFormat format = VideoFormat::MP4_H264;
    VideoQuality quality = VideoQuality::High;
    
    // Resolution
    int width = 1920;
    int height = 1080;
    bool matchViewport = true;     // Use current viewport size
    
    // Frame rate
    int frameRate = 30;
    bool captureEveryFrame = true; // Capture at fixed timestep
    
    // Duration
    float startTime = 0.0f;
    float endTime = 10.0f;         // In seconds
    bool useSceneDuration = false;
    
    // Encoding
    int bitrate = 8000000;         // 8 Mbps
    int keyframeInterval = 30;     // I-frame every N frames
    
    // Audio (future)
    bool includeAudio = false;
    int audioSampleRate = 44100;
    int audioBitrate = 192000;
    
    // Advanced
    bool multiThreaded = true;
    int encoderThreads = 4;
    bool showProgress = true;
    
    int getTotalFrames() const {
        return (int)((endTime - startTime) * frameRate);
    }
    
    double getFrameDuration() const {
        return 1.0 / frameRate;
    }
};

// ===== Frame Capture Interface =====
class IFrameCapture {
public:
    virtual ~IFrameCapture() = default;
    
    // Capture current frame
    virtual bool capture(FrameData& outFrame) = 0;
    
    // Get capture resolution
    virtual void getResolution(int& width, int& height) = 0;
};

// ===== Video Encoder Interface =====
class IVideoEncoder {
public:
    virtual ~IVideoEncoder() = default;
    
    // Initialize encoder
    virtual bool initialize(const VideoExportSettings& settings) = 0;
    
    // Encode a frame
    virtual bool encodeFrame(const FrameData& frame) = 0;
    
    // Finalize and close file
    virtual bool finalize() = 0;
    
    // Get progress (0.0 - 1.0)
    virtual float getProgress() const = 0;
    
    // Get error message
    virtual std::string getError() const = 0;
};

// ===== Image Sequence Encoder =====
// Simple encoder that outputs individual images
class ImageSequenceEncoder : public IVideoEncoder {
public:
    bool initialize(const VideoExportSettings& settings) override {
        settings_ = settings;
        frameCount_ = 0;
        
        // Determine extension
        switch (settings.format) {
            case VideoFormat::ImageSequence_PNG: extension_ = ".png"; break;
            case VideoFormat::ImageSequence_JPG: extension_ = ".jpg"; break;
            case VideoFormat::ImageSequence_TGA: extension_ = ".tga"; break;
            default: extension_ = ".png"; break;
        }
        
        // Create output directory path
        basePath_ = settings.outputPath;
        if (basePath_.find('.') != std::string::npos) {
            basePath_ = basePath_.substr(0, basePath_.rfind('.'));
        }
        
        return true;
    }
    
    bool encodeFrame(const FrameData& frame) override {
        char filename[256];
        snprintf(filename, sizeof(filename), "%s_%05d%s", 
                basePath_.c_str(), (int)frameCount_, extension_.c_str());
        
        bool success = false;
        
        if (extension_ == ".tga") {
            success = writeTGA(filename, frame);
        } else if (extension_ == ".png") {
            success = writePNG(filename, frame);
        } else {
            // JPG - simplified placeholder
            success = writeTGA(filename, frame);  // Fallback to TGA
        }
        
        if (success) {
            frameCount_++;
        }
        
        return success;
    }
    
    bool finalize() override {
        return true;
    }
    
    float getProgress() const override {
        if (settings_.getTotalFrames() == 0) return 1.0f;
        return (float)frameCount_ / settings_.getTotalFrames();
    }
    
    std::string getError() const override { return error_; }
    
private:
    bool writeTGA(const char* filename, const FrameData& frame) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            error_ = "Failed to open file: " + std::string(filename);
            return false;
        }
        
        // TGA header
        uint8_t header[18] = {0};
        header[2] = 2;  // Uncompressed RGB
        header[12] = frame.width & 0xFF;
        header[13] = (frame.width >> 8) & 0xFF;
        header[14] = frame.height & 0xFF;
        header[15] = (frame.height >> 8) & 0xFF;
        header[16] = frame.channels * 8;  // Bits per pixel
        header[17] = (frame.channels == 4) ? 8 : 0;  // Alpha bits
        
        file.write((char*)header, 18);
        
        // Write pixel data (TGA is BGR/BGRA)
        for (int y = 0; y < frame.height; y++) {
            for (int x = 0; x < frame.width; x++) {
                int idx = (y * frame.width + x) * frame.channels;
                
                // Swap R and B
                uint8_t b = frame.pixels[idx + 0];
                uint8_t g = frame.pixels[idx + 1];
                uint8_t r = frame.pixels[idx + 2];
                
                file.write((char*)&r, 1);
                file.write((char*)&g, 1);
                file.write((char*)&b, 1);
                
                if (frame.channels == 4) {
                    uint8_t a = frame.pixels[idx + 3];
                    file.write((char*)&a, 1);
                }
            }
        }
        
        return true;
    }
    
    bool writePNG(const char* filename, const FrameData& frame) {
        // Simplified PNG writing - in production would use libpng or stb_image_write
        // For now, fall back to TGA
        std::string tgaFilename = std::string(filename);
        size_t pos = tgaFilename.rfind(".png");
        if (pos != std::string::npos) {
            tgaFilename.replace(pos, 4, ".tga");
        }
        return writeTGA(tgaFilename.c_str(), frame);
    }
    
    VideoExportSettings settings_;
    std::string basePath_;
    std::string extension_;
    uint64_t frameCount_ = 0;
    std::string error_;
};

// ===== FFmpeg Pipe Encoder =====
// Pipes frames to FFmpeg for encoding
class FFmpegEncoder : public IVideoEncoder {
public:
    bool initialize(const VideoExportSettings& settings) override {
        settings_ = settings;
        frameCount_ = 0;
        
        // Build FFmpeg command
        std::string codecArgs;
        std::string formatArgs;
        
        switch (settings.format) {
            case VideoFormat::MP4_H264:
                codecArgs = "-c:v libx264";
                formatArgs = "-f mp4";
                break;
            case VideoFormat::MP4_H265:
                codecArgs = "-c:v libx265";
                formatArgs = "-f mp4";
                break;
            case VideoFormat::WebM_VP9:
                codecArgs = "-c:v libvpx-vp9";
                formatArgs = "-f webm";
                break;
            case VideoFormat::AVI_MJPEG:
                codecArgs = "-c:v mjpeg";
                formatArgs = "-f avi";
                break;
            default:
                codecArgs = "-c:v libx264";
                formatArgs = "-f mp4";
                break;
        }
        
        // Quality preset
        std::string qualityArgs;
        switch (settings.quality) {
            case VideoQuality::Low:
                qualityArgs = "-preset ultrafast -crf 28";
                break;
            case VideoQuality::Medium:
                qualityArgs = "-preset medium -crf 23";
                break;
            case VideoQuality::High:
                qualityArgs = "-preset slow -crf 18";
                break;
            case VideoQuality::Lossless:
                qualityArgs = "-preset veryslow -crf 0";
                break;
        }
        
        // Build full command
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
            "ffmpeg -y -f rawvideo -pix_fmt rgb24 -s %dx%d -r %d -i - "
            "%s %s -b:v %d %s \"%s\" 2>/dev/null",
            settings.width, settings.height, settings.frameRate,
            codecArgs.c_str(), qualityArgs.c_str(), settings.bitrate,
            formatArgs.c_str(), settings.outputPath.c_str()
        );
        
        command_ = cmd;
        
        // Open pipe to FFmpeg
#ifdef _WIN32
        pipe_ = _popen(command_.c_str(), "wb");
#else
        pipe_ = popen(command_.c_str(), "w");
#endif
        
        if (!pipe_) {
            error_ = "Failed to open FFmpeg pipe. Is FFmpeg installed?";
            return false;
        }
        
        initialized_ = true;
        return true;
    }
    
    bool encodeFrame(const FrameData& frame) override {
        if (!pipe_) return false;
        
        // FFmpeg expects RGB, no alpha
        const uint8_t* data = frame.pixels.data();
        size_t dataSize = frame.width * frame.height * 3;
        
        // If RGBA, need to convert
        std::vector<uint8_t> rgbData;
        if (frame.channels == 4) {
            rgbData.resize(frame.width * frame.height * 3);
            for (int i = 0; i < frame.width * frame.height; i++) {
                rgbData[i * 3 + 0] = frame.pixels[i * 4 + 0];
                rgbData[i * 3 + 1] = frame.pixels[i * 4 + 1];
                rgbData[i * 3 + 2] = frame.pixels[i * 4 + 2];
            }
            data = rgbData.data();
        }
        
        size_t written = fwrite(data, 1, dataSize, pipe_);
        if (written != dataSize) {
            error_ = "Failed to write frame data to FFmpeg";
            return false;
        }
        
        frameCount_++;
        return true;
    }
    
    bool finalize() override {
        if (pipe_) {
#ifdef _WIN32
            _pclose(pipe_);
#else
            pclose(pipe_);
#endif
            pipe_ = nullptr;
        }
        return true;
    }
    
    float getProgress() const override {
        if (settings_.getTotalFrames() == 0) return 1.0f;
        return (float)frameCount_ / settings_.getTotalFrames();
    }
    
    std::string getError() const override { return error_; }
    
    bool isInitialized() const { return initialized_; }
    
private:
    VideoExportSettings settings_;
    std::string command_;
    FILE* pipe_ = nullptr;
    uint64_t frameCount_ = 0;
    std::string error_;
    bool initialized_ = false;
};

// ===== Recording State =====
enum class RecordingState {
    Idle,
    Preparing,
    Recording,
    Paused,
    Finalizing,
    Complete,
    Error
};

// ===== Recording Manager =====
class RecordingManager {
public:
    using ProgressCallback = std::function<void(float progress, int frame, int total)>;
    using CompleteCallback = std::function<void(bool success, const std::string& error)>;
    
    RecordingManager() = default;
    ~RecordingManager() { stopRecording(); }
    
    // Set frame capture source
    void setFrameCapture(std::shared_ptr<IFrameCapture> capture) {
        frameCapture_ = capture;
    }
    
    // Start recording
    bool startRecording(const VideoExportSettings& settings) {
        if (state_ == RecordingState::Recording) {
            return false;
        }
        
        settings_ = settings;
        state_ = RecordingState::Preparing;
        frameCount_ = 0;
        startTime_ = 0.0;
        error_.clear();
        
        // Create encoder based on format
        if (settings.format == VideoFormat::ImageSequence_PNG ||
            settings.format == VideoFormat::ImageSequence_JPG ||
            settings.format == VideoFormat::ImageSequence_TGA) {
            encoder_ = std::make_unique<ImageSequenceEncoder>();
        } else {
            encoder_ = std::make_unique<FFmpegEncoder>();
        }
        
        if (!encoder_->initialize(settings)) {
            error_ = encoder_->getError();
            state_ = RecordingState::Error;
            return false;
        }
        
        state_ = RecordingState::Recording;
        return true;
    }
    
    // Stop recording
    void stopRecording() {
        if (state_ != RecordingState::Recording && state_ != RecordingState::Paused) {
            return;
        }
        
        state_ = RecordingState::Finalizing;
        
        if (encoder_) {
            encoder_->finalize();
        }
        
        state_ = RecordingState::Complete;
        
        if (completeCallback_) {
            completeCallback_(true, "");
        }
    }
    
    // Pause/resume
    void pauseRecording() {
        if (state_ == RecordingState::Recording) {
            state_ = RecordingState::Paused;
        }
    }
    
    void resumeRecording() {
        if (state_ == RecordingState::Paused) {
            state_ = RecordingState::Recording;
        }
    }
    
    // Capture current frame (call from render loop)
    bool captureFrame(double currentTime) {
        if (state_ != RecordingState::Recording) {
            return false;
        }
        
        if (!frameCapture_ || !encoder_) {
            return false;
        }
        
        // Check if we should capture this frame
        if (settings_.captureEveryFrame) {
            double targetTime = settings_.startTime + frameCount_ * settings_.getFrameDuration();
            if (currentTime < targetTime) {
                return false;  // Not time yet
            }
        }
        
        // Check if recording is complete
        int totalFrames = settings_.getTotalFrames();
        if (frameCount_ >= (uint64_t)totalFrames) {
            stopRecording();
            return false;
        }
        
        // Capture frame
        FrameData frame;
        if (!frameCapture_->capture(frame)) {
            error_ = "Failed to capture frame";
            return false;
        }
        
        frame.frameNumber = frameCount_;
        frame.timestamp = settings_.startTime + frameCount_ * settings_.getFrameDuration();
        
        // Flip if needed (OpenGL)
        frame.flipVertical();
        
        // Encode
        if (!encoder_->encodeFrame(frame)) {
            error_ = encoder_->getError();
            state_ = RecordingState::Error;
            return false;
        }
        
        frameCount_++;
        
        // Progress callback
        if (progressCallback_) {
            progressCallback_(getProgress(), (int)frameCount_, totalFrames);
        }
        
        return true;
    }
    
    // Manual frame submission
    bool submitFrame(const FrameData& frame) {
        if (state_ != RecordingState::Recording || !encoder_) {
            return false;
        }
        
        if (!encoder_->encodeFrame(frame)) {
            error_ = encoder_->getError();
            return false;
        }
        
        frameCount_++;
        return true;
    }
    
    // State
    RecordingState getState() const { return state_; }
    bool isRecording() const { return state_ == RecordingState::Recording; }
    bool isPaused() const { return state_ == RecordingState::Paused; }
    
    // Progress
    float getProgress() const {
        int total = settings_.getTotalFrames();
        if (total == 0) return 0.0f;
        return (float)frameCount_ / total;
    }
    
    uint64_t getFrameCount() const { return frameCount_; }
    int getTotalFrames() const { return settings_.getTotalFrames(); }
    
    // Error
    std::string getError() const { return error_; }
    
    // Callbacks
    void setProgressCallback(ProgressCallback callback) { progressCallback_ = callback; }
    void setCompleteCallback(CompleteCallback callback) { completeCallback_ = callback; }
    
    // Settings
    const VideoExportSettings& getSettings() const { return settings_; }
    
    // Estimated file size (rough)
    size_t getEstimatedFileSize() const {
        double duration = settings_.endTime - settings_.startTime;
        return (size_t)(settings_.bitrate * duration / 8);
    }
    
    // Estimated time remaining
    double getEstimatedTimeRemaining(double avgFrameTime) const {
        int remaining = settings_.getTotalFrames() - (int)frameCount_;
        return remaining * avgFrameTime;
    }
    
private:
    VideoExportSettings settings_;
    std::shared_ptr<IFrameCapture> frameCapture_;
    std::unique_ptr<IVideoEncoder> encoder_;
    
    RecordingState state_ = RecordingState::Idle;
    uint64_t frameCount_ = 0;
    double startTime_ = 0.0;
    std::string error_;
    
    ProgressCallback progressCallback_;
    CompleteCallback completeCallback_;
};

// ===== Global Recording Manager =====
inline RecordingManager& getRecordingManager() {
    static RecordingManager manager;
    return manager;
}

// ===== GIF Encoder (simple implementation) =====
class GIFEncoder : public IVideoEncoder {
public:
    bool initialize(const VideoExportSettings& settings) override {
        settings_ = settings;
        frameCount_ = 0;
        
        file_.open(settings.outputPath, std::ios::binary);
        if (!file_) {
            error_ = "Failed to create GIF file";
            return false;
        }
        
        // Write GIF header
        writeHeader();
        return true;
    }
    
    bool encodeFrame(const FrameData& frame) override {
        if (!file_) return false;
        
        // Simplified GIF frame writing
        // In production, would use proper LZW compression and color quantization
        writeFrame(frame);
        frameCount_++;
        return true;
    }
    
    bool finalize() override {
        if (file_) {
            // Write GIF trailer
            file_.put(0x3B);
            file_.close();
        }
        return true;
    }
    
    float getProgress() const override {
        if (settings_.getTotalFrames() == 0) return 1.0f;
        return (float)frameCount_ / settings_.getTotalFrames();
    }
    
    std::string getError() const override { return error_; }
    
private:
    void writeHeader() {
        // GIF89a header
        file_.write("GIF89a", 6);
        
        // Logical screen descriptor
        uint16_t width = settings_.width;
        uint16_t height = settings_.height;
        file_.write((char*)&width, 2);
        file_.write((char*)&height, 2);
        
        // Packed field: global color table, color resolution, sorted, size
        uint8_t packed = 0xF7;  // 256 color global table
        file_.put(packed);
        file_.put(0);  // Background color index
        file_.put(0);  // Pixel aspect ratio
        
        // Global color table (256 colors)
        for (int i = 0; i < 256; i++) {
            file_.put(i);  // R
            file_.put(i);  // G
            file_.put(i);  // B
        }
        
        // Netscape extension for looping
        file_.put(0x21);  // Extension
        file_.put(0xFF);  // Application extension
        file_.put(0x0B);  // Block size
        file_.write("NETSCAPE2.0", 11);
        file_.put(0x03);  // Data block size
        file_.put(0x01);  // Loop indicator
        file_.put(0x00);  // Loop count (0 = infinite)
        file_.put(0x00);
        file_.put(0x00);  // Block terminator
    }
    
    void writeFrame(const FrameData& frame) {
        // Graphics control extension
        file_.put(0x21);  // Extension
        file_.put(0xF9);  // Graphics control
        file_.put(0x04);  // Block size
        
        int delay = 100 / settings_.frameRate;  // Delay in centiseconds
        file_.put(0x00);  // Packed (no transparency)
        file_.put(delay & 0xFF);
        file_.put((delay >> 8) & 0xFF);
        file_.put(0x00);  // Transparent color index
        file_.put(0x00);  // Block terminator
        
        // Image descriptor
        file_.put(0x2C);  // Image separator
        file_.put(0x00); file_.put(0x00);  // Left
        file_.put(0x00); file_.put(0x00);  // Top
        file_.put(settings_.width & 0xFF);
        file_.put((settings_.width >> 8) & 0xFF);
        file_.put(settings_.height & 0xFF);
        file_.put((settings_.height >> 8) & 0xFF);
        file_.put(0x00);  // Packed (no local color table)
        
        // Image data (simplified - just grayscale)
        file_.put(0x08);  // LZW minimum code size
        
        // Very simplified "encoding" - not real LZW
        // In production, would implement proper LZW compression
        std::vector<uint8_t> indices;
        for (int y = 0; y < frame.height; y++) {
            for (int x = 0; x < frame.width; x++) {
                int idx = (y * frame.width + x) * frame.channels;
                // Convert to grayscale
                uint8_t gray = (frame.pixels[idx] + frame.pixels[idx+1] + frame.pixels[idx+2]) / 3;
                indices.push_back(gray);
            }
        }
        
        // Write in sub-blocks (max 255 bytes each)
        size_t offset = 0;
        while (offset < indices.size()) {
            size_t blockSize = std::min((size_t)255, indices.size() - offset);
            file_.put((uint8_t)blockSize);
            file_.write((char*)&indices[offset], blockSize);
            offset += blockSize;
        }
        
        file_.put(0x00);  // Block terminator
    }
    
    VideoExportSettings settings_;
    std::ofstream file_;
    uint64_t frameCount_ = 0;
    std::string error_;
};

}  // namespace luma
