// Lip Sync System - Audio-driven mouth animation
// Analyzes audio and drives BlendShape-based mouth shapes
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <functional>
#include <cmath>
#include <deque>

namespace luma {

// ============================================================================
// Viseme Definition (Mouth Shapes)
// ============================================================================

enum class Viseme {
    Silence,    // 沉默 - Mouth closed
    AA,         // "ah" as in "father"
    AE,         // "a" as in "cat"
    AH,         // "u" as in "but"
    AO,         // "o" as in "caught"
    AW,         // "ow" as in "cow"
    AY,         // "i" as in "bite"
    B_M_P,      // b, m, p - lips together
    CH_J_SH,    // ch, j, sh - teeth together, lips rounded
    D_T_N,      // d, t, n - tongue on teeth ridge
    EH,         // "e" as in "bet"
    ER,         // "er" as in "bird"
    EY,         // "a" as in "bake"
    F_V,        // f, v - lower lip to upper teeth
    G_K_NG,     // g, k, ng - back of tongue raised
    IH,         // "i" as in "bit"
    IY,         // "ee" as in "beet"
    L,          // l - tongue tip up
    OW,         // "o" as in "boat"
    OY,         // "oy" as in "boy"
    R,          // r - tongue back
    S_Z,        // s, z - teeth together
    TH,         // th - tongue between teeth
    UH,         // "oo" as in "book"
    UW,         // "oo" as in "boot"
    W,          // w - lips rounded
    Y,          // y - tongue high
    COUNT
};

inline std::string visemeToString(Viseme v) {
    switch (v) {
        case Viseme::Silence: return "Silence";
        case Viseme::AA: return "AA";
        case Viseme::AE: return "AE";
        case Viseme::AH: return "AH";
        case Viseme::AO: return "AO";
        case Viseme::AW: return "AW";
        case Viseme::AY: return "AY";
        case Viseme::B_M_P: return "B_M_P";
        case Viseme::CH_J_SH: return "CH_J_SH";
        case Viseme::D_T_N: return "D_T_N";
        case Viseme::EH: return "EH";
        case Viseme::ER: return "ER";
        case Viseme::EY: return "EY";
        case Viseme::F_V: return "F_V";
        case Viseme::G_K_NG: return "G_K_NG";
        case Viseme::IH: return "IH";
        case Viseme::IY: return "IY";
        case Viseme::L: return "L";
        case Viseme::OW: return "OW";
        case Viseme::OY: return "OY";
        case Viseme::R: return "R";
        case Viseme::S_Z: return "S_Z";
        case Viseme::TH: return "TH";
        case Viseme::UH: return "UH";
        case Viseme::UW: return "UW";
        case Viseme::W: return "W";
        case Viseme::Y: return "Y";
        default: return "Unknown";
    }
}

// ============================================================================
// Viseme to BlendShape Mapping
// ============================================================================

struct VisemeBlendShapes {
    // ARKit-style BlendShape names and weights for each viseme
    std::unordered_map<std::string, float> shapes;
};

class VisemeMapping {
public:
    static VisemeMapping& getInstance() {
        static VisemeMapping instance;
        return instance;
    }
    
    void initialize() {
        if (initialized_) return;
        
        // Define BlendShape weights for each viseme
        // Based on ARKit 52 BlendShapes
        
        // Silence - neutral mouth
        mappings_[Viseme::Silence] = {};
        
        // AA - open mouth, tongue low
        mappings_[Viseme::AA] = {
            {"jawOpen", 0.6f},
            {"mouthOpen", 0.4f}
        };
        
        // AE - as in "cat"
        mappings_[Viseme::AE] = {
            {"jawOpen", 0.4f},
            {"mouthOpen", 0.3f},
            {"mouthStretchLeft", 0.2f},
            {"mouthStretchRight", 0.2f}
        };
        
        // AH - as in "but"
        mappings_[Viseme::AH] = {
            {"jawOpen", 0.35f},
            {"mouthOpen", 0.25f}
        };
        
        // AO - as in "caught"
        mappings_[Viseme::AO] = {
            {"jawOpen", 0.5f},
            {"mouthFunnel", 0.3f}
        };
        
        // AW - as in "cow"
        mappings_[Viseme::AW] = {
            {"jawOpen", 0.4f},
            {"mouthFunnel", 0.4f},
            {"mouthPucker", 0.2f}
        };
        
        // AY - as in "bite"
        mappings_[Viseme::AY] = {
            {"jawOpen", 0.3f},
            {"mouthSmileLeft", 0.2f},
            {"mouthSmileRight", 0.2f}
        };
        
        // B, M, P - lips pressed together
        mappings_[Viseme::B_M_P] = {
            {"mouthClose", 0.8f},
            {"mouthPressLeft", 0.3f},
            {"mouthPressRight", 0.3f}
        };
        
        // CH, J, SH - teeth together, lips rounded
        mappings_[Viseme::CH_J_SH] = {
            {"jawOpen", 0.15f},
            {"mouthFunnel", 0.4f},
            {"mouthShrugLower", 0.2f}
        };
        
        // D, T, N - tongue tip on teeth ridge
        mappings_[Viseme::D_T_N] = {
            {"jawOpen", 0.2f},
            {"tongueOut", 0.2f}
        };
        
        // EH - as in "bet"
        mappings_[Viseme::EH] = {
            {"jawOpen", 0.25f},
            {"mouthStretchLeft", 0.15f},
            {"mouthStretchRight", 0.15f}
        };
        
        // ER - as in "bird"
        mappings_[Viseme::ER] = {
            {"jawOpen", 0.2f},
            {"mouthFunnel", 0.2f},
            {"mouthRollLower", 0.1f}
        };
        
        // EY - as in "bake"
        mappings_[Viseme::EY] = {
            {"jawOpen", 0.2f},
            {"mouthSmileLeft", 0.25f},
            {"mouthSmileRight", 0.25f}
        };
        
        // F, V - lower lip to upper teeth
        mappings_[Viseme::F_V] = {
            {"jawOpen", 0.1f},
            {"mouthRollLower", 0.5f},
            {"mouthLowerDownLeft", 0.2f},
            {"mouthLowerDownRight", 0.2f}
        };
        
        // G, K, NG - back of tongue raised
        mappings_[Viseme::G_K_NG] = {
            {"jawOpen", 0.25f},
            {"mouthOpen", 0.15f}
        };
        
        // IH - as in "bit"
        mappings_[Viseme::IH] = {
            {"jawOpen", 0.15f},
            {"mouthStretchLeft", 0.25f},
            {"mouthStretchRight", 0.25f}
        };
        
        // IY - as in "beet"
        mappings_[Viseme::IY] = {
            {"jawOpen", 0.1f},
            {"mouthSmileLeft", 0.35f},
            {"mouthSmileRight", 0.35f}
        };
        
        // L - tongue tip up
        mappings_[Viseme::L] = {
            {"jawOpen", 0.2f},
            {"tongueOut", 0.15f}
        };
        
        // OW - as in "boat"
        mappings_[Viseme::OW] = {
            {"jawOpen", 0.35f},
            {"mouthFunnel", 0.5f},
            {"mouthPucker", 0.3f}
        };
        
        // OY - as in "boy"
        mappings_[Viseme::OY] = {
            {"jawOpen", 0.3f},
            {"mouthFunnel", 0.4f}
        };
        
        // R - tongue back
        mappings_[Viseme::R] = {
            {"jawOpen", 0.15f},
            {"mouthFunnel", 0.25f}
        };
        
        // S, Z - teeth together
        mappings_[Viseme::S_Z] = {
            {"jawOpen", 0.1f},
            {"mouthStretchLeft", 0.1f},
            {"mouthStretchRight", 0.1f}
        };
        
        // TH - tongue between teeth
        mappings_[Viseme::TH] = {
            {"jawOpen", 0.15f},
            {"tongueOut", 0.4f}
        };
        
        // UH - as in "book"
        mappings_[Viseme::UH] = {
            {"jawOpen", 0.2f},
            {"mouthFunnel", 0.3f}
        };
        
        // UW - as in "boot"
        mappings_[Viseme::UW] = {
            {"jawOpen", 0.15f},
            {"mouthFunnel", 0.5f},
            {"mouthPucker", 0.5f}
        };
        
        // W - lips rounded
        mappings_[Viseme::W] = {
            {"mouthFunnel", 0.6f},
            {"mouthPucker", 0.5f}
        };
        
        // Y - tongue high
        mappings_[Viseme::Y] = {
            {"jawOpen", 0.1f},
            {"mouthSmileLeft", 0.2f},
            {"mouthSmileRight", 0.2f}
        };
        
        initialized_ = true;
    }
    
    const VisemeBlendShapes& getMapping(Viseme viseme) const {
        static VisemeBlendShapes empty;
        auto it = mappings_.find(viseme);
        return (it != mappings_.end()) ? it->second : empty;
    }
    
    // Get all BlendShape names used
    std::vector<std::string> getAllBlendShapeNames() const {
        std::vector<std::string> names;
        for (const auto& [viseme, shapes] : mappings_) {
            for (const auto& [name, weight] : shapes.shapes) {
                if (std::find(names.begin(), names.end(), name) == names.end()) {
                    names.push_back(name);
                }
            }
        }
        return names;
    }
    
private:
    VisemeMapping() { initialize(); }
    
    std::unordered_map<Viseme, VisemeBlendShapes> mappings_;
    bool initialized_ = false;
};

// ============================================================================
// Audio Analysis
// ============================================================================

struct AudioFrame {
    float timestamp = 0;
    float amplitude = 0;        // RMS amplitude
    float pitch = 0;            // Estimated pitch frequency
    std::array<float, 8> spectrum{};  // Frequency bands
    
    // Derived features
    float energy = 0;
    float zeroCrossingRate = 0;
    float spectralCentroid = 0;
};

class AudioAnalyzer {
public:
    // Analyze audio samples
    // samples: mono audio data, sampleRate: e.g., 44100
    AudioFrame analyze(const float* samples, int numSamples, float sampleRate, float timestamp) {
        AudioFrame frame;
        frame.timestamp = timestamp;
        
        if (numSamples == 0 || !samples) return frame;
        
        // Calculate RMS amplitude
        float sumSquares = 0;
        for (int i = 0; i < numSamples; i++) {
            sumSquares += samples[i] * samples[i];
        }
        frame.amplitude = std::sqrt(sumSquares / numSamples);
        
        // Zero crossing rate
        int crossings = 0;
        for (int i = 1; i < numSamples; i++) {
            if ((samples[i] >= 0) != (samples[i-1] >= 0)) {
                crossings++;
            }
        }
        frame.zeroCrossingRate = static_cast<float>(crossings) / numSamples;
        
        // Simple pitch estimation using autocorrelation
        frame.pitch = estimatePitch(samples, numSamples, sampleRate);
        
        // Simple spectrum analysis (8 bands)
        analyzeSpectrum(samples, numSamples, sampleRate, frame.spectrum);
        
        // Energy (sum of spectrum)
        frame.energy = 0;
        for (float band : frame.spectrum) {
            frame.energy += band;
        }
        
        // Spectral centroid
        float weightedSum = 0;
        float totalWeight = 0;
        for (int i = 0; i < 8; i++) {
            float freq = (i + 0.5f) * sampleRate / 16.0f;
            weightedSum += freq * frame.spectrum[i];
            totalWeight += frame.spectrum[i];
        }
        frame.spectralCentroid = (totalWeight > 0) ? weightedSum / totalWeight : 0;
        
        return frame;
    }
    
private:
    float estimatePitch(const float* samples, int numSamples, float sampleRate) {
        // Simple autocorrelation-based pitch detection
        int minLag = static_cast<int>(sampleRate / 500);   // Max 500 Hz
        int maxLag = static_cast<int>(sampleRate / 80);    // Min 80 Hz
        
        if (maxLag >= numSamples) maxLag = numSamples - 1;
        
        float maxCorr = 0;
        int bestLag = minLag;
        
        for (int lag = minLag; lag <= maxLag; lag++) {
            float corr = 0;
            for (int i = 0; i < numSamples - lag; i++) {
                corr += samples[i] * samples[i + lag];
            }
            corr /= (numSamples - lag);
            
            if (corr > maxCorr) {
                maxCorr = corr;
                bestLag = lag;
            }
        }
        
        return sampleRate / bestLag;
    }
    
    void analyzeSpectrum(const float* samples, int numSamples, float sampleRate,
                         std::array<float, 8>& spectrum) {
        // Simple filter bank analysis (not FFT for simplicity)
        // Band edges roughly: 100, 200, 400, 800, 1600, 3200, 6400, 12800 Hz
        
        std::fill(spectrum.begin(), spectrum.end(), 0.0f);
        
        // Use simple bandpass filters
        for (int band = 0; band < 8; band++) {
            float lowFreq = 100.0f * std::pow(2.0f, band);
            float highFreq = 100.0f * std::pow(2.0f, band + 1);
            
            // Simple energy measurement with frequency weighting
            float energy = 0;
            for (int i = 1; i < numSamples; i++) {
                float diff = samples[i] - samples[i-1];
                // Higher bands respond more to rapid changes
                energy += diff * diff * (1.0f + band * 0.5f);
            }
            spectrum[band] = std::sqrt(energy / numSamples);
        }
    }
};

// ============================================================================
// Lip Sync Engine
// ============================================================================

struct LipSyncSettings {
    float smoothingFactor = 0.3f;       // Blend between frames
    float amplitudeThreshold = 0.01f;   // Below this = silence
    float amplitudeScale = 1.5f;        // Multiply amplitude for mouth openness
    float transitionSpeed = 15.0f;      // Viseme transition speed
    
    bool autoAmplitude = true;          // Auto-adjust to audio level
    float minAmplitude = 0.0f;          // For auto-amplitude normalization
    float maxAmplitude = 1.0f;
    
    // Emphasis
    float jawEmphasis = 1.0f;           // Extra jaw movement
    float lipEmphasis = 1.0f;           // Extra lip movement
    
    // Delay for timing adjustment
    float audioDelay = 0.0f;            // Positive = lip sync ahead of audio
};

class LipSyncEngine {
public:
    LipSyncEngine() {
        VisemeMapping::getInstance().initialize();
    }
    
    // Process audio frame and get BlendShape weights
    std::unordered_map<std::string, float> process(const AudioFrame& frame,
                                                    const LipSyncSettings& settings) {
        // Determine current viseme from audio features
        Viseme targetViseme = classifyViseme(frame, settings);
        
        // Get target weights
        auto& mapping = VisemeMapping::getInstance().getMapping(targetViseme);
        std::unordered_map<std::string, float> targetWeights;
        for (const auto& [name, weight] : mapping.shapes) {
            targetWeights[name] = weight;
        }
        
        // Apply amplitude-based scaling
        float amplitudeScale = 1.0f;
        if (settings.autoAmplitude) {
            updateAmplitudeRange(frame.amplitude);
            float normalizedAmp = (frame.amplitude - amplitudeMin_) / 
                                  (amplitudeMax_ - amplitudeMin_ + 0.001f);
            amplitudeScale = normalizedAmp * settings.amplitudeScale;
        } else {
            amplitudeScale = frame.amplitude * settings.amplitudeScale;
        }
        amplitudeScale = std::clamp(amplitudeScale, 0.0f, 1.5f);
        
        // Scale weights by amplitude
        for (auto& [name, weight] : targetWeights) {
            weight *= amplitudeScale;
            
            // Apply emphasis
            if (name.find("jaw") != std::string::npos) {
                weight *= settings.jawEmphasis;
            }
            if (name.find("mouth") != std::string::npos || name.find("lip") != std::string::npos) {
                weight *= settings.lipEmphasis;
            }
        }
        
        // Smooth transition from current to target
        for (auto& [name, target] : targetWeights) {
            float current = currentWeights_[name];
            float smoothed = current + (target - current) * settings.smoothingFactor;
            currentWeights_[name] = smoothed;
        }
        
        // Decay unused weights
        for (auto& [name, weight] : currentWeights_) {
            if (targetWeights.find(name) == targetWeights.end()) {
                weight *= (1.0f - settings.smoothingFactor);
            }
        }
        
        return currentWeights_;
    }
    
    // Reset state
    void reset() {
        currentWeights_.clear();
        currentViseme_ = Viseme::Silence;
        amplitudeMin_ = 1.0f;
        amplitudeMax_ = 0.0f;
    }
    
    // Get current viseme
    Viseme getCurrentViseme() const { return currentViseme_; }
    
private:
    Viseme classifyViseme(const AudioFrame& frame, const LipSyncSettings& settings) {
        // Below threshold = silence
        if (frame.amplitude < settings.amplitudeThreshold) {
            currentViseme_ = Viseme::Silence;
            return Viseme::Silence;
        }
        
        // Simple viseme classification based on spectral features
        // This is a simplified approach - real systems use ML or phoneme timing
        
        float lowEnergy = frame.spectrum[0] + frame.spectrum[1];    // 100-400 Hz
        float midEnergy = frame.spectrum[2] + frame.spectrum[3];    // 400-1600 Hz
        float highEnergy = frame.spectrum[4] + frame.spectrum[5];   // 1600-6400 Hz
        float veryHighEnergy = frame.spectrum[6] + frame.spectrum[7]; // 6400+ Hz
        
        float totalEnergy = lowEnergy + midEnergy + highEnergy + veryHighEnergy + 0.001f;
        
        float lowRatio = lowEnergy / totalEnergy;
        float midRatio = midEnergy / totalEnergy;
        float highRatio = highEnergy / totalEnergy;
        
        Viseme viseme = Viseme::AH;  // Default open mouth
        
        // Classify based on spectral shape
        if (veryHighEnergy > highEnergy && highRatio > 0.3f) {
            // Sibilants (s, z, sh)
            viseme = Viseme::S_Z;
        } else if (frame.zeroCrossingRate > 0.3f && highRatio > 0.25f) {
            // Fricatives
            if (lowRatio > 0.3f) {
                viseme = Viseme::F_V;
            } else {
                viseme = Viseme::CH_J_SH;
            }
        } else if (lowRatio > 0.5f) {
            // Low frequency dominant - back vowels
            if (midRatio > 0.25f) {
                viseme = Viseme::AO;
            } else {
                viseme = Viseme::UW;
            }
        } else if (midRatio > 0.4f) {
            // Mid frequency dominant
            if (highRatio > 0.2f) {
                viseme = Viseme::EH;
            } else {
                viseme = Viseme::AA;
            }
        } else if (highRatio > 0.35f) {
            // High frequency dominant - front vowels
            viseme = Viseme::IY;
        } else {
            // Balanced - neutral vowels
            viseme = Viseme::AH;
        }
        
        // Check for stops (sudden energy changes)
        if (frame.energy > prevEnergy_ * 3.0f && frame.amplitude > 0.1f) {
            // Possible plosive
            if (lowRatio > 0.4f) {
                viseme = Viseme::B_M_P;
            } else {
                viseme = Viseme::D_T_N;
            }
        }
        
        prevEnergy_ = frame.energy;
        currentViseme_ = viseme;
        
        return viseme;
    }
    
    void updateAmplitudeRange(float amplitude) {
        // Slowly adapt to audio levels
        const float adaptRate = 0.01f;
        
        if (amplitude < amplitudeMin_) {
            amplitudeMin_ = amplitude;
        } else {
            amplitudeMin_ += (amplitude - amplitudeMin_) * adaptRate;
        }
        
        if (amplitude > amplitudeMax_) {
            amplitudeMax_ = amplitude;
        } else {
            amplitudeMax_ -= (amplitudeMax_ - amplitude) * adaptRate;
        }
        
        // Ensure reasonable range
        if (amplitudeMax_ - amplitudeMin_ < 0.05f) {
            amplitudeMax_ = amplitudeMin_ + 0.05f;
        }
    }
    
    std::unordered_map<std::string, float> currentWeights_;
    Viseme currentViseme_ = Viseme::Silence;
    
    float amplitudeMin_ = 1.0f;
    float amplitudeMax_ = 0.0f;
    float prevEnergy_ = 0;
};

// ============================================================================
// Lip Sync Track - Pre-baked animation from audio file
// ============================================================================

struct VisemeKeyframe {
    float time = 0;
    Viseme viseme = Viseme::Silence;
    float weight = 1.0f;
};

struct LipSyncTrack {
    std::string name;
    float duration = 0;
    std::vector<VisemeKeyframe> keyframes;
    
    // Sample at time
    std::pair<Viseme, float> sample(float time) const {
        if (keyframes.empty()) return {Viseme::Silence, 0};
        
        // Find surrounding keyframes
        int idx = 0;
        for (size_t i = 0; i < keyframes.size(); i++) {
            if (keyframes[i].time > time) break;
            idx = static_cast<int>(i);
        }
        
        return {keyframes[idx].viseme, keyframes[idx].weight};
    }
    
    // Add keyframe
    void addKeyframe(float time, Viseme viseme, float weight = 1.0f) {
        VisemeKeyframe kf;
        kf.time = time;
        kf.viseme = viseme;
        kf.weight = weight;
        
        // Insert sorted
        auto it = keyframes.begin();
        while (it != keyframes.end() && it->time < time) {
            ++it;
        }
        keyframes.insert(it, kf);
        
        duration = std::max(duration, time);
    }
};

// ============================================================================
// Lip Sync Generator - Generate track from audio
// ============================================================================

class LipSyncGenerator {
public:
    // Generate lip sync track from audio data
    LipSyncTrack generate(const float* samples, int numSamples, float sampleRate,
                          const LipSyncSettings& settings = {}) {
        LipSyncTrack track;
        track.name = "Generated";
        
        LipSyncEngine engine;
        AudioAnalyzer analyzer;
        
        // Process audio in chunks
        int chunkSize = static_cast<int>(sampleRate / 30.0f);  // ~30 fps
        int numChunks = numSamples / chunkSize;
        
        Viseme lastViseme = Viseme::Silence;
        
        for (int i = 0; i < numChunks; i++) {
            float time = static_cast<float>(i * chunkSize) / sampleRate;
            
            AudioFrame frame = analyzer.analyze(
                samples + i * chunkSize, 
                chunkSize, 
                sampleRate, 
                time
            );
            
            engine.process(frame, settings);
            Viseme currentViseme = engine.getCurrentViseme();
            
            // Add keyframe on viseme change or periodically
            if (currentViseme != lastViseme || i % 5 == 0) {
                track.addKeyframe(time, currentViseme, frame.amplitude * settings.amplitudeScale);
                lastViseme = currentViseme;
            }
        }
        
        track.duration = static_cast<float>(numSamples) / sampleRate;
        
        return track;
    }
    
    // Generate from phoneme timing (for TTS or subtitle-based)
    LipSyncTrack generateFromPhonemes(const std::vector<std::pair<float, std::string>>& phonemeTiming) {
        LipSyncTrack track;
        track.name = "FromPhonemes";
        
        for (const auto& [time, phoneme] : phonemeTiming) {
            Viseme viseme = phonemeToViseme(phoneme);
            track.addKeyframe(time, viseme);
        }
        
        if (!phonemeTiming.empty()) {
            track.duration = phonemeTiming.back().first + 0.5f;
        }
        
        return track;
    }
    
private:
    Viseme phonemeToViseme(const std::string& phoneme) {
        // ARPABET phoneme to viseme mapping
        static std::unordered_map<std::string, Viseme> mapping = {
            {"AA", Viseme::AA}, {"AE", Viseme::AE}, {"AH", Viseme::AH},
            {"AO", Viseme::AO}, {"AW", Viseme::AW}, {"AY", Viseme::AY},
            {"B", Viseme::B_M_P}, {"CH", Viseme::CH_J_SH},
            {"D", Viseme::D_T_N}, {"DH", Viseme::TH},
            {"EH", Viseme::EH}, {"ER", Viseme::ER}, {"EY", Viseme::EY},
            {"F", Viseme::F_V}, {"G", Viseme::G_K_NG},
            {"HH", Viseme::AH}, {"IH", Viseme::IH}, {"IY", Viseme::IY},
            {"JH", Viseme::CH_J_SH}, {"K", Viseme::G_K_NG},
            {"L", Viseme::L}, {"M", Viseme::B_M_P}, {"N", Viseme::D_T_N},
            {"NG", Viseme::G_K_NG}, {"OW", Viseme::OW}, {"OY", Viseme::OY},
            {"P", Viseme::B_M_P}, {"R", Viseme::R},
            {"S", Viseme::S_Z}, {"SH", Viseme::CH_J_SH},
            {"T", Viseme::D_T_N}, {"TH", Viseme::TH},
            {"UH", Viseme::UH}, {"UW", Viseme::UW},
            {"V", Viseme::F_V}, {"W", Viseme::W}, {"Y", Viseme::Y},
            {"Z", Viseme::S_Z}, {"ZH", Viseme::CH_J_SH},
            {"SIL", Viseme::Silence}, {"SP", Viseme::Silence}
        };
        
        auto it = mapping.find(phoneme);
        return (it != mapping.end()) ? it->second : Viseme::AH;
    }
};

// ============================================================================
// Lip Sync Player - Play lip sync with character
// ============================================================================

class LipSyncPlayer {
public:
    void setTrack(const LipSyncTrack& track) {
        track_ = track;
        currentTime_ = 0;
    }
    
    void setBlendShapeCallback(std::function<void(const std::string&, float)> callback) {
        blendShapeCallback_ = callback;
    }
    
    void play() {
        isPlaying_ = true;
    }
    
    void pause() {
        isPlaying_ = false;
    }
    
    void stop() {
        isPlaying_ = false;
        currentTime_ = 0;
        applyViseme(Viseme::Silence, 0);
    }
    
    void setTime(float time) {
        currentTime_ = time;
    }
    
    void update(float deltaTime, const LipSyncSettings& settings) {
        if (!isPlaying_) return;
        
        currentTime_ += deltaTime;
        
        if (currentTime_ > track_.duration) {
            if (loop_) {
                currentTime_ = 0;
            } else {
                stop();
                return;
            }
        }
        
        // Sample track
        auto [viseme, weight] = track_.sample(currentTime_);
        
        // Smooth transition
        targetViseme_ = viseme;
        targetWeight_ = weight;
        
        // Apply with smoothing
        applyViseme(viseme, weight * settings.amplitudeScale);
    }
    
    bool isPlaying() const { return isPlaying_; }
    float getCurrentTime() const { return currentTime_; }
    
    void setLoop(bool loop) { loop_ = loop; }
    
private:
    void applyViseme(Viseme viseme, float intensity) {
        if (!blendShapeCallback_) return;
        
        auto& mapping = VisemeMapping::getInstance().getMapping(viseme);
        
        // Apply all shapes in the mapping
        for (const auto& [name, weight] : mapping.shapes) {
            float finalWeight = weight * intensity;
            
            // Smooth from current
            float& current = currentWeights_[name];
            current = current + (finalWeight - current) * 0.3f;
            
            blendShapeCallback_(name, current);
        }
        
        // Fade out unused shapes
        for (auto& [name, current] : currentWeights_) {
            if (mapping.shapes.find(name) == mapping.shapes.end()) {
                current *= 0.7f;
                if (current > 0.01f) {
                    blendShapeCallback_(name, current);
                }
            }
        }
    }
    
    LipSyncTrack track_;
    float currentTime_ = 0;
    bool isPlaying_ = false;
    bool loop_ = false;
    
    Viseme targetViseme_ = Viseme::Silence;
    float targetWeight_ = 0;
    
    std::unordered_map<std::string, float> currentWeights_;
    std::function<void(const std::string&, float)> blendShapeCallback_;
};

// ============================================================================
// Lip Sync Manager
// ============================================================================

class LipSyncManager {
public:
    static LipSyncManager& getInstance() {
        static LipSyncManager instance;
        return instance;
    }
    
    void initialize() {
        VisemeMapping::getInstance().initialize();
    }
    
    // Get components
    LipSyncEngine& getEngine() { return engine_; }
    LipSyncGenerator& getGenerator() { return generator_; }
    LipSyncPlayer& getPlayer() { return player_; }
    LipSyncSettings& getSettings() { return settings_; }
    
private:
    LipSyncManager() = default;
    
    LipSyncEngine engine_;
    LipSyncGenerator generator_;
    LipSyncPlayer player_;
    LipSyncSettings settings_;
};

inline LipSyncManager& getLipSync() {
    return LipSyncManager::getInstance();
}

}  // namespace luma
