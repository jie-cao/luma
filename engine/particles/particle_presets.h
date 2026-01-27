// Particle Presets - Ready-to-use particle effects
// Fire, Smoke, Explosions, Magic, Rain, Snow, Sparks, etc.
#pragma once

#include "particle.h"
#include "particle_modules.h"

namespace luma {
namespace ParticlePresets {

// ===== Fire =====
inline ParticleEmitterSettings fire() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 50.0f;
    s.maxParticles = 500;
    s.looping = true;
    
    s.shape.shape = EmissionShape::Cone;
    s.shape.coneAngle = 15.0f;
    s.shape.coneRadius = 0.3f;
    s.shape.coneLength = 0.1f;
    
    s.startLife = {0.5f, 1.5f};
    s.startSpeed = {2.0f, 4.0f};
    s.startSize = {0.3f, 0.6f};
    s.endSize = {0.0f, 0.1f};
    
    // Orange to red to transparent
    s.startColor = {1.0f, 0.5f, 0.1f, 1.0f};
    s.endColor = {1.0f, 0.0f, 0.0f, 0.0f};
    
    s.gravityMultiplier = -0.5f;  // Rise up
    s.drag = 0.5f;
    
    return s;
}

// ===== Fire with Sparks =====
inline ParticleSystem* createFireWithSparks() {
    ParticleSystem* sys = getParticleManager().createSystem("Fire with Sparks");
    
    // Main fire emitter
    auto& fireEmitter = sys->addEmitter();
    fireEmitter.setSettings(fire());
    
    // Sparks emitter
    ParticleEmitterSettings sparkSettings;
    sparkSettings.emissionRate = 10.0f;
    sparkSettings.maxParticles = 100;
    
    sparkSettings.shape.shape = EmissionShape::Cone;
    sparkSettings.shape.coneAngle = 45.0f;
    
    sparkSettings.startLife = {1.0f, 2.0f};
    sparkSettings.startSpeed = {3.0f, 6.0f};
    sparkSettings.startSize = {0.02f, 0.05f};
    sparkSettings.endSize = {0.0f, 0.0f};
    
    sparkSettings.startColor = {1.0f, 0.8f, 0.2f, 1.0f};
    sparkSettings.endColor = {1.0f, 0.3f, 0.0f, 0.0f};
    
    sparkSettings.gravityMultiplier = 0.3f;
    sparkSettings.drag = 0.2f;
    
    auto& sparksEmitter = sys->addEmitter();
    sparksEmitter.setSettings(sparkSettings);
    
    return sys;
}

// ===== Smoke =====
inline ParticleEmitterSettings smoke() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 20.0f;
    s.maxParticles = 300;
    s.looping = true;
    
    s.shape.shape = EmissionShape::Sphere;
    s.shape.radius = 0.2f;
    s.shape.radiusThickness = 1.0f;
    
    s.startLife = {2.0f, 4.0f};
    s.startSpeed = {0.5f, 1.5f};
    s.startSize = {0.5f, 1.0f};
    s.endSize = {2.0f, 4.0f};
    
    // Gray smoke
    s.startColor = {0.4f, 0.4f, 0.4f, 0.6f};
    s.endColor = {0.3f, 0.3f, 0.3f, 0.0f};
    
    s.gravityMultiplier = -0.2f;
    s.drag = 0.3f;
    
    s.startRotation = {0.0f, 360.0f};
    s.angularVelocity = {-30.0f, 30.0f};
    
    return s;
}

// ===== Campfire Smoke =====
inline ParticleEmitterSettings campfireSmoke() {
    ParticleEmitterSettings s = smoke();
    
    s.emissionRate = 10.0f;
    s.startColor = {0.2f, 0.2f, 0.2f, 0.4f};
    s.startSpeed = {0.3f, 0.8f};
    s.startSize = {0.3f, 0.5f};
    s.endSize = {1.5f, 3.0f};
    
    return s;
}

// ===== Explosion =====
inline ParticleEmitterSettings explosion() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 0.0f;  // Burst only
    s.maxParticles = 200;
    s.looping = false;
    s.duration = 2.0f;
    
    // Single burst
    ParticleBurst burst;
    burst.time = 0.0f;
    burst.minCount = 100;
    burst.maxCount = 150;
    burst.cycles = 1;
    s.bursts.push_back(burst);
    
    s.shape.shape = EmissionShape::Sphere;
    s.shape.radius = 0.1f;
    s.shape.radiusThickness = 1.0f;
    
    s.startLife = {0.5f, 1.5f};
    s.startSpeed = {5.0f, 15.0f};
    s.startSize = {0.2f, 0.5f};
    s.endSize = {0.0f, 0.1f};
    
    // Bright orange/yellow
    s.startColor = {1.0f, 0.7f, 0.2f, 1.0f};
    s.endColor = {1.0f, 0.2f, 0.0f, 0.0f};
    
    s.gravityMultiplier = 0.5f;
    s.drag = 2.0f;
    
    return s;
}

// ===== Explosion with Smoke =====
inline ParticleSystem* createExplosion() {
    ParticleSystem* sys = getParticleManager().createSystem("Explosion");
    
    // Fire/debris burst
    auto& fireEmitter = sys->addEmitter();
    fireEmitter.setSettings(explosion());
    
    // Smoke cloud
    ParticleEmitterSettings smokeSettings;
    smokeSettings.emissionRate = 0.0f;
    smokeSettings.maxParticles = 50;
    smokeSettings.looping = false;
    smokeSettings.duration = 3.0f;
    
    ParticleBurst smokeBurst;
    smokeBurst.time = 0.1f;  // Slightly delayed
    smokeBurst.minCount = 30;
    smokeBurst.maxCount = 50;
    smokeBurst.cycles = 1;
    smokeSettings.bursts.push_back(smokeBurst);
    
    smokeSettings.shape.shape = EmissionShape::Sphere;
    smokeSettings.shape.radius = 0.5f;
    
    smokeSettings.startLife = {1.5f, 3.0f};
    smokeSettings.startSpeed = {2.0f, 5.0f};
    smokeSettings.startSize = {0.5f, 1.0f};
    smokeSettings.endSize = {3.0f, 5.0f};
    
    smokeSettings.startColor = {0.3f, 0.3f, 0.3f, 0.8f};
    smokeSettings.endColor = {0.2f, 0.2f, 0.2f, 0.0f};
    
    smokeSettings.gravityMultiplier = -0.1f;
    smokeSettings.drag = 1.0f;
    
    auto& smokeEmitter = sys->addEmitter();
    smokeEmitter.setSettings(smokeSettings);
    
    return sys;
}

// ===== Magic Sparkle =====
inline ParticleEmitterSettings magicSparkle() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 30.0f;
    s.maxParticles = 300;
    s.looping = true;
    
    s.shape.shape = EmissionShape::Sphere;
    s.shape.radius = 1.0f;
    s.shape.radiusThickness = 0.0f;  // Surface only
    
    s.startLife = {0.5f, 1.5f};
    s.startSpeed = {0.0f, 0.5f};
    s.startSize = {0.05f, 0.15f};
    s.endSize = {0.0f, 0.0f};
    
    // Purple/blue magic
    s.startColor = {0.6f, 0.3f, 1.0f, 1.0f};
    s.endColor = {0.3f, 0.6f, 1.0f, 0.0f};
    
    s.gravityMultiplier = 0.0f;
    s.drag = 0.0f;
    
    return s;
}

// ===== Magic Aura =====
inline ParticleEmitterSettings magicAura() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 15.0f;
    s.maxParticles = 100;
    s.looping = true;
    
    s.shape.shape = EmissionShape::Circle;
    s.shape.radius = 0.5f;
    
    s.startLife = {1.0f, 2.0f};
    s.startSpeed = {0.5f, 1.0f};
    s.startSize = {0.1f, 0.2f};
    s.endSize = {0.3f, 0.5f};
    
    s.startColor = {0.2f, 0.5f, 1.0f, 0.8f};
    s.endColor = {0.5f, 0.2f, 1.0f, 0.0f};
    
    s.gravityMultiplier = -0.1f;
    
    return s;
}

// ===== Magic Orb =====
inline ParticleSystem* createMagicOrb() {
    ParticleSystem* sys = getParticleManager().createSystem("Magic Orb");
    
    // Core glow
    auto& coreEmitter = sys->addEmitter();
    ParticleEmitterSettings coreSettings;
    coreSettings.emissionRate = 5.0f;
    coreSettings.maxParticles = 20;
    coreSettings.shape.shape = EmissionShape::Point;
    coreSettings.startLife = {0.5f, 1.0f};
    coreSettings.startSpeed = {0.0f, 0.0f};
    coreSettings.startSize = {0.5f, 0.8f};
    coreSettings.endSize = {0.3f, 0.5f};
    coreSettings.startColor = {0.8f, 0.9f, 1.0f, 0.5f};
    coreSettings.endColor = {0.5f, 0.7f, 1.0f, 0.0f};
    coreEmitter.setSettings(coreSettings);
    
    // Orbiting sparkles
    auto& sparkleEmitter = sys->addEmitter();
    sparkleEmitter.setSettings(magicSparkle());
    
    return sys;
}

// ===== Rain =====
inline ParticleEmitterSettings rain() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 500.0f;
    s.maxParticles = 5000;
    s.looping = true;
    
    s.shape.shape = EmissionShape::Box;
    s.shape.boxSize = {20.0f, 0.0f, 20.0f};
    
    s.startLife = {1.0f, 1.5f};
    s.startSpeed = {15.0f, 20.0f};
    s.startSize = {0.02f, 0.03f};
    s.endSize = {0.02f, 0.03f};
    
    // Blue-ish rain drops
    s.startColor = {0.7f, 0.8f, 0.9f, 0.8f};
    s.endColor = {0.7f, 0.8f, 0.9f, 0.6f};
    
    s.gravityMultiplier = 1.0f;
    s.gravity = {0.0f, -9.81f, 0.0f};
    
    s.stretchWithVelocity = true;
    s.velocityStretch = 0.5f;
    
    return s;
}

// ===== Heavy Rain =====
inline ParticleEmitterSettings heavyRain() {
    auto s = rain();
    s.emissionRate = 1000.0f;
    s.maxParticles = 10000;
    s.startSpeed = {20.0f, 25.0f};
    return s;
}

// ===== Snow =====
inline ParticleEmitterSettings snow() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 100.0f;
    s.maxParticles = 2000;
    s.looping = true;
    
    s.shape.shape = EmissionShape::Box;
    s.shape.boxSize = {20.0f, 0.0f, 20.0f};
    
    s.startLife = {5.0f, 8.0f};
    s.startSpeed = {0.5f, 1.5f};
    s.startSize = {0.03f, 0.08f};
    s.endSize = {0.03f, 0.08f};
    
    // White snowflakes
    s.startColor = {1.0f, 1.0f, 1.0f, 0.9f};
    s.endColor = {1.0f, 1.0f, 1.0f, 0.7f};
    
    s.gravityMultiplier = 0.1f;
    s.drag = 1.0f;
    
    s.startRotation = {0.0f, 360.0f};
    s.angularVelocity = {-60.0f, 60.0f};
    
    return s;
}

// ===== Blizzard =====
inline ParticleEmitterSettings blizzard() {
    auto s = snow();
    s.emissionRate = 500.0f;
    s.maxParticles = 5000;
    s.startSpeed = {2.0f, 5.0f};
    s.shape.directionalSpread = 0.5f;
    return s;
}

// ===== Sparks =====
inline ParticleEmitterSettings sparks() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 50.0f;
    s.maxParticles = 500;
    s.looping = true;
    
    s.shape.shape = EmissionShape::Hemisphere;
    s.shape.radius = 0.1f;
    
    s.startLife = {0.3f, 0.8f};
    s.startSpeed = {3.0f, 8.0f};
    s.startSize = {0.01f, 0.03f};
    s.endSize = {0.0f, 0.0f};
    
    // Bright yellow/orange
    s.startColor = {1.0f, 0.9f, 0.4f, 1.0f};
    s.endColor = {1.0f, 0.5f, 0.0f, 0.0f};
    
    s.gravityMultiplier = 1.0f;
    s.drag = 0.5f;
    
    return s;
}

// ===== Welding Sparks =====
inline ParticleEmitterSettings weldingSparks() {
    auto s = sparks();
    s.emissionRate = 200.0f;
    s.maxParticles = 1000;
    s.startSpeed = {5.0f, 12.0f};
    s.shape.coneAngle = 60.0f;
    return s;
}

// ===== Dust =====
inline ParticleEmitterSettings dust() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 10.0f;
    s.maxParticles = 200;
    s.looping = true;
    
    s.shape.shape = EmissionShape::Box;
    s.shape.boxSize = {5.0f, 3.0f, 5.0f};
    s.shape.randomizeDirection = true;
    
    s.startLife = {3.0f, 6.0f};
    s.startSpeed = {0.1f, 0.3f};
    s.startSize = {0.02f, 0.05f};
    s.endSize = {0.01f, 0.03f};
    
    // Brownish dust
    s.startColor = {0.6f, 0.5f, 0.4f, 0.3f};
    s.endColor = {0.6f, 0.5f, 0.4f, 0.0f};
    
    s.gravityMultiplier = 0.0f;
    
    return s;
}

// ===== Steam =====
inline ParticleEmitterSettings steam() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 30.0f;
    s.maxParticles = 300;
    s.looping = true;
    
    s.shape.shape = EmissionShape::Circle;
    s.shape.radius = 0.1f;
    
    s.startLife = {1.0f, 2.0f};
    s.startSpeed = {1.0f, 2.0f};
    s.startSize = {0.1f, 0.2f};
    s.endSize = {0.5f, 1.0f};
    
    // White steam
    s.startColor = {1.0f, 1.0f, 1.0f, 0.5f};
    s.endColor = {1.0f, 1.0f, 1.0f, 0.0f};
    
    s.gravityMultiplier = -0.3f;
    s.drag = 0.5f;
    
    return s;
}

// ===== Water Splash =====
inline ParticleEmitterSettings waterSplash() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 0.0f;
    s.maxParticles = 100;
    s.looping = false;
    s.duration = 1.0f;
    
    ParticleBurst burst;
    burst.time = 0.0f;
    burst.minCount = 50;
    burst.maxCount = 80;
    burst.cycles = 1;
    s.bursts.push_back(burst);
    
    s.shape.shape = EmissionShape::Hemisphere;
    s.shape.radius = 0.1f;
    
    s.startLife = {0.3f, 0.8f};
    s.startSpeed = {2.0f, 5.0f};
    s.startSize = {0.02f, 0.05f};
    s.endSize = {0.01f, 0.03f};
    
    s.startColor = {0.7f, 0.85f, 1.0f, 0.8f};
    s.endColor = {0.7f, 0.85f, 1.0f, 0.0f};
    
    s.gravityMultiplier = 1.0f;
    
    return s;
}

// ===== Blood Splash =====
inline ParticleEmitterSettings bloodSplash() {
    auto s = waterSplash();
    s.startColor = {0.5f, 0.0f, 0.0f, 1.0f};
    s.endColor = {0.3f, 0.0f, 0.0f, 0.0f};
    return s;
}

// ===== Leaves Falling =====
inline ParticleEmitterSettings fallingLeaves() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 5.0f;
    s.maxParticles = 100;
    s.looping = true;
    
    s.shape.shape = EmissionShape::Box;
    s.shape.boxSize = {10.0f, 0.0f, 10.0f};
    
    s.startLife = {3.0f, 5.0f};
    s.startSpeed = {0.1f, 0.5f};
    s.startSize = {0.1f, 0.2f};
    s.endSize = {0.1f, 0.2f};
    
    // Green/yellow/orange leaves
    s.startColor = {0.5f, 0.6f, 0.2f, 1.0f};
    s.endColor = {0.7f, 0.5f, 0.2f, 0.8f};
    
    s.gravityMultiplier = 0.2f;
    s.drag = 2.0f;
    
    s.startRotation = {0.0f, 360.0f};
    s.angularVelocity = {-90.0f, 90.0f};
    
    return s;
}

// ===== Fireflies =====
inline ParticleEmitterSettings fireflies() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 3.0f;
    s.maxParticles = 50;
    s.looping = true;
    
    s.shape.shape = EmissionShape::Box;
    s.shape.boxSize = {5.0f, 2.0f, 5.0f};
    s.shape.randomizeDirection = true;
    
    s.startLife = {2.0f, 5.0f};
    s.startSpeed = {0.2f, 0.5f};
    s.startSize = {0.03f, 0.06f};
    s.endSize = {0.02f, 0.04f};
    
    // Yellowish glow
    s.startColor = {0.8f, 1.0f, 0.3f, 0.0f};  // Start invisible
    s.endColor = {0.8f, 1.0f, 0.3f, 0.0f};    // End invisible
    
    s.gravityMultiplier = 0.0f;
    s.drag = 1.0f;
    
    return s;
}

// ===== Confetti =====
inline ParticleEmitterSettings confetti() {
    ParticleEmitterSettings s;
    
    s.emissionRate = 0.0f;
    s.maxParticles = 500;
    s.looping = false;
    s.duration = 5.0f;
    
    ParticleBurst burst;
    burst.time = 0.0f;
    burst.minCount = 200;
    burst.maxCount = 300;
    burst.cycles = 1;
    s.bursts.push_back(burst);
    
    s.shape.shape = EmissionShape::Cone;
    s.shape.coneAngle = 30.0f;
    
    s.startLife = {2.0f, 4.0f};
    s.startSpeed = {5.0f, 10.0f};
    s.startSize = {0.05f, 0.1f};
    s.endSize = {0.05f, 0.1f};
    
    // Colorful (would use random colors per particle ideally)
    s.startColor = {1.0f, 0.8f, 0.0f, 1.0f};
    s.endColor = {1.0f, 0.8f, 0.0f, 0.8f};
    
    s.gravityMultiplier = 0.5f;
    s.drag = 1.5f;
    
    s.startRotation = {0.0f, 360.0f};
    s.angularVelocity = {-180.0f, 180.0f};
    
    return s;
}

// ===== Portal Effect =====
inline ParticleSystem* createPortal() {
    ParticleSystem* sys = getParticleManager().createSystem("Portal");
    
    // Ring particles
    auto& ringEmitter = sys->addEmitter();
    ParticleEmitterSettings ringSettings;
    ringSettings.emissionRate = 50.0f;
    ringSettings.maxParticles = 200;
    ringSettings.shape.shape = EmissionShape::Circle;
    ringSettings.shape.radius = 1.0f;
    ringSettings.shape.arcAngle = 360.0f;
    ringSettings.shape.randomizeDirection = false;
    ringSettings.startLife = {1.0f, 2.0f};
    ringSettings.startSpeed = {0.0f, 0.0f};
    ringSettings.startSize = {0.1f, 0.2f};
    ringSettings.endSize = {0.0f, 0.0f};
    ringSettings.startColor = {0.3f, 0.5f, 1.0f, 1.0f};
    ringSettings.endColor = {0.8f, 0.3f, 1.0f, 0.0f};
    ringEmitter.setSettings(ringSettings);
    
    // Center vortex
    auto& vortexEmitter = sys->addEmitter();
    ParticleEmitterSettings vortexSettings;
    vortexSettings.emissionRate = 30.0f;
    vortexSettings.maxParticles = 100;
    vortexSettings.shape.shape = EmissionShape::Sphere;
    vortexSettings.shape.radius = 0.3f;
    vortexSettings.startLife = {0.5f, 1.0f};
    vortexSettings.startSpeed = {0.5f, 1.0f};
    vortexSettings.startSize = {0.2f, 0.4f};
    vortexSettings.endSize = {0.0f, 0.0f};
    vortexSettings.startColor = {0.5f, 0.2f, 1.0f, 0.8f};
    vortexSettings.endColor = {0.2f, 0.5f, 1.0f, 0.0f};
    vortexEmitter.setSettings(vortexSettings);
    
    return sys;
}

// ===== All Preset Names =====
inline std::vector<std::pair<std::string, std::string>> getAllPresetNames() {
    return {
        // Fire
        {"fire", "Fire"},
        {"campfire_smoke", "Campfire Smoke"},
        
        // Smoke
        {"smoke", "Smoke"},
        
        // Explosions
        {"explosion", "Explosion"},
        
        // Magic
        {"magic_sparkle", "Magic Sparkle"},
        {"magic_aura", "Magic Aura"},
        
        // Weather
        {"rain", "Rain"},
        {"heavy_rain", "Heavy Rain"},
        {"snow", "Snow"},
        {"blizzard", "Blizzard"},
        
        // Effects
        {"sparks", "Sparks"},
        {"welding_sparks", "Welding Sparks"},
        {"dust", "Dust"},
        {"steam", "Steam"},
        {"water_splash", "Water Splash"},
        {"blood_splash", "Blood Splash"},
        {"falling_leaves", "Falling Leaves"},
        {"fireflies", "Fireflies"},
        {"confetti", "Confetti"},
    };
}

// Get preset settings by name
inline ParticleEmitterSettings getPreset(const std::string& name) {
    if (name == "fire") return fire();
    if (name == "campfire_smoke") return campfireSmoke();
    if (name == "smoke") return smoke();
    if (name == "explosion") return explosion();
    if (name == "magic_sparkle") return magicSparkle();
    if (name == "magic_aura") return magicAura();
    if (name == "rain") return rain();
    if (name == "heavy_rain") return heavyRain();
    if (name == "snow") return snow();
    if (name == "blizzard") return blizzard();
    if (name == "sparks") return sparks();
    if (name == "welding_sparks") return weldingSparks();
    if (name == "dust") return dust();
    if (name == "steam") return steam();
    if (name == "water_splash") return waterSplash();
    if (name == "blood_splash") return bloodSplash();
    if (name == "falling_leaves") return fallingLeaves();
    if (name == "fireflies") return fireflies();
    if (name == "confetti") return confetti();
    
    // Default
    return fire();
}

}  // namespace ParticlePresets
}  // namespace luma
