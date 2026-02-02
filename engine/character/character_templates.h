// LUMA Character Templates Implementation
// Concrete implementations of ICharacterTemplate for different character types
#pragma once

#include "engine/character/character_template.h"
#include "engine/character/body_part_system.h"
#include "engine/character/cartoon_features.h"
#include "engine/character/base_human_loader.h"
#include <cmath>

namespace luma {

// ============================================================================
// Human Template - Realistic human characters
// ============================================================================

class HumanTemplate : public BaseCharacterTemplate {
public:
    HumanTemplate() 
        : BaseCharacterTemplate(CharacterType::Human, "Human", 
                               "Realistic human character with proper proportions") {}
    
    std::vector<std::string> getSupportedFeatures() const override {
        return {"face", "body", "hands", "feet", "hair", "clothing", 
                "blendshapes", "skeleton", "animation"};
    }
    
    CharacterParams getDefaultParams() const override {
        CharacterParams params;
        params.type = CharacterType::Human;
        params.proportions = BodyProportions::realistic();
        params.height = 1.8f;
        params.primaryColor = Vec3(0.9f, 0.75f, 0.65f);  // Skin tone
        params.hasEars = true;
        params.hasMouth = true;
        params.hasNose = true;
        return params;
    }
    
    BodyProportions getDefaultProportions() const override {
        return BodyProportions::realistic();
    }
    
    Skeleton createSkeleton(const CharacterParams& params) override {
        Skeleton skeleton;
        
        float height = params.height;
        
        // Root and spine
        int root = skeleton.addBone("root", -1);
        skeleton.setBoneLocalTransform(root, Vec3(0, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int hips = skeleton.addBone("hips", root);
        skeleton.setBoneLocalTransform(hips, Vec3(0, height * 0.5f, 0), Quat(), Vec3(1, 1, 1));
        
        int spine = skeleton.addBone("spine", hips);
        skeleton.setBoneLocalTransform(spine, Vec3(0, height * 0.1f, 0), Quat(), Vec3(1, 1, 1));
        
        int spine1 = skeleton.addBone("spine1", spine);
        skeleton.setBoneLocalTransform(spine1, Vec3(0, height * 0.1f, 0), Quat(), Vec3(1, 1, 1));
        
        int spine2 = skeleton.addBone("spine2", spine1);
        skeleton.setBoneLocalTransform(spine2, Vec3(0, height * 0.1f, 0), Quat(), Vec3(1, 1, 1));
        
        int neck = skeleton.addBone("neck", spine2);
        skeleton.setBoneLocalTransform(neck, Vec3(0, height * 0.08f, 0), Quat(), Vec3(1, 1, 1));
        
        int head = skeleton.addBone("head", neck);
        skeleton.setBoneLocalTransform(head, Vec3(0, height * 0.05f, 0), Quat(), Vec3(1, 1, 1));
        
        // Left arm
        int leftShoulder = skeleton.addBone("shoulder_L", spine2);
        skeleton.setBoneLocalTransform(leftShoulder, Vec3(-0.08f, height * 0.05f, 0), Quat(), Vec3(1, 1, 1));
        
        int leftArm = skeleton.addBone("arm_L", leftShoulder);
        skeleton.setBoneLocalTransform(leftArm, Vec3(-0.12f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int leftForearm = skeleton.addBone("forearm_L", leftArm);
        skeleton.setBoneLocalTransform(leftForearm, Vec3(-height * 0.15f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int leftHand = skeleton.addBone("hand_L", leftForearm);
        skeleton.setBoneLocalTransform(leftHand, Vec3(-height * 0.13f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        // Right arm
        int rightShoulder = skeleton.addBone("shoulder_R", spine2);
        skeleton.setBoneLocalTransform(rightShoulder, Vec3(0.08f, height * 0.05f, 0), Quat(), Vec3(1, 1, 1));
        
        int rightArm = skeleton.addBone("arm_R", rightShoulder);
        skeleton.setBoneLocalTransform(rightArm, Vec3(0.12f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int rightForearm = skeleton.addBone("forearm_R", rightArm);
        skeleton.setBoneLocalTransform(rightForearm, Vec3(height * 0.15f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int rightHand = skeleton.addBone("hand_R", rightForearm);
        skeleton.setBoneLocalTransform(rightHand, Vec3(height * 0.13f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        // Left leg
        int leftUpLeg = skeleton.addBone("upleg_L", hips);
        skeleton.setBoneLocalTransform(leftUpLeg, Vec3(-0.08f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int leftLeg = skeleton.addBone("leg_L", leftUpLeg);
        skeleton.setBoneLocalTransform(leftLeg, Vec3(0, -height * 0.23f, 0), Quat(), Vec3(1, 1, 1));
        
        int leftFoot = skeleton.addBone("foot_L", leftLeg);
        skeleton.setBoneLocalTransform(leftFoot, Vec3(0, -height * 0.23f, 0), Quat(), Vec3(1, 1, 1));
        
        // Right leg
        int rightUpLeg = skeleton.addBone("upleg_R", hips);
        skeleton.setBoneLocalTransform(rightUpLeg, Vec3(0.08f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int rightLeg = skeleton.addBone("leg_R", rightUpLeg);
        skeleton.setBoneLocalTransform(rightLeg, Vec3(0, -height * 0.23f, 0), Quat(), Vec3(1, 1, 1));
        
        int rightFoot = skeleton.addBone("foot_R", rightLeg);
        skeleton.setBoneLocalTransform(rightFoot, Vec3(0, -height * 0.23f, 0), Quat(), Vec3(1, 1, 1));
        
        return skeleton;
    }
    
    std::vector<std::string> getRequiredBones() const override {
        return {"root", "hips", "spine", "spine1", "spine2", "neck", "head",
                "shoulder_L", "arm_L", "forearm_L", "hand_L",
                "shoulder_R", "arm_R", "forearm_R", "hand_R",
                "upleg_L", "leg_L", "foot_L",
                "upleg_R", "leg_R", "foot_R"};
    }
    
    std::vector<std::string> getOptionalBones() const override {
        return {"eye_L", "eye_R", "jaw", "tongue",
                "thumb_L", "index_L", "middle_L", "ring_L", "pinky_L",
                "thumb_R", "index_R", "middle_R", "ring_R", "pinky_R",
                "toe_L", "toe_R"};
    }
    
    Mesh createBaseMesh(const CharacterParams& params) override {
        // Use existing procedural human generator
        ProceduralHumanGenerator::GeneratorParams genParams;
        genParams.height = params.height;
        genParams.bodySubdivisions = 16;
        genParams.heightSegments = 30;
        
        BaseHumanModel model = ProceduralHumanGenerator::generate(genParams);
        
        Mesh mesh;
        mesh.vertices = model.vertices;
        mesh.indices = model.indices;
        mesh.baseColor[0] = params.primaryColor.x;
        mesh.baseColor[1] = params.primaryColor.y;
        mesh.baseColor[2] = params.primaryColor.z;
        
        return mesh;
    }
    
    BlendShapeMesh createBlendShapes(const CharacterParams& params,
                                     const Mesh& baseMesh) override {
        BlendShapeMesh blendShapes;
        
        // Basic expression blend shapes would go here
        // For now, return empty
        
        return blendShapes;
    }
    
    std::vector<std::string> getAvailableExpressions() const override {
        return {"smile", "frown", "eyebrow_raise", "eyebrow_lower",
                "blink_L", "blink_R", "mouth_open", "jaw_open"};
    }
    
    std::vector<std::string> getCustomizableAttributes() const override {
        return {"height", "weight", "muscle", "age", "skinColor"};
    }
    
    void applyCustomization(CharacterCreationResult& result,
                           const std::string& attribute,
                           float value) override {
        // Customization logic would go here
    }
};

// ============================================================================
// Mascot Template - Hello Kitty style characters
// ============================================================================

class MascotTemplate : public BaseCharacterTemplate {
public:
    MascotTemplate()
        : BaseCharacterTemplate(CharacterType::Mascot, "Mascot",
                               "Cute mascot character (Hello Kitty, Sanrio style)") {}
    
    std::vector<std::string> getSupportedFeatures() const override {
        return {"face", "body", "ears", "accessories", "blendshapes", 
                "skeleton", "simple_animation"};
    }
    
    CharacterParams getDefaultParams() const override {
        CharacterParams params;
        params.type = CharacterType::Mascot;
        params.proportions = BodyProportions::mascot();
        params.height = 1.0f;
        params.primaryColor = Vec3(1.0f, 1.0f, 1.0f);  // White (Hello Kitty)
        params.accentColor = Vec3(1.0f, 0.3f, 0.4f);   // Pink/red bow
        params.hasEars = true;
        params.hasMouth = false;  // Hello Kitty has no mouth!
        params.hasNose = true;
        params.earStyle = 1;  // Cat ears
        return params;
    }
    
    BodyProportions getDefaultProportions() const override {
        return BodyProportions::mascot();
    }
    
    Skeleton createSkeleton(const CharacterParams& params) override {
        Skeleton skeleton;
        
        float scale = params.height;
        
        // Simple skeleton for mascot
        int root = skeleton.addBone("root", -1);
        skeleton.setBoneLocalTransform(root, Vec3(0, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int body = skeleton.addBone("body", root);
        skeleton.setBoneLocalTransform(body, Vec3(0, scale * 0.2f, 0), Quat(), Vec3(1, 1, 1));
        
        int head = skeleton.addBone("head", body);
        skeleton.setBoneLocalTransform(head, Vec3(0, scale * 0.3f, 0), Quat(), Vec3(1, 1, 1));
        
        // Ears (for wiggle animation)
        int earL = skeleton.addBone("ear_L", head);
        skeleton.setBoneLocalTransform(earL, Vec3(-scale * 0.15f, scale * 0.35f, 0), Quat(), Vec3(1, 1, 1));
        
        int earR = skeleton.addBone("ear_R", head);
        skeleton.setBoneLocalTransform(earR, Vec3(scale * 0.15f, scale * 0.35f, 0), Quat(), Vec3(1, 1, 1));
        
        // Bow (accessory)
        int bow = skeleton.addBone("bow", head);
        skeleton.setBoneLocalTransform(bow, Vec3(-scale * 0.2f, scale * 0.35f, 0.02f), Quat(), Vec3(1, 1, 1));
        
        // Simple arms
        int armL = skeleton.addBone("arm_L", body);
        skeleton.setBoneLocalTransform(armL, Vec3(-scale * 0.15f, scale * 0.1f, 0), Quat(), Vec3(1, 1, 1));
        
        int armR = skeleton.addBone("arm_R", body);
        skeleton.setBoneLocalTransform(armR, Vec3(scale * 0.15f, scale * 0.1f, 0), Quat(), Vec3(1, 1, 1));
        
        // Simple legs
        int legL = skeleton.addBone("leg_L", body);
        skeleton.setBoneLocalTransform(legL, Vec3(-scale * 0.06f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int legR = skeleton.addBone("leg_R", body);
        skeleton.setBoneLocalTransform(legR, Vec3(scale * 0.06f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        return skeleton;
    }
    
    std::vector<std::string> getRequiredBones() const override {
        return {"root", "body", "head"};
    }
    
    std::vector<std::string> getOptionalBones() const override {
        return {"ear_L", "ear_R", "bow", "arm_L", "arm_R", "leg_L", "leg_R", "tail"};
    }
    
    Mesh createBaseMesh(const CharacterParams& params) override {
        BodyPartAssembly assembly;
        float scale = params.height;
        
        // Head (large, round) - main feature of mascot
        BodyPartDef head;
        head.id = "head";
        head.type = BodyPartType::Head;
        head.shape = PartShape::Ellipsoid;
        head.size = Vec3(scale * 0.5f, scale * 0.45f, scale * 0.4f);
        head.offset = Vec3(0, scale * 0.5f, 0);
        head.color = params.primaryColor;
        head.segments = 24;
        head.createBone = true;
        head.boneName = "head";
        assembly.addPart(head);
        
        // Body (small, oval)
        BodyPartDef body;
        body.id = "body";
        body.type = BodyPartType::Torso;
        body.shape = PartShape::Ellipsoid;
        body.size = Vec3(scale * 0.25f, scale * 0.2f, scale * 0.2f);
        body.offset = Vec3(0, scale * 0.15f, 0);
        body.color = params.primaryColor;
        body.segments = 16;
        body.parentPartId = "head";
        assembly.addPart(body);
        
        // Cat ears
        if (params.hasEars) {
            CartoonEarParams earParams;
            earParams.style = CartoonEarStyle::CatPointed;
            earParams.width = scale * 0.12f;
            earParams.height = scale * 0.15f;
            earParams.outerColor = params.primaryColor;
            earParams.innerColor = Vec3(1.0f, 0.8f, 0.8f);  // Pink inner
            
            BodyPartDef leftEar = CartoonFeatureGenerator::createEar(earParams, true);
            leftEar.offset = Vec3(-scale * 0.15f, scale * 0.8f, 0);
            leftEar.parentPartId = "head";
            assembly.addPart(leftEar);
            
            BodyPartDef rightEar = CartoonFeatureGenerator::createEar(earParams, false);
            rightEar.offset = Vec3(scale * 0.15f, scale * 0.8f, 0);
            rightEar.parentPartId = "head";
            assembly.addPart(rightEar);
        }
        
        // Eyes (simple dots)
        CartoonEyeParams eyeParams;
        eyeParams.style = CartoonEyeStyle::Dot;
        eyeParams.width = scale * 0.03f;
        eyeParams.height = scale * 0.04f;
        eyeParams.irisColor = Vec3(0, 0, 0);  // Black eyes
        eyeParams.hasOutline = false;
        eyeParams.hasHighlight = false;
        
        BodyPartDef leftEye = CartoonFeatureGenerator::createEye(eyeParams, true);
        leftEye.offset = Vec3(-scale * 0.08f, scale * 0.55f, scale * 0.18f);
        leftEye.parentPartId = "head";
        assembly.addPart(leftEye);
        
        BodyPartDef rightEye = CartoonFeatureGenerator::createEye(eyeParams, false);
        rightEye.offset = Vec3(scale * 0.08f, scale * 0.55f, scale * 0.18f);
        rightEye.parentPartId = "head";
        assembly.addPart(rightEye);
        
        // Nose (small yellow oval - Hello Kitty style)
        if (params.hasNose) {
            CartoonNoseParams noseParams;
            noseParams.style = CartoonNoseStyle::Animal;
            noseParams.width = scale * 0.03f;
            noseParams.height = scale * 0.02f;
            noseParams.color = Vec3(1.0f, 0.9f, 0.3f);  // Yellow
            
            BodyPartDef nose = CartoonFeatureGenerator::createNose(noseParams);
            nose.offset = Vec3(0, scale * 0.48f, scale * 0.2f);
            nose.parentPartId = "head";
            assembly.addPart(nose);
        }
        
        // Whiskers (6 lines) - would be added as additional geometry
        // Simplified: just skip for now
        
        // Bow (signature Hello Kitty accessory)
        AccessoryParams bowParams;
        bowParams.type = AccessoryType::Bow;
        bowParams.primaryColor = params.accentColor;
        bowParams.secondaryColor = Vec3(1.0f, 1.0f, 0.3f);  // Yellow center
        bowParams.size = scale * 0.12f;
        bowParams.position = Vec3(-scale * 0.2f, scale * 0.75f, scale * 0.05f);
        
        BodyPartDef bow = CartoonFeatureGenerator::createAccessory(bowParams);
        bow.parentPartId = "head";
        assembly.addPart(bow);
        
        // Simple arms (tiny stubs)
        BodyPartDef leftArm;
        leftArm.id = "left_arm";
        leftArm.type = BodyPartType::LeftArm;
        leftArm.shape = PartShape::Capsule;
        leftArm.size = Vec3(scale * 0.08f, scale * 0.1f, scale * 0.08f);
        leftArm.offset = Vec3(-scale * 0.18f, scale * 0.12f, 0);
        leftArm.color = params.primaryColor;
        leftArm.segments = 8;
        leftArm.parentPartId = "body";
        assembly.addPart(leftArm);
        
        BodyPartDef rightArm;
        rightArm.id = "right_arm";
        rightArm.type = BodyPartType::RightArm;
        rightArm.shape = PartShape::Capsule;
        rightArm.size = Vec3(scale * 0.08f, scale * 0.1f, scale * 0.08f);
        rightArm.offset = Vec3(scale * 0.18f, scale * 0.12f, 0);
        rightArm.color = params.primaryColor;
        rightArm.segments = 8;
        rightArm.parentPartId = "body";
        assembly.addPart(rightArm);
        
        // Simple legs (tiny stubs)
        BodyPartDef leftLeg;
        leftLeg.id = "left_leg";
        leftLeg.type = BodyPartType::LeftLeg;
        leftLeg.shape = PartShape::Capsule;
        leftLeg.size = Vec3(scale * 0.06f, scale * 0.08f, scale * 0.06f);
        leftLeg.offset = Vec3(-scale * 0.08f, scale * 0.02f, 0);
        leftLeg.color = params.primaryColor;
        leftLeg.segments = 8;
        leftLeg.parentPartId = "body";
        assembly.addPart(leftLeg);
        
        BodyPartDef rightLeg;
        rightLeg.id = "right_leg";
        rightLeg.type = BodyPartType::RightLeg;
        rightLeg.shape = PartShape::Capsule;
        rightLeg.size = Vec3(scale * 0.06f, scale * 0.08f, scale * 0.06f);
        rightLeg.offset = Vec3(scale * 0.08f, scale * 0.02f, 0);
        rightLeg.color = params.primaryColor;
        rightLeg.segments = 8;
        rightLeg.parentPartId = "body";
        assembly.addPart(rightLeg);
        
        // Generate all parts
        assembly.generateAll();
        
        // Combine into single mesh
        Mesh mesh = assembly.combineMesh();
        mesh.baseColor[0] = params.primaryColor.x;
        mesh.baseColor[1] = params.primaryColor.y;
        mesh.baseColor[2] = params.primaryColor.z;
        
        return mesh;
    }
    
    BlendShapeMesh createBlendShapes(const CharacterParams& params,
                                     const Mesh& baseMesh) override {
        BlendShapeMesh blendShapes;
        
        // Simple expressions for mascot
        // Blink, head tilt, ear wiggle
        
        return blendShapes;
    }
    
    std::vector<std::string> getAvailableExpressions() const override {
        return {"blink", "head_tilt", "ear_wiggle_L", "ear_wiggle_R", "surprised"};
    }
    
    std::vector<std::string> getCustomizableAttributes() const override {
        return {"size", "bodyColor", "earColor", "bowColor", "bowPosition"};
    }
    
    void applyCustomization(CharacterCreationResult& result,
                           const std::string& attribute,
                           float value) override {
        // Customization logic
    }
};

// ============================================================================
// Cartoon Template - Mickey Mouse style characters
// ============================================================================

class CartoonTemplate : public BaseCharacterTemplate {
public:
    CartoonTemplate()
        : BaseCharacterTemplate(CharacterType::Cartoon, "Cartoon",
                               "Classic cartoon character (Mickey, Disney style)") {}
    
    std::vector<std::string> getSupportedFeatures() const override {
        return {"face", "body", "ears", "hands", "feet", "tail",
                "blendshapes", "skeleton", "animation", "squash_stretch"};
    }
    
    CharacterParams getDefaultParams() const override {
        CharacterParams params;
        params.type = CharacterType::Cartoon;
        params.proportions = BodyProportions::cartoon();
        params.height = 1.2f;
        params.primaryColor = Vec3(0.1f, 0.1f, 0.1f);  // Black (Mickey)
        params.secondaryColor = Vec3(0.95f, 0.85f, 0.75f);  // Skin tone for face
        params.accentColor = Vec3(1.0f, 0.2f, 0.2f);   // Red (shorts)
        params.hasEars = true;
        params.hasMouth = true;
        params.hasNose = true;
        params.hasTail = true;
        params.earStyle = 0;  // Mouse ears
        return params;
    }
    
    BodyProportions getDefaultProportions() const override {
        return BodyProportions::cartoon();
    }
    
    Skeleton createSkeleton(const CharacterParams& params) override {
        Skeleton skeleton;
        
        float scale = params.height;
        
        // Root
        int root = skeleton.addBone("root", -1);
        skeleton.setBoneLocalTransform(root, Vec3(0, 0, 0), Quat(), Vec3(1, 1, 1));
        
        // Body chain
        int hips = skeleton.addBone("hips", root);
        skeleton.setBoneLocalTransform(hips, Vec3(0, scale * 0.35f, 0), Quat(), Vec3(1, 1, 1));
        
        int spine = skeleton.addBone("spine", hips);
        skeleton.setBoneLocalTransform(spine, Vec3(0, scale * 0.1f, 0), Quat(), Vec3(1, 1, 1));
        
        int chest = skeleton.addBone("chest", spine);
        skeleton.setBoneLocalTransform(chest, Vec3(0, scale * 0.1f, 0), Quat(), Vec3(1, 1, 1));
        
        int neck = skeleton.addBone("neck", chest);
        skeleton.setBoneLocalTransform(neck, Vec3(0, scale * 0.08f, 0), Quat(), Vec3(1, 1, 1));
        
        int head = skeleton.addBone("head", neck);
        skeleton.setBoneLocalTransform(head, Vec3(0, scale * 0.1f, 0), Quat(), Vec3(1, 1, 1));
        
        // Ears (large round ears)
        int earL = skeleton.addBone("ear_L", head);
        skeleton.setBoneLocalTransform(earL, Vec3(-scale * 0.15f, scale * 0.2f, 0), Quat(), Vec3(1, 1, 1));
        
        int earR = skeleton.addBone("ear_R", head);
        skeleton.setBoneLocalTransform(earR, Vec3(scale * 0.15f, scale * 0.2f, 0), Quat(), Vec3(1, 1, 1));
        
        // Arms with gloved hands
        int shoulderL = skeleton.addBone("shoulder_L", chest);
        skeleton.setBoneLocalTransform(shoulderL, Vec3(-scale * 0.1f, scale * 0.05f, 0), Quat(), Vec3(1, 1, 1));
        
        int armL = skeleton.addBone("arm_L", shoulderL);
        skeleton.setBoneLocalTransform(armL, Vec3(-scale * 0.1f, -scale * 0.02f, 0), Quat(), Vec3(1, 1, 1));
        
        int forearmL = skeleton.addBone("forearm_L", armL);
        skeleton.setBoneLocalTransform(forearmL, Vec3(-scale * 0.12f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int handL = skeleton.addBone("hand_L", forearmL);
        skeleton.setBoneLocalTransform(handL, Vec3(-scale * 0.08f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int shoulderR = skeleton.addBone("shoulder_R", chest);
        skeleton.setBoneLocalTransform(shoulderR, Vec3(scale * 0.1f, scale * 0.05f, 0), Quat(), Vec3(1, 1, 1));
        
        int armR = skeleton.addBone("arm_R", shoulderR);
        skeleton.setBoneLocalTransform(armR, Vec3(scale * 0.1f, -scale * 0.02f, 0), Quat(), Vec3(1, 1, 1));
        
        int forearmR = skeleton.addBone("forearm_R", armR);
        skeleton.setBoneLocalTransform(forearmR, Vec3(scale * 0.12f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int handR = skeleton.addBone("hand_R", forearmR);
        skeleton.setBoneLocalTransform(handR, Vec3(scale * 0.08f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        // Legs with large shoes
        int uplegL = skeleton.addBone("upleg_L", hips);
        skeleton.setBoneLocalTransform(uplegL, Vec3(-scale * 0.06f, -scale * 0.02f, 0), Quat(), Vec3(1, 1, 1));
        
        int legL = skeleton.addBone("leg_L", uplegL);
        skeleton.setBoneLocalTransform(legL, Vec3(0, -scale * 0.15f, 0), Quat(), Vec3(1, 1, 1));
        
        int footL = skeleton.addBone("foot_L", legL);
        skeleton.setBoneLocalTransform(footL, Vec3(0, -scale * 0.12f, scale * 0.02f), Quat(), Vec3(1, 1, 1));
        
        int uplegR = skeleton.addBone("upleg_R", hips);
        skeleton.setBoneLocalTransform(uplegR, Vec3(scale * 0.06f, -scale * 0.02f, 0), Quat(), Vec3(1, 1, 1));
        
        int legR = skeleton.addBone("leg_R", uplegR);
        skeleton.setBoneLocalTransform(legR, Vec3(0, -scale * 0.15f, 0), Quat(), Vec3(1, 1, 1));
        
        int footR = skeleton.addBone("foot_R", legR);
        skeleton.setBoneLocalTransform(footR, Vec3(0, -scale * 0.12f, scale * 0.02f), Quat(), Vec3(1, 1, 1));
        
        // Tail
        if (params.hasTail) {
            int tail = skeleton.addBone("tail", hips);
            skeleton.setBoneLocalTransform(tail, Vec3(0, 0, -scale * 0.08f), Quat(), Vec3(1, 1, 1));
        }
        
        return skeleton;
    }
    
    std::vector<std::string> getRequiredBones() const override {
        return {"root", "hips", "spine", "chest", "neck", "head",
                "shoulder_L", "arm_L", "forearm_L", "hand_L",
                "shoulder_R", "arm_R", "forearm_R", "hand_R",
                "upleg_L", "leg_L", "foot_L",
                "upleg_R", "leg_R", "foot_R"};
    }
    
    std::vector<std::string> getOptionalBones() const override {
        return {"ear_L", "ear_R", "tail", "jaw", "tongue"};
    }
    
    Mesh createBaseMesh(const CharacterParams& params) override {
        BodyPartAssembly assembly;
        float scale = params.height;
        
        // Head (larger than realistic proportions)
        BodyPartDef head;
        head.id = "head";
        head.type = BodyPartType::Head;
        head.shape = PartShape::Ellipsoid;
        head.size = Vec3(scale * 0.28f, scale * 0.3f, scale * 0.25f);
        head.offset = Vec3(0, scale * 0.75f, 0);
        head.color = params.primaryColor;
        head.segments = 24;
        assembly.addPart(head);
        
        // Face area (lighter color)
        BodyPartDef face;
        face.id = "face";
        face.shape = PartShape::Ellipsoid;
        face.size = Vec3(scale * 0.18f, scale * 0.2f, scale * 0.15f);
        face.offset = Vec3(0, scale * 0.72f, scale * 0.08f);
        face.color = params.secondaryColor;
        face.segments = 20;
        face.parentPartId = "head";
        assembly.addPart(face);
        
        // Large round ears (Mickey style)
        if (params.hasEars) {
            CartoonEarParams earParams;
            earParams.style = CartoonEarStyle::MouseRound;
            earParams.width = scale * 0.15f;
            earParams.outerColor = params.primaryColor;
            earParams.innerColor = params.primaryColor;
            
            BodyPartDef leftEar = CartoonFeatureGenerator::createEar(earParams, true);
            leftEar.offset = Vec3(-scale * 0.2f, scale * 0.95f, -scale * 0.05f);
            leftEar.parentPartId = "head";
            assembly.addPart(leftEar);
            
            BodyPartDef rightEar = CartoonFeatureGenerator::createEar(earParams, false);
            rightEar.offset = Vec3(scale * 0.2f, scale * 0.95f, -scale * 0.05f);
            rightEar.parentPartId = "head";
            assembly.addPart(rightEar);
        }
        
        // Eyes (large, expressive)
        CartoonEyeParams eyeParams;
        eyeParams.style = CartoonEyeStyle::Oval;
        eyeParams.width = scale * 0.04f;
        eyeParams.height = scale * 0.06f;
        eyeParams.scleraColor = Vec3(1, 1, 1);
        eyeParams.irisColor = Vec3(0, 0, 0);
        eyeParams.hasHighlight = true;
        
        BodyPartDef leftEye = CartoonFeatureGenerator::createEye(eyeParams, true);
        leftEye.offset = Vec3(-scale * 0.06f, scale * 0.78f, scale * 0.15f);
        leftEye.parentPartId = "head";
        assembly.addPart(leftEye);
        
        BodyPartDef rightEye = CartoonFeatureGenerator::createEye(eyeParams, false);
        rightEye.offset = Vec3(scale * 0.06f, scale * 0.78f, scale * 0.15f);
        rightEye.parentPartId = "head";
        assembly.addPart(rightEye);
        
        // Button nose (black)
        CartoonNoseParams noseParams;
        noseParams.style = CartoonNoseStyle::Button;
        noseParams.width = scale * 0.04f;
        noseParams.height = scale * 0.03f;
        noseParams.color = Vec3(0, 0, 0);
        
        BodyPartDef nose = CartoonFeatureGenerator::createNose(noseParams);
        nose.offset = Vec3(0, scale * 0.68f, scale * 0.2f);
        nose.parentPartId = "head";
        assembly.addPart(nose);
        
        // Smiling mouth
        if (params.hasMouth) {
            CartoonMouthParams mouthParams;
            mouthParams.style = CartoonMouthStyle::Smile;
            mouthParams.width = scale * 0.1f;
            mouthParams.height = scale * 0.02f;
            mouthParams.smileAmount = 0.5f;
            mouthParams.lipColor = Vec3(0, 0, 0);
            
            BodyPartDef mouth = CartoonFeatureGenerator::createMouth(mouthParams);
            mouth.offset = Vec3(0, scale * 0.62f, scale * 0.15f);
            mouth.parentPartId = "head";
            assembly.addPart(mouth);
        }
        
        // Body (torso)
        BodyPartDef torso;
        torso.id = "torso";
        torso.type = BodyPartType::Torso;
        torso.shape = PartShape::Ellipsoid;
        torso.size = Vec3(scale * 0.18f, scale * 0.2f, scale * 0.15f);
        torso.offset = Vec3(0, scale * 0.45f, 0);
        torso.color = params.primaryColor;
        torso.segments = 16;
        assembly.addPart(torso);
        
        // Shorts/pants area (red for Mickey)
        BodyPartDef shorts;
        shorts.id = "shorts";
        shorts.shape = PartShape::Ellipsoid;
        shorts.size = Vec3(scale * 0.15f, scale * 0.1f, scale * 0.12f);
        shorts.offset = Vec3(0, scale * 0.32f, 0);
        shorts.color = params.accentColor;
        shorts.segments = 12;
        shorts.parentPartId = "torso";
        assembly.addPart(shorts);
        
        // Arms (thin, noodle-like)
        BodyPartDef leftArm;
        leftArm.id = "left_arm";
        leftArm.type = BodyPartType::LeftArm;
        leftArm.shape = PartShape::Capsule;
        leftArm.size = Vec3(scale * 0.04f, scale * 0.2f, scale * 0.04f);
        leftArm.offset = Vec3(-scale * 0.2f, scale * 0.5f, 0);
        leftArm.rotation = Quat::fromAxisAngle(Vec3(0, 0, 1), 0.3f);
        leftArm.color = params.primaryColor;
        leftArm.segments = 8;
        assembly.addPart(leftArm);
        
        BodyPartDef rightArm;
        rightArm.id = "right_arm";
        rightArm.type = BodyPartType::RightArm;
        rightArm.shape = PartShape::Capsule;
        rightArm.size = Vec3(scale * 0.04f, scale * 0.2f, scale * 0.04f);
        rightArm.offset = Vec3(scale * 0.2f, scale * 0.5f, 0);
        rightArm.rotation = Quat::fromAxisAngle(Vec3(0, 0, 1), -0.3f);
        rightArm.color = params.primaryColor;
        rightArm.segments = 8;
        assembly.addPart(rightArm);
        
        // Gloved hands (white, large)
        BodyPartDef leftHand;
        leftHand.id = "left_hand";
        leftHand.type = BodyPartType::LeftHand;
        leftHand.shape = PartShape::Sphere;
        leftHand.size = Vec3(scale * 0.08f, scale * 0.08f, scale * 0.06f);
        leftHand.offset = Vec3(-scale * 0.32f, scale * 0.42f, 0);
        leftHand.color = Vec3(1, 1, 1);  // White gloves
        leftHand.segments = 12;
        leftHand.parentPartId = "left_arm";
        assembly.addPart(leftHand);
        
        BodyPartDef rightHand;
        rightHand.id = "right_hand";
        rightHand.type = BodyPartType::RightHand;
        rightHand.shape = PartShape::Sphere;
        rightHand.size = Vec3(scale * 0.08f, scale * 0.08f, scale * 0.06f);
        rightHand.offset = Vec3(scale * 0.32f, scale * 0.42f, 0);
        rightHand.color = Vec3(1, 1, 1);
        rightHand.segments = 12;
        rightHand.parentPartId = "right_arm";
        assembly.addPart(rightHand);
        
        // Legs
        BodyPartDef leftLeg;
        leftLeg.id = "left_leg";
        leftLeg.type = BodyPartType::LeftLeg;
        leftLeg.shape = PartShape::Capsule;
        leftLeg.size = Vec3(scale * 0.05f, scale * 0.18f, scale * 0.05f);
        leftLeg.offset = Vec3(-scale * 0.06f, scale * 0.15f, 0);
        leftLeg.color = params.primaryColor;
        leftLeg.segments = 8;
        assembly.addPart(leftLeg);
        
        BodyPartDef rightLeg;
        rightLeg.id = "right_leg";
        rightLeg.type = BodyPartType::RightLeg;
        rightLeg.shape = PartShape::Capsule;
        rightLeg.size = Vec3(scale * 0.05f, scale * 0.18f, scale * 0.05f);
        rightLeg.offset = Vec3(scale * 0.06f, scale * 0.15f, 0);
        rightLeg.color = params.primaryColor;
        rightLeg.segments = 8;
        assembly.addPart(rightLeg);
        
        // Large shoes (yellow for Mickey)
        BodyPartDef leftFoot;
        leftFoot.id = "left_foot";
        leftFoot.type = BodyPartType::LeftFoot;
        leftFoot.shape = PartShape::Ellipsoid;
        leftFoot.size = Vec3(scale * 0.08f, scale * 0.05f, scale * 0.12f);
        leftFoot.offset = Vec3(-scale * 0.06f, scale * 0.02f, scale * 0.03f);
        leftFoot.color = Vec3(1.0f, 0.85f, 0.2f);  // Yellow shoes
        leftFoot.segments = 10;
        leftFoot.parentPartId = "left_leg";
        assembly.addPart(leftFoot);
        
        BodyPartDef rightFoot;
        rightFoot.id = "right_foot";
        rightFoot.type = BodyPartType::RightFoot;
        rightFoot.shape = PartShape::Ellipsoid;
        rightFoot.size = Vec3(scale * 0.08f, scale * 0.05f, scale * 0.12f);
        rightFoot.offset = Vec3(scale * 0.06f, scale * 0.02f, scale * 0.03f);
        rightFoot.color = Vec3(1.0f, 0.85f, 0.2f);
        rightFoot.segments = 10;
        rightFoot.parentPartId = "right_leg";
        assembly.addPart(rightFoot);
        
        // Tail
        if (params.hasTail) {
            BodyPartDef tail = CartoonFeatureGenerator::createTail(
                scale * 0.2f, scale * 0.015f, params.primaryColor);
            tail.offset = Vec3(0, scale * 0.3f, -scale * 0.1f);
            tail.parentPartId = "torso";
            assembly.addPart(tail);
        }
        
        // Generate all parts
        assembly.generateAll();
        
        // Combine into single mesh
        Mesh mesh = assembly.combineMesh();
        mesh.baseColor[0] = params.primaryColor.x;
        mesh.baseColor[1] = params.primaryColor.y;
        mesh.baseColor[2] = params.primaryColor.z;
        
        return mesh;
    }
    
    BlendShapeMesh createBlendShapes(const CharacterParams& params,
                                     const Mesh& baseMesh) override {
        BlendShapeMesh blendShapes;
        
        // Cartoon expressions
        // Would include squash/stretch, smear frames, etc.
        
        return blendShapes;
    }
    
    std::vector<std::string> getAvailableExpressions() const override {
        return {"smile", "surprised", "angry", "sad", "laugh",
                "blink", "wink_L", "wink_R", "squash", "stretch"};
    }
    
    std::vector<std::string> getCustomizableAttributes() const override {
        return {"size", "bodyColor", "faceColor", "earSize", "shortsColor", "shoeColor"};
    }
    
    void applyCustomization(CharacterCreationResult& result,
                           const std::string& attribute,
                           float value) override {
        // Customization logic
    }
};

// ============================================================================
// Template Registration
// ============================================================================

inline void registerDefaultTemplates() {
    auto& registry = getTemplateRegistry();
    
    registry.registerTemplate(std::make_shared<HumanTemplate>());
    registry.registerTemplate(std::make_shared<MascotTemplate>());
    registry.registerTemplate(std::make_shared<CartoonTemplate>());
}

}  // namespace luma
