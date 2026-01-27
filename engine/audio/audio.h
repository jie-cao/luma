// Audio System - Core audio management
// AudioClip, AudioSource, AudioListener, 3D spatial audio
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <cmath>
#include <algorithm>
#include <mutex>
#include <atomic>

namespace luma {

// Forward declarations
class AudioClip;
class AudioSource;
class AudioListener;
class AudioMixer;

// ===== Audio Format =====
enum class AudioFormat {
    Mono8,
    Mono16,
    Stereo8,
    Stereo16,
    MonoFloat,
    StereoFloat
};

// ===== Audio Clip =====
// Loaded audio data (WAV, etc.)
class AudioClip {
public:
    AudioClip() = default;
    AudioClip(const std::string& name) : name_(name) {}
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    // Audio properties
    int getSampleRate() const { return sampleRate_; }
    int getChannels() const { return channels_; }
    int getBitsPerSample() const { return bitsPerSample_; }
    AudioFormat getFormat() const { return format_; }
    
    // Duration in seconds
    float getDuration() const {
        if (sampleRate_ == 0 || channels_ == 0) return 0.0f;
        size_t bytesPerSample = bitsPerSample_ / 8;
        size_t totalSamples = data_.size() / (bytesPerSample * channels_);
        return (float)totalSamples / sampleRate_;
    }
    
    // Data access
    const std::vector<uint8_t>& getData() const { return data_; }
    std::vector<uint8_t>& getData() { return data_; }
    size_t getDataSize() const { return data_.size(); }
    
    // Load from WAV file (simplified - header only)
    bool loadFromMemory(const uint8_t* data, size_t size,
                        int sampleRate, int channels, int bitsPerSample)
    {
        data_.assign(data, data + size);
        sampleRate_ = sampleRate;
        channels_ = channels;
        bitsPerSample_ = bitsPerSample;
        
        // Determine format
        if (channels == 1) {
            if (bitsPerSample == 8) format_ = AudioFormat::Mono8;
            else if (bitsPerSample == 16) format_ = AudioFormat::Mono16;
            else format_ = AudioFormat::MonoFloat;
        } else {
            if (bitsPerSample == 8) format_ = AudioFormat::Stereo8;
            else if (bitsPerSample == 16) format_ = AudioFormat::Stereo16;
            else format_ = AudioFormat::StereoFloat;
        }
        
        return true;
    }
    
    // Generate simple waveforms for testing
    void generateSineWave(float frequency, float duration, int sampleRate = 44100) {
        sampleRate_ = sampleRate;
        channels_ = 1;
        bitsPerSample_ = 16;
        format_ = AudioFormat::Mono16;
        
        int totalSamples = (int)(sampleRate * duration);
        data_.resize(totalSamples * 2);  // 16-bit = 2 bytes
        
        for (int i = 0; i < totalSamples; i++) {
            float t = (float)i / sampleRate;
            float sample = sinf(2.0f * 3.14159f * frequency * t);
            int16_t s16 = (int16_t)(sample * 32767);
            data_[i * 2] = s16 & 0xFF;
            data_[i * 2 + 1] = (s16 >> 8) & 0xFF;
        }
    }
    
    void generateWhiteNoise(float duration, int sampleRate = 44100) {
        sampleRate_ = sampleRate;
        channels_ = 1;
        bitsPerSample_ = 16;
        format_ = AudioFormat::Mono16;
        
        int totalSamples = (int)(sampleRate * duration);
        data_.resize(totalSamples * 2);
        
        for (int i = 0; i < totalSamples; i++) {
            float sample = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
            int16_t s16 = (int16_t)(sample * 32767);
            data_[i * 2] = s16 & 0xFF;
            data_[i * 2 + 1] = (s16 >> 8) & 0xFF;
        }
    }
    
private:
    std::string name_;
    std::vector<uint8_t> data_;
    int sampleRate_ = 44100;
    int channels_ = 1;
    int bitsPerSample_ = 16;
    AudioFormat format_ = AudioFormat::Mono16;
};

// ===== Audio Rolloff Mode =====
enum class AudioRolloff {
    Linear,
    Logarithmic,
    Custom
};

// ===== Audio Source Settings =====
struct AudioSourceSettings {
    // Playback
    float volume = 1.0f;
    float pitch = 1.0f;
    bool loop = false;
    bool playOnAwake = false;
    int priority = 128;  // 0 = highest, 256 = lowest
    
    // 3D Sound
    bool spatialize = true;
    float minDistance = 1.0f;    // Distance at full volume
    float maxDistance = 500.0f;  // Distance at which sound is inaudible
    AudioRolloff rolloff = AudioRolloff::Logarithmic;
    float rolloffFactor = 1.0f;
    
    // Doppler
    float dopplerLevel = 1.0f;
    
    // Spread
    float spread = 0.0f;  // 0 = point source, 360 = omnidirectional
    
    // Reverb
    float reverbZoneMix = 1.0f;
};

// ===== Audio Source State =====
enum class AudioState {
    Stopped,
    Playing,
    Paused
};

// ===== Audio Source =====
// Plays AudioClips in 3D space
class AudioSource {
public:
    AudioSource() : id_(nextId_++) {}
    
    uint32_t getId() const { return id_; }
    
    // Clip
    void setClip(std::shared_ptr<AudioClip> clip) { clip_ = clip; }
    AudioClip* getClip() { return clip_.get(); }
    const AudioClip* getClip() const { return clip_.get(); }
    
    // Settings
    void setSettings(const AudioSourceSettings& settings) { settings_ = settings; }
    AudioSourceSettings& getSettings() { return settings_; }
    const AudioSourceSettings& getSettings() const { return settings_; }
    
    // Transform
    void setPosition(const Vec3& pos) { position_ = pos; }
    Vec3 getPosition() const { return position_; }
    
    void setVelocity(const Vec3& vel) { velocity_ = vel; }
    Vec3 getVelocity() const { return velocity_; }
    
    // Playback control
    void play() {
        if (clip_) {
            state_ = AudioState::Playing;
            playbackPosition_ = 0;
        }
    }
    
    void pause() {
        if (state_ == AudioState::Playing) {
            state_ = AudioState::Paused;
        }
    }
    
    void unpause() {
        if (state_ == AudioState::Paused) {
            state_ = AudioState::Playing;
        }
    }
    
    void stop() {
        state_ = AudioState::Stopped;
        playbackPosition_ = 0;
    }
    
    AudioState getState() const { return state_; }
    bool isPlaying() const { return state_ == AudioState::Playing; }
    
    // Playback position
    float getTime() const {
        if (!clip_ || clip_->getSampleRate() == 0) return 0.0f;
        return (float)playbackPosition_ / clip_->getSampleRate();
    }
    
    void setTime(float time) {
        if (clip_) {
            playbackPosition_ = (size_t)(time * clip_->getSampleRate());
        }
    }
    
    // Volume/pitch shortcuts
    void setVolume(float v) { settings_.volume = std::max(0.0f, std::min(1.0f, v)); }
    float getVolume() const { return settings_.volume; }
    
    void setPitch(float p) { settings_.pitch = std::max(0.01f, std::min(3.0f, p)); }
    float getPitch() const { return settings_.pitch; }
    
    void setLoop(bool loop) { settings_.loop = loop; }
    bool isLooping() const { return settings_.loop; }
    
    // Internal
    size_t getPlaybackPosition() const { return playbackPosition_; }
    void advancePlayback(size_t samples) { playbackPosition_ += samples; }
    
    // For mixing - computed effective volume after distance attenuation
    float computedVolume = 0.0f;
    float computedPanL = 1.0f;
    float computedPanR = 1.0f;
    
private:
    uint32_t id_;
    static inline uint32_t nextId_ = 0;
    
    std::shared_ptr<AudioClip> clip_;
    AudioSourceSettings settings_;
    
    Vec3 position_ = {0, 0, 0};
    Vec3 velocity_ = {0, 0, 0};
    
    AudioState state_ = AudioState::Stopped;
    size_t playbackPosition_ = 0;
};

// ===== Audio Listener =====
// The "ears" - typically attached to camera
class AudioListener {
public:
    void setPosition(const Vec3& pos) { position_ = pos; }
    Vec3 getPosition() const { return position_; }
    
    void setVelocity(const Vec3& vel) { velocity_ = vel; }
    Vec3 getVelocity() const { return velocity_; }
    
    void setForward(const Vec3& fwd) { forward_ = fwd.normalized(); }
    Vec3 getForward() const { return forward_; }
    
    void setUp(const Vec3& up) { up_ = up.normalized(); }
    Vec3 getUp() const { return up_; }
    
    // Compute right vector
    Vec3 getRight() const { return forward_.cross(up_).normalized(); }
    
    void setVolume(float v) { volume_ = std::max(0.0f, std::min(1.0f, v)); }
    float getVolume() const { return volume_; }
    
private:
    Vec3 position_ = {0, 0, 0};
    Vec3 velocity_ = {0, 0, 0};
    Vec3 forward_ = {0, 0, -1};
    Vec3 up_ = {0, 1, 0};
    float volume_ = 1.0f;
};

// ===== Audio Mixer Group =====
struct AudioMixerGroup {
    std::string name = "Master";
    float volume = 1.0f;
    bool mute = false;
    bool solo = false;
    int parentIndex = -1;  // -1 = root
    
    // Effects (placeholder)
    bool lowPassEnabled = false;
    float lowPassCutoff = 22000.0f;
    bool reverbEnabled = false;
    float reverbMix = 0.0f;
};

// ===== Audio Mixer =====
class AudioMixer {
public:
    AudioMixer() {
        // Create default groups
        AudioMixerGroup master;
        master.name = "Master";
        groups_.push_back(master);
        
        AudioMixerGroup music;
        music.name = "Music";
        music.parentIndex = 0;
        groups_.push_back(music);
        
        AudioMixerGroup sfx;
        sfx.name = "SFX";
        sfx.parentIndex = 0;
        groups_.push_back(sfx);
        
        AudioMixerGroup ambient;
        ambient.name = "Ambient";
        ambient.parentIndex = 0;
        groups_.push_back(ambient);
        
        AudioMixerGroup ui;
        ui.name = "UI";
        ui.parentIndex = 0;
        groups_.push_back(ui);
    }
    
    size_t addGroup(const std::string& name, int parentIndex = 0) {
        AudioMixerGroup group;
        group.name = name;
        group.parentIndex = parentIndex;
        groups_.push_back(group);
        return groups_.size() - 1;
    }
    
    AudioMixerGroup* getGroup(size_t index) {
        return index < groups_.size() ? &groups_[index] : nullptr;
    }
    
    AudioMixerGroup* getGroupByName(const std::string& name) {
        for (auto& group : groups_) {
            if (group.name == name) return &group;
        }
        return nullptr;
    }
    
    // Get effective volume (including parent chain)
    float getEffectiveVolume(size_t groupIndex) const {
        if (groupIndex >= groups_.size()) return 1.0f;
        
        const auto& group = groups_[groupIndex];
        if (group.mute) return 0.0f;
        
        float vol = group.volume;
        if (group.parentIndex >= 0 && group.parentIndex < (int)groups_.size()) {
            vol *= getEffectiveVolume(group.parentIndex);
        }
        
        return vol;
    }
    
    const std::vector<AudioMixerGroup>& getGroups() const { return groups_; }
    std::vector<AudioMixerGroup>& getGroups() { return groups_; }
    
private:
    std::vector<AudioMixerGroup> groups_;
};

// ===== Audio System =====
class AudioSystem {
public:
    static AudioSystem& get() {
        static AudioSystem instance;
        return instance;
    }
    
    void initialize(int sampleRate = 44100, int channels = 2, int bufferSize = 4096) {
        sampleRate_ = sampleRate;
        channels_ = channels;
        bufferSize_ = bufferSize;
        mixBuffer_.resize(bufferSize * channels, 0.0f);
        initialized_ = true;
    }
    
    void shutdown() {
        sources_.clear();
        clips_.clear();
        initialized_ = false;
    }
    
    bool isInitialized() const { return initialized_; }
    int getSampleRate() const { return sampleRate_; }
    int getChannels() const { return channels_; }
    
    // Clip management
    std::shared_ptr<AudioClip> createClip(const std::string& name) {
        auto clip = std::make_shared<AudioClip>(name);
        clips_[name] = clip;
        return clip;
    }
    
    std::shared_ptr<AudioClip> getClip(const std::string& name) {
        auto it = clips_.find(name);
        return it != clips_.end() ? it->second : nullptr;
    }
    
    // Source management
    AudioSource* createSource() {
        sources_.push_back(std::make_unique<AudioSource>());
        return sources_.back().get();
    }
    
    void destroySource(AudioSource* source) {
        sources_.erase(
            std::remove_if(sources_.begin(), sources_.end(),
                [source](const auto& s) { return s.get() == source; }),
            sources_.end()
        );
    }
    
    const std::vector<std::unique_ptr<AudioSource>>& getSources() const { return sources_; }
    
    // Listener
    AudioListener& getListener() { return listener_; }
    const AudioListener& getListener() const { return listener_; }
    
    // Mixer
    AudioMixer& getMixer() { return mixer_; }
    const AudioMixer& getMixer() const { return mixer_; }
    
    // Master volume
    void setMasterVolume(float v) { masterVolume_ = std::max(0.0f, std::min(1.0f, v)); }
    float getMasterVolume() const { return masterVolume_; }
    
    // Mute
    void setMuted(bool muted) { muted_ = muted; }
    bool isMuted() const { return muted_; }
    
    // Pause all
    void pauseAll() {
        for (auto& source : sources_) {
            if (source->isPlaying()) {
                source->pause();
            }
        }
    }
    
    void unpauseAll() {
        for (auto& source : sources_) {
            if (source->getState() == AudioState::Paused) {
                source->unpause();
            }
        }
    }
    
    void stopAll() {
        for (auto& source : sources_) {
            source->stop();
        }
    }
    
    // Update (call each frame to update 3D audio calculations)
    void update(float dt) {
        if (!initialized_) return;
        
        // Update 3D calculations for each source
        for (auto& source : sources_) {
            if (!source->isPlaying()) continue;
            
            updateSource3D(*source);
        }
    }
    
    // Mix audio (call from audio callback)
    void mixAudio(float* outputBuffer, size_t frameCount) {
        if (!initialized_ || muted_) {
            std::fill(outputBuffer, outputBuffer + frameCount * channels_, 0.0f);
            return;
        }
        
        std::fill(mixBuffer_.begin(), mixBuffer_.end(), 0.0f);
        
        // Mix all playing sources
        for (auto& source : sources_) {
            if (!source->isPlaying()) continue;
            
            mixSource(*source, frameCount);
        }
        
        // Apply master volume and copy to output
        float vol = masterVolume_ * listener_.getVolume();
        for (size_t i = 0; i < frameCount * channels_; i++) {
            outputBuffer[i] = mixBuffer_[i] * vol;
        }
    }
    
    // Get active source count
    size_t getPlayingCount() const {
        size_t count = 0;
        for (const auto& source : sources_) {
            if (source->isPlaying()) count++;
        }
        return count;
    }
    
    // Play one-shot sound (creates temporary source)
    void playOneShot(std::shared_ptr<AudioClip> clip, const Vec3& position, float volume = 1.0f) {
        AudioSource* source = createSource();
        source->setClip(clip);
        source->setPosition(position);
        source->setVolume(volume);
        source->play();
        // Note: In real implementation, would clean up after playback
    }
    
private:
    AudioSystem() = default;
    
    void updateSource3D(AudioSource& source) {
        const auto& settings = source.getSettings();
        
        if (!settings.spatialize) {
            source.computedVolume = settings.volume;
            source.computedPanL = 1.0f;
            source.computedPanR = 1.0f;
            return;
        }
        
        Vec3 listenerPos = listener_.getPosition();
        Vec3 sourcePos = source.getPosition();
        Vec3 delta = sourcePos - listenerPos;
        float distance = delta.length();
        
        // Distance attenuation
        float attenuation = 1.0f;
        if (distance > settings.minDistance) {
            if (settings.rolloff == AudioRolloff::Linear) {
                attenuation = 1.0f - (distance - settings.minDistance) / 
                             (settings.maxDistance - settings.minDistance);
                attenuation = std::max(0.0f, attenuation);
            } else {
                // Logarithmic
                attenuation = settings.minDistance / 
                             (settings.minDistance + settings.rolloffFactor * 
                              (distance - settings.minDistance));
            }
        }
        
        source.computedVolume = settings.volume * attenuation;
        
        // Stereo panning
        if (distance > 0.001f) {
            Vec3 dir = delta * (1.0f / distance);
            Vec3 right = listener_.getRight();
            float pan = dir.dot(right);  // -1 to 1
            
            // Constant power panning
            float angle = (pan + 1.0f) * 0.25f * 3.14159f;  // 0 to PI/2
            source.computedPanL = cosf(angle);
            source.computedPanR = sinf(angle);
        } else {
            source.computedPanL = source.computedPanR = 0.707f;  // Equal power
        }
        
        // Doppler effect (simplified)
        if (settings.dopplerLevel > 0.0f) {
            Vec3 listenerVel = listener_.getVelocity();
            Vec3 sourceVel = source.getVelocity();
            
            if (distance > 0.001f) {
                Vec3 dir = delta * (1.0f / distance);
                float vListener = listenerVel.dot(dir);
                float vSource = sourceVel.dot(dir);
                
                const float speedOfSound = 343.0f;
                float dopplerShift = (speedOfSound + vListener * settings.dopplerLevel) /
                                    (speedOfSound + vSource * settings.dopplerLevel);
                dopplerShift = std::max(0.5f, std::min(2.0f, dopplerShift));
                // Would apply to pitch in real implementation
            }
        }
    }
    
    void mixSource(AudioSource& source, size_t frameCount) {
        AudioClip* clip = source.getClip();
        if (!clip || clip->getDataSize() == 0) return;
        
        size_t samplePos = source.getPlaybackPosition();
        size_t clipSamples = clip->getDataSize() / (clip->getBitsPerSample() / 8) / clip->getChannels();
        
        const auto& settings = source.getSettings();
        float volume = source.computedVolume;
        float panL = source.computedPanL;
        float panR = source.computedPanR;
        
        const uint8_t* data = clip->getData().data();
        int bytesPerSample = clip->getBitsPerSample() / 8;
        int clipChannels = clip->getChannels();
        
        for (size_t i = 0; i < frameCount; i++) {
            if (samplePos >= clipSamples) {
                if (settings.loop) {
                    samplePos = 0;
                } else {
                    source.stop();
                    break;
                }
            }
            
            // Read sample
            float sample = 0.0f;
            size_t byteOffset = samplePos * bytesPerSample * clipChannels;
            
            if (bytesPerSample == 2) {
                int16_t s16 = (int16_t)(data[byteOffset] | (data[byteOffset + 1] << 8));
                sample = s16 / 32768.0f;
            } else if (bytesPerSample == 1) {
                sample = (data[byteOffset] - 128) / 128.0f;
            }
            
            // Apply volume and panning
            sample *= volume;
            
            if (channels_ == 2) {
                mixBuffer_[i * 2] += sample * panL;
                mixBuffer_[i * 2 + 1] += sample * panR;
            } else {
                mixBuffer_[i] += sample;
            }
            
            samplePos++;
        }
        
        source.advancePlayback(frameCount);
    }
    
    bool initialized_ = false;
    int sampleRate_ = 44100;
    int channels_ = 2;
    int bufferSize_ = 4096;
    
    std::unordered_map<std::string, std::shared_ptr<AudioClip>> clips_;
    std::vector<std::unique_ptr<AudioSource>> sources_;
    AudioListener listener_;
    AudioMixer mixer_;
    
    std::vector<float> mixBuffer_;
    float masterVolume_ = 1.0f;
    bool muted_ = false;
};

// ===== Convenience Functions =====
inline AudioSystem& getAudioSystem() { return AudioSystem::get(); }

}  // namespace luma
