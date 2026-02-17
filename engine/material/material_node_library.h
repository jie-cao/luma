// LUMA Material Node Library
// Complete library of material nodes for the node-based material editor
// Provides ~60 node types across 9 categories

#pragma once

#include "engine/material/material_node.h"
#include <vector>
#include <algorithm>

namespace luma {

class MaterialNodeLibrary {
public:
    static MaterialNodeLibrary& instance() {
        static MaterialNodeLibrary lib;
        return lib;
    }
    
    const std::vector<MaterialNodeDef>& getAllNodes() const { return defs_; }
    
    const MaterialNodeDef* findByType(const std::string& typeName) const {
        for (const auto& d : defs_) {
            if (d.typeName == typeName) return &d;
        }
        return nullptr;
    }
    
    std::vector<const MaterialNodeDef*> getByCategory(NodeCategory cat) const {
        std::vector<const MaterialNodeDef*> result;
        for (const auto& d : defs_) {
            if (d.category == cat) result.push_back(&d);
        }
        return result;
    }
    
    std::vector<const MaterialNodeDef*> search(const std::string& query) const {
        std::vector<const MaterialNodeDef*> result;
        std::string q = query;
        std::transform(q.begin(), q.end(), q.begin(), ::tolower);
        for (const auto& d : defs_) {
            std::string name = d.displayName;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            std::string type = d.typeName;
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);
            if (name.find(q) != std::string::npos || type.find(q) != std::string::npos) {
                result.push_back(&d);
            }
        }
        return result;
    }
    
    // Get all material categories
    static const std::vector<NodeCategory>& getMaterialCategories() {
        static std::vector<NodeCategory> cats = {
            NodeCategory::Mat_Input,
            NodeCategory::Mat_Texture,
            NodeCategory::Mat_Math,
            NodeCategory::Mat_Color,
            NodeCategory::Mat_Vector,
            NodeCategory::Mat_Procedural,
            NodeCategory::Mat_UV,
            NodeCategory::Mat_Output,
            NodeCategory::Mat_Utility
        };
        return cats;
    }
    
private:
    MaterialNodeLibrary() { registerAllNodes(); }
    
    void registerAllNodes() {
        registerInputNodes();
        registerTextureNodes();
        registerMathNodes();
        registerColorNodes();
        registerVectorNodes();
        registerProceduralNodes();
        registerUVNodes();
        registerOutputNodes();
        registerUtilityNodes();
    }
    
    // =========================================================================
    // INPUT NODES
    // =========================================================================
    void registerInputNodes() {
        // --- Texture Coordinate ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_TexCoord";
            d.displayName = "Texture Coordinate";
            d.category = NodeCategory::Mat_Input;
            d.description = "Provides UV coordinates and geometry data from the vertex shader";
            d.headerColor = 0xFFCC3333;
            d.addOutput("UV", PinType::UV);
            d.addOutput("Normal", PinType::Normal);
            d.addOutput("Position", PinType::Vec3);
            d.addOutput("Tangent", PinType::Vec3);
            d.addOutput("Bitangent", PinType::Vec3);
            d.addOutput("View Dir", PinType::Vec3);
            d.hlslTemplate =
                "{output:UV} = _input.uv;\n"
                "{output:Normal} = normalize(_input.normal);\n"
                "{output:Position} = _input.worldPos;\n"
                "{output:Tangent} = normalize(_input.tangent);\n"
                "{output:Bitangent} = normalize(_input.bitangent);\n"
                "{output:View Dir} = normalize(cameraPos - _input.worldPos);\n";
            defs_.push_back(d);
        }
        // --- Vertex Color ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_VertexColor";
            d.displayName = "Vertex Color";
            d.category = NodeCategory::Mat_Input;
            d.description = "Vertex color attribute";
            d.headerColor = 0xFFCC3333;
            d.addOutput("Color", PinType::Color);
            d.addOutput("R", PinType::Float);
            d.addOutput("G", PinType::Float);
            d.addOutput("B", PinType::Float);
            d.hlslTemplate =
                "{output:Color} = float4(_input.color, 1.0);\n"
                "{output:R} = _input.color.r;\n"
                "{output:G} = _input.color.g;\n"
                "{output:B} = _input.color.b;\n";
            defs_.push_back(d);
        }
        // --- Camera Data ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_CameraData";
            d.displayName = "Camera Data";
            d.category = NodeCategory::Mat_Input;
            d.description = "Camera position and view direction";
            d.headerColor = 0xFFCC3333;
            d.addOutput("Position", PinType::Vec3);
            d.addOutput("View Dir", PinType::Vec3);
            d.addOutput("Distance", PinType::Float);
            d.hlslTemplate =
                "{output:Position} = cameraPos;\n"
                "{output:View Dir} = normalize(cameraPos - _input.worldPos);\n"
                "{output:Distance} = length(cameraPos - _input.worldPos);\n";
            defs_.push_back(d);
        }
        // --- Value (Float Constant) ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Value";
            d.displayName = "Value";
            d.category = NodeCategory::Mat_Input;
            d.description = "Constant float value";
            d.headerColor = 0xFFCC3333;
            d.addOutput("Value", PinType::Float);
            d.addProperty("Value", "Value", PinType::Float, 0.5f, 0.0f, 100.0f);
            d.hlslTemplate = "{output:Value} = {param:Value};\n";
            defs_.push_back(d);
        }
        // --- Color Constant ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_ColorConst";
            d.displayName = "Color";
            d.category = NodeCategory::Mat_Input;
            d.description = "Constant color value";
            d.headerColor = 0xFFCC3333;
            d.addOutput("Color", PinType::Color);
            d.addOutput("R", PinType::Float);
            d.addOutput("G", PinType::Float);
            d.addOutput("B", PinType::Float);
            d.addOutput("A", PinType::Float);
            d.addProperty("Color", "Color", PinType::Color, Vec4(0.8f, 0.8f, 0.8f, 1.0f));
            d.hlslTemplate =
                "{output:Color} = {param:Color};\n"
                "{output:R} = {param:Color}.r;\n"
                "{output:G} = {param:Color}.g;\n"
                "{output:B} = {param:Color}.b;\n"
                "{output:A} = {param:Color}.a;\n";
            defs_.push_back(d);
        }
        // --- Vec3 Constant ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Vec3Const";
            d.displayName = "Vector3";
            d.category = NodeCategory::Mat_Input;
            d.description = "Constant Vec3 value";
            d.headerColor = 0xFFCC3333;
            d.addOutput("Vector", PinType::Vec3);
            d.addProperty("X", "X", PinType::Float, 0.0f, -100.0f, 100.0f);
            d.addProperty("Y", "Y", PinType::Float, 0.0f, -100.0f, 100.0f);
            d.addProperty("Z", "Z", PinType::Float, 0.0f, -100.0f, 100.0f);
            d.hlslTemplate = "{output:Vector} = float3({param:X}, {param:Y}, {param:Z});\n";
            defs_.push_back(d);
        }
    }
    
    // =========================================================================
    // TEXTURE NODES
    // =========================================================================
    void registerTextureNodes() {
        // --- Image Texture ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_ImageTexture";
            d.displayName = "Image Texture";
            d.category = NodeCategory::Mat_Texture;
            d.description = "Sample a 2D texture image";
            d.headerColor = 0xFF9933CC;
            d.addInput("UV", PinType::UV, Vec2(0, 0));
            d.addOutput("Color", PinType::Color);
            d.addOutput("R", PinType::Float);
            d.addOutput("G", PinType::Float);
            d.addOutput("B", PinType::Float);
            d.addOutput("A", PinType::Float);
            d.addProperty("Texture", "Texture", PinType::Texture2D, std::string(""));
            d.hlslTemplate =
                "float2 _uv_{node} = {input:UV};\n"
                "{output:Color} = {texture}.Sample(matSampler, _uv_{node});\n"
                "{output:R} = {output:Color}.r;\n"
                "{output:G} = {output:Color}.g;\n"
                "{output:B} = {output:Color}.b;\n"
                "{output:A} = {output:Color}.a;\n";
            defs_.push_back(d);
        }
    }
    
    // =========================================================================
    // MATH NODES
    // =========================================================================
    void registerMathNodes() {
        // --- Add ---
        registerBinaryMathNode("Mat_Add", "Add", "+", "Add two values");
        // --- Subtract ---
        registerBinaryMathNode("Mat_Subtract", "Subtract", "-", "Subtract B from A");
        // --- Multiply ---
        registerBinaryMathNode("Mat_Multiply", "Multiply", "*", "Multiply two values");
        // --- Divide ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Divide";
            d.displayName = "Divide";
            d.category = NodeCategory::Mat_Math;
            d.description = "Divide A by B (safe, avoids division by zero)";
            d.headerColor = 0xFF33AA33;
            d.addInput("A", PinType::Float, 0.0f);
            d.addInput("B", PinType::Float, 1.0f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = {input:A} / max({input:B}, 0.00001);\n";
            defs_.push_back(d);
        }
        // --- Power ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Power";
            d.displayName = "Power";
            d.category = NodeCategory::Mat_Math;
            d.description = "Raise A to the power of B";
            d.headerColor = 0xFF33AA33;
            d.addInput("Base", PinType::Float, 0.0f);
            d.addInput("Exponent", PinType::Float, 2.0f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = pow(max(abs({input:Base}), 0.00001), {input:Exponent});\n";
            defs_.push_back(d);
        }
        // --- Sqrt ---
        registerUnaryMathNode("Mat_Sqrt", "Square Root", "sqrt", "Square root");
        // --- Abs ---
        registerUnaryMathNode("Mat_Abs", "Absolute", "abs", "Absolute value");
        // --- Floor ---
        registerUnaryMathNode("Mat_Floor", "Floor", "floor", "Floor value");
        // --- Ceil ---
        registerUnaryMathNode("Mat_Ceil", "Ceil", "ceil", "Ceiling value");
        // --- Frac ---
        registerUnaryMathNode("Mat_Frac", "Fraction", "frac", "Fractional part");
        // --- Round ---
        registerUnaryMathNode("Mat_Round", "Round", "round", "Round to nearest integer");
        // --- Sign ---
        registerUnaryMathNode("Mat_Sign", "Sign", "sign", "Sign of value (-1, 0, or 1)");
        // --- Mod ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Mod";
            d.displayName = "Modulo";
            d.category = NodeCategory::Mat_Math;
            d.description = "A modulo B";
            d.headerColor = 0xFF33AA33;
            d.addInput("A", PinType::Float, 0.0f);
            d.addInput("B", PinType::Float, 1.0f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = fmod({input:A}, max({input:B}, 0.00001));\n";
            defs_.push_back(d);
        }
        // --- Sin ---
        registerUnaryMathNode("Mat_Sin", "Sine", "sin", "Sine function");
        // --- Cos ---
        registerUnaryMathNode("Mat_Cos", "Cosine", "cos", "Cosine function");
        // --- Tan ---
        registerUnaryMathNode("Mat_Tan", "Tangent", "tan", "Tangent function");
        // --- ArcSin ---
        registerUnaryMathNode("Mat_Asin", "Arc Sine", "asin", "Arc sine (inverse sine)");
        // --- ArcCos ---
        registerUnaryMathNode("Mat_Acos", "Arc Cosine", "acos", "Arc cosine");
        // --- ArcTan2 ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Atan2";
            d.displayName = "Arc Tangent 2";
            d.category = NodeCategory::Mat_Math;
            d.description = "Two-argument arc tangent";
            d.headerColor = 0xFF33AA33;
            d.addInput("Y", PinType::Float, 0.0f);
            d.addInput("X", PinType::Float, 1.0f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = atan2({input:Y}, {input:X});\n";
            defs_.push_back(d);
        }
        // --- Lerp ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Lerp";
            d.displayName = "Lerp";
            d.category = NodeCategory::Mat_Math;
            d.description = "Linear interpolation between A and B";
            d.headerColor = 0xFF33AA33;
            d.addInput("A", PinType::Float, 0.0f);
            d.addInput("B", PinType::Float, 1.0f);
            d.addInput("Factor", PinType::Float, 0.5f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = lerp({input:A}, {input:B}, {input:Factor});\n";
            defs_.push_back(d);
        }
        // --- SmoothStep ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_SmoothStep";
            d.displayName = "Smooth Step";
            d.category = NodeCategory::Mat_Math;
            d.description = "Hermite interpolation between edge0 and edge1";
            d.headerColor = 0xFF33AA33;
            d.addInput("Edge0", PinType::Float, 0.0f);
            d.addInput("Edge1", PinType::Float, 1.0f);
            d.addInput("Value", PinType::Float, 0.5f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = smoothstep({input:Edge0}, {input:Edge1}, {input:Value});\n";
            defs_.push_back(d);
        }
        // --- Step ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Step";
            d.displayName = "Step";
            d.category = NodeCategory::Mat_Math;
            d.description = "Step function: 0 if Value < Edge, else 1";
            d.headerColor = 0xFF33AA33;
            d.addInput("Edge", PinType::Float, 0.5f);
            d.addInput("Value", PinType::Float, 0.0f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = step({input:Edge}, {input:Value});\n";
            defs_.push_back(d);
        }
        // --- Clamp ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Clamp";
            d.displayName = "Clamp";
            d.category = NodeCategory::Mat_Math;
            d.description = "Clamp value between min and max";
            d.headerColor = 0xFF33AA33;
            d.addInput("Value", PinType::Float, 0.0f);
            d.addInput("Min", PinType::Float, 0.0f);
            d.addInput("Max", PinType::Float, 1.0f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = clamp({input:Value}, {input:Min}, {input:Max});\n";
            defs_.push_back(d);
        }
        // --- Map Range ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_MapRange";
            d.displayName = "Map Range";
            d.category = NodeCategory::Mat_Math;
            d.description = "Remap value from one range to another";
            d.headerColor = 0xFF33AA33;
            d.addInput("Value", PinType::Float, 0.0f);
            d.addInput("From Min", PinType::Float, 0.0f);
            d.addInput("From Max", PinType::Float, 1.0f);
            d.addInput("To Min", PinType::Float, 0.0f);
            d.addInput("To Max", PinType::Float, 1.0f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate =
                "float _range_{node} = max({input:From Max} - {input:From Min}, 0.00001);\n"
                "{output:Result} = {input:To Min} + ({input:Value} - {input:From Min}) / _range_{node} * ({input:To Max} - {input:To Min});\n";
            defs_.push_back(d);
        }
        // --- Min ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Min";
            d.displayName = "Minimum";
            d.category = NodeCategory::Mat_Math;
            d.description = "Minimum of two values";
            d.headerColor = 0xFF33AA33;
            d.addInput("A", PinType::Float, 0.0f);
            d.addInput("B", PinType::Float, 0.0f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = min({input:A}, {input:B});\n";
            defs_.push_back(d);
        }
        // --- Max ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Max";
            d.displayName = "Maximum";
            d.category = NodeCategory::Mat_Math;
            d.description = "Maximum of two values";
            d.headerColor = 0xFF33AA33;
            d.addInput("A", PinType::Float, 0.0f);
            d.addInput("B", PinType::Float, 0.0f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = max({input:A}, {input:B});\n";
            defs_.push_back(d);
        }
        // --- OneMinus ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_OneMinus";
            d.displayName = "One Minus";
            d.category = NodeCategory::Mat_Math;
            d.description = "1.0 - Value";
            d.headerColor = 0xFF33AA33;
            d.addInput("Value", PinType::Float, 0.0f);
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = 1.0 - {input:Value};\n";
            defs_.push_back(d);
        }
        // --- Saturate ---
        registerUnaryMathNode("Mat_Saturate", "Saturate", "saturate", "Clamp to [0,1]");
    }
    
    // =========================================================================
    // COLOR NODES
    // =========================================================================
    void registerColorNodes() {
        // --- Mix / Blend ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_MixColor";
            d.displayName = "Mix";
            d.category = NodeCategory::Mat_Color;
            d.description = "Blend two colors by factor";
            d.headerColor = 0xFFCCCC33;
            d.addInput("Color1", PinType::Color, Vec4(0, 0, 0, 1));
            d.addInput("Color2", PinType::Color, Vec4(1, 1, 1, 1));
            d.addInput("Factor", PinType::Float, 0.5f);
            d.addOutput("Color", PinType::Color);
            d.addEnumProperty("BlendMode", "Blend Mode", 
                {"Mix", "Multiply", "Screen", "Overlay", "Add", "Subtract", "Darken", "Lighten"}, 0);
            // Code gen handles blend modes via switch on property
            d.hlslTemplate =
                "float4 _c1_{node} = {input:Color1};\n"
                "float4 _c2_{node} = {input:Color2};\n"
                "float _fac_{node} = {input:Factor};\n"
                "{output:Color} = lerp(_c1_{node}, _c2_{node}, _fac_{node});\n";
            defs_.push_back(d);
        }
        // --- RGB to HSV ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_RGBtoHSV";
            d.displayName = "RGB to HSV";
            d.category = NodeCategory::Mat_Color;
            d.description = "Convert RGB color to HSV";
            d.headerColor = 0xFFCCCC33;
            d.addInput("Color", PinType::Color, Vec4(0.5f, 0.5f, 0.5f, 1));
            d.addOutput("H", PinType::Float);
            d.addOutput("S", PinType::Float);
            d.addOutput("V", PinType::Float);
            d.hlslHelperIncludes = "procedural";
            d.hlslTemplate =
                "float3 _hsv_{node} = rgbToHsv({input:Color}.rgb);\n"
                "{output:H} = _hsv_{node}.x;\n"
                "{output:S} = _hsv_{node}.y;\n"
                "{output:V} = _hsv_{node}.z;\n";
            defs_.push_back(d);
        }
        // --- HSV to RGB ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_HSVtoRGB";
            d.displayName = "HSV to RGB";
            d.category = NodeCategory::Mat_Color;
            d.description = "Convert HSV to RGB color";
            d.headerColor = 0xFFCCCC33;
            d.addInput("H", PinType::Float, 0.0f);
            d.addInput("S", PinType::Float, 1.0f);
            d.addInput("V", PinType::Float, 1.0f);
            d.addOutput("Color", PinType::Color);
            d.hlslHelperIncludes = "procedural";
            d.hlslTemplate =
                "{output:Color} = float4(hsvToRgb(float3({input:H}, {input:S}, {input:V})), 1.0);\n";
            defs_.push_back(d);
        }
        // --- Brightness/Contrast ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_BrightnessContrast";
            d.displayName = "Brightness/Contrast";
            d.category = NodeCategory::Mat_Color;
            d.description = "Adjust brightness and contrast";
            d.headerColor = 0xFFCCCC33;
            d.addInput("Color", PinType::Color, Vec4(0.5f, 0.5f, 0.5f, 1));
            d.addInput("Brightness", PinType::Float, 0.0f);
            d.addInput("Contrast", PinType::Float, 0.0f);
            d.addOutput("Color", PinType::Color);
            d.hlslTemplate =
                "float4 _bc_{node} = {input:Color};\n"
                "_bc_{node}.rgb += {input:Brightness};\n"
                "_bc_{node}.rgb = (_bc_{node}.rgb - 0.5) * (1.0 + {input:Contrast}) + 0.5;\n"
                "{output:Color} = _bc_{node};\n";
            defs_.push_back(d);
        }
        // --- Hue Saturation Value ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_HueSatVal";
            d.displayName = "Hue/Saturation/Value";
            d.category = NodeCategory::Mat_Color;
            d.description = "Adjust hue, saturation, and value";
            d.headerColor = 0xFFCCCC33;
            d.addInput("Color", PinType::Color, Vec4(0.5f, 0.5f, 0.5f, 1));
            d.addInput("Hue", PinType::Float, 0.5f);
            d.addInput("Saturation", PinType::Float, 1.0f);
            d.addInput("Value", PinType::Float, 1.0f);
            d.addOutput("Color", PinType::Color);
            d.hlslHelperIncludes = "procedural";
            d.hlslTemplate =
                "float3 _hsv2_{node} = rgbToHsv({input:Color}.rgb);\n"
                "_hsv2_{node}.x = frac(_hsv2_{node}.x + {input:Hue} - 0.5);\n"
                "_hsv2_{node}.y *= {input:Saturation};\n"
                "_hsv2_{node}.z *= {input:Value};\n"
                "{output:Color} = float4(hsvToRgb(_hsv2_{node}), {input:Color}.a);\n";
            defs_.push_back(d);
        }
        // --- Invert ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Invert";
            d.displayName = "Invert";
            d.category = NodeCategory::Mat_Color;
            d.description = "Invert color (1 - RGB)";
            d.headerColor = 0xFFCCCC33;
            d.addInput("Color", PinType::Color, Vec4(0.5f, 0.5f, 0.5f, 1));
            d.addOutput("Color", PinType::Color);
            d.hlslTemplate =
                "{output:Color} = float4(1.0 - {input:Color}.rgb, {input:Color}.a);\n";
            defs_.push_back(d);
        }
        // --- Gamma ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Gamma";
            d.displayName = "Gamma";
            d.category = NodeCategory::Mat_Color;
            d.description = "Apply gamma correction";
            d.headerColor = 0xFFCCCC33;
            d.addInput("Color", PinType::Color, Vec4(0.5f, 0.5f, 0.5f, 1));
            d.addInput("Gamma", PinType::Float, 2.2f);
            d.addOutput("Color", PinType::Color);
            d.hlslTemplate =
                "{output:Color} = float4(pow(max(abs({input:Color}.rgb), 0.00001), 1.0/{input:Gamma}), {input:Color}.a);\n";
            defs_.push_back(d);
        }
        // --- Color Ramp ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_ColorRamp";
            d.displayName = "Color Ramp";
            d.category = NodeCategory::Mat_Color;
            d.description = "Map a value to a color gradient";
            d.headerColor = 0xFFCCCC33;
            d.addInput("Factor", PinType::Float, 0.5f);
            d.addOutput("Color", PinType::Color);
            d.addOutput("Alpha", PinType::Float);
            // Color ramp data is stored per-node in MaterialGraph::colorRamps
            // Code gen creates a lookup function
            d.hlslTemplate = 
                "{output:Color} = colorRamp_{node}({input:Factor});\n"
                "{output:Alpha} = {output:Color}.a;\n";
            defs_.push_back(d);
        }
    }
    
    // =========================================================================
    // VECTOR NODES
    // =========================================================================
    void registerVectorNodes() {
        // --- Combine XYZ ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_CombineXYZ";
            d.displayName = "Combine XYZ";
            d.category = NodeCategory::Mat_Vector;
            d.description = "Combine X, Y, Z components into a vector";
            d.headerColor = 0xFF3366CC;
            d.addInput("X", PinType::Float, 0.0f);
            d.addInput("Y", PinType::Float, 0.0f);
            d.addInput("Z", PinType::Float, 0.0f);
            d.addOutput("Vector", PinType::Vec3);
            d.hlslTemplate = "{output:Vector} = float3({input:X}, {input:Y}, {input:Z});\n";
            defs_.push_back(d);
        }
        // --- Separate XYZ ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_SeparateXYZ";
            d.displayName = "Separate XYZ";
            d.category = NodeCategory::Mat_Vector;
            d.description = "Split a vector into X, Y, Z components";
            d.headerColor = 0xFF3366CC;
            d.addInput("Vector", PinType::Vec3, Vec3(0, 0, 0));
            d.addOutput("X", PinType::Float);
            d.addOutput("Y", PinType::Float);
            d.addOutput("Z", PinType::Float);
            d.hlslTemplate =
                "{output:X} = {input:Vector}.x;\n"
                "{output:Y} = {input:Vector}.y;\n"
                "{output:Z} = {input:Vector}.z;\n";
            defs_.push_back(d);
        }
        // --- Combine RGBA ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_CombineRGBA";
            d.displayName = "Combine RGBA";
            d.category = NodeCategory::Mat_Vector;
            d.description = "Combine R, G, B, A into a color";
            d.headerColor = 0xFF3366CC;
            d.addInput("R", PinType::Float, 0.0f);
            d.addInput("G", PinType::Float, 0.0f);
            d.addInput("B", PinType::Float, 0.0f);
            d.addInput("A", PinType::Float, 1.0f);
            d.addOutput("Color", PinType::Color);
            d.hlslTemplate = "{output:Color} = float4({input:R}, {input:G}, {input:B}, {input:A});\n";
            defs_.push_back(d);
        }
        // --- Separate RGBA ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_SeparateRGBA";
            d.displayName = "Separate RGBA";
            d.category = NodeCategory::Mat_Vector;
            d.description = "Split a color into R, G, B, A components";
            d.headerColor = 0xFF3366CC;
            d.addInput("Color", PinType::Color, Vec4(0, 0, 0, 1));
            d.addOutput("R", PinType::Float);
            d.addOutput("G", PinType::Float);
            d.addOutput("B", PinType::Float);
            d.addOutput("A", PinType::Float);
            d.hlslTemplate =
                "{output:R} = {input:Color}.r;\n"
                "{output:G} = {input:Color}.g;\n"
                "{output:B} = {input:Color}.b;\n"
                "{output:A} = {input:Color}.a;\n";
            defs_.push_back(d);
        }
        // --- Dot Product ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_DotProduct";
            d.displayName = "Dot Product";
            d.category = NodeCategory::Mat_Vector;
            d.description = "Dot product of two vectors";
            d.headerColor = 0xFF3366CC;
            d.addInput("A", PinType::Vec3, Vec3(0, 0, 0));
            d.addInput("B", PinType::Vec3, Vec3(0, 0, 0));
            d.addOutput("Result", PinType::Float);
            d.hlslTemplate = "{output:Result} = dot({input:A}, {input:B});\n";
            defs_.push_back(d);
        }
        // --- Cross Product ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_CrossProduct";
            d.displayName = "Cross Product";
            d.category = NodeCategory::Mat_Vector;
            d.description = "Cross product of two vectors";
            d.headerColor = 0xFF3366CC;
            d.addInput("A", PinType::Vec3, Vec3(0, 1, 0));
            d.addInput("B", PinType::Vec3, Vec3(1, 0, 0));
            d.addOutput("Result", PinType::Vec3);
            d.hlslTemplate = "{output:Result} = cross({input:A}, {input:B});\n";
            defs_.push_back(d);
        }
        // --- Normalize ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Normalize";
            d.displayName = "Normalize";
            d.category = NodeCategory::Mat_Vector;
            d.description = "Normalize a vector to unit length";
            d.headerColor = 0xFF3366CC;
            d.addInput("Vector", PinType::Vec3, Vec3(0, 0, 1));
            d.addOutput("Result", PinType::Vec3);
            d.hlslTemplate = "{output:Result} = normalize({input:Vector});\n";
            defs_.push_back(d);
        }
        // --- Length ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_VecLength";
            d.displayName = "Vector Length";
            d.category = NodeCategory::Mat_Vector;
            d.description = "Length of a vector";
            d.headerColor = 0xFF3366CC;
            d.addInput("Vector", PinType::Vec3, Vec3(0, 0, 0));
            d.addOutput("Length", PinType::Float);
            d.hlslTemplate = "{output:Length} = length({input:Vector});\n";
            defs_.push_back(d);
        }
        // --- Reflect ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Reflect";
            d.displayName = "Reflect";
            d.category = NodeCategory::Mat_Vector;
            d.description = "Reflect vector about normal";
            d.headerColor = 0xFF3366CC;
            d.addInput("Vector", PinType::Vec3, Vec3(0, 0, 0));
            d.addInput("Normal", PinType::Normal, Vec3(0, 1, 0));
            d.addOutput("Result", PinType::Vec3);
            d.hlslTemplate = "{output:Result} = reflect({input:Vector}, {input:Normal});\n";
            defs_.push_back(d);
        }
        // --- Normal Map ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_NormalMap";
            d.displayName = "Normal Map";
            d.category = NodeCategory::Mat_Vector;
            d.description = "Unpack a normal map texture and transform to world space";
            d.headerColor = 0xFF3366CC;
            d.addInput("Color", PinType::Color, Vec4(0.5f, 0.5f, 1.0f, 1.0f));
            d.addInput("Strength", PinType::Float, 1.0f);
            d.addOutput("Normal", PinType::Normal);
            d.hlslTemplate =
                "float3 _nm_{node} = {input:Color}.rgb * 2.0 - 1.0;\n"
                "_nm_{node}.xy *= {input:Strength};\n"
                "_nm_{node} = normalize(_nm_{node});\n"
                "float3x3 _TBN_{node} = float3x3(normalize(_input.tangent), normalize(_input.bitangent), normalize(_input.normal));\n"
                "{output:Normal} = normalize(mul(_nm_{node}, _TBN_{node}));\n";
            defs_.push_back(d);
        }
        // --- Bump ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Bump";
            d.displayName = "Bump";
            d.category = NodeCategory::Mat_Vector;
            d.description = "Generate normal from height map (central differences)";
            d.headerColor = 0xFF3366CC;
            d.addInput("Height", PinType::Float, 0.0f);
            d.addInput("Strength", PinType::Float, 0.1f);
            d.addOutput("Normal", PinType::Normal);
            d.hlslTemplate =
                "float _bumpStr_{node} = {input:Strength};\n"
                "float3 _bumpN_{node} = normalize(_input.normal + float3(ddx({input:Height}), ddy({input:Height}), 0.0) * _bumpStr_{node} * 100.0);\n"
                "{output:Normal} = _bumpN_{node};\n";
            defs_.push_back(d);
        }
    }
    
    // =========================================================================
    // PROCEDURAL NODES
    // =========================================================================
    void registerProceduralNodes() {
        // --- Noise Texture ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Noise";
            d.displayName = "Noise Texture";
            d.category = NodeCategory::Mat_Procedural;
            d.description = "Procedural noise texture (Perlin/Simplex)";
            d.headerColor = 0xFF33CCCC;
            d.addInput("UV", PinType::UV, Vec2(0, 0));
            d.addInput("Scale", PinType::Float, 5.0f);
            d.addInput("Detail", PinType::Float, 2.0f);
            d.addInput("Roughness", PinType::Float, 0.5f);
            d.addOutput("Factor", PinType::Float);
            d.addOutput("Color", PinType::Color);
            d.addEnumProperty("NoiseType", "Type", {"Perlin", "Simplex"}, 0);
            d.hlslHelperIncludes = "procedural";
            d.hlslTemplate =
                "float2 _nuv_{node} = {input:UV} * {input:Scale};\n"
                "float _nval_{node} = fbm2D(_nuv_{node}, (int){input:Detail}, {input:Roughness});\n"
                "_nval_{node} = _nval_{node} * 0.5 + 0.5;\n"
                "{output:Factor} = _nval_{node};\n"
                "{output:Color} = float4(_nval_{node}, _nval_{node}, _nval_{node}, 1.0);\n";
            defs_.push_back(d);
        }
        // --- Voronoi Texture ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Voronoi";
            d.displayName = "Voronoi Texture";
            d.category = NodeCategory::Mat_Procedural;
            d.description = "Voronoi/Worley noise texture";
            d.headerColor = 0xFF33CCCC;
            d.addInput("UV", PinType::UV, Vec2(0, 0));
            d.addInput("Scale", PinType::Float, 5.0f);
            d.addInput("Randomness", PinType::Float, 1.0f);
            d.addOutput("Distance", PinType::Float);
            d.addOutput("Color", PinType::Color);
            d.addOutput("Position", PinType::Vec3);
            d.addEnumProperty("Feature", "Feature", {"F1", "F2", "Smooth F1"}, 0);
            d.hlslHelperIncludes = "procedural";
            d.hlslTemplate =
                "float2 _vuv_{node} = {input:UV} * {input:Scale};\n"
                "float2 _vcell_{node};\n"
                "float _vdist_{node} = voronoi2D(_vuv_{node}, {input:Randomness}, _vcell_{node});\n"
                "{output:Distance} = _vdist_{node};\n"
                "{output:Color} = float4(_vdist_{node}, _vdist_{node}, _vdist_{node}, 1.0);\n"
                "{output:Position} = float3(_vcell_{node}, 0.0);\n";
            defs_.push_back(d);
        }
        // --- Checker Texture ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Checker";
            d.displayName = "Checker Texture";
            d.category = NodeCategory::Mat_Procedural;
            d.description = "Checkerboard pattern";
            d.headerColor = 0xFF33CCCC;
            d.addInput("UV", PinType::UV, Vec2(0, 0));
            d.addInput("Color1", PinType::Color, Vec4(0, 0, 0, 1));
            d.addInput("Color2", PinType::Color, Vec4(1, 1, 1, 1));
            d.addInput("Scale", PinType::Float, 5.0f);
            d.addOutput("Color", PinType::Color);
            d.addOutput("Factor", PinType::Float);
            d.hlslTemplate =
                "float2 _cuv_{node} = {input:UV} * {input:Scale};\n"
                "float _cfac_{node} = (fmod(floor(_cuv_{node}.x) + floor(_cuv_{node}.y), 2.0) < 0.5) ? 0.0 : 1.0;\n"
                "{output:Factor} = _cfac_{node};\n"
                "{output:Color} = lerp({input:Color1}, {input:Color2}, _cfac_{node});\n";
            defs_.push_back(d);
        }
        // --- Gradient Texture ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Gradient";
            d.displayName = "Gradient Texture";
            d.category = NodeCategory::Mat_Procedural;
            d.description = "Gradient pattern (linear, radial, spherical)";
            d.headerColor = 0xFF33CCCC;
            d.addInput("UV", PinType::UV, Vec2(0, 0));
            d.addOutput("Factor", PinType::Float);
            d.addOutput("Color", PinType::Color);
            d.addEnumProperty("GradientType", "Type", {"Linear", "Quadratic", "Radial", "Spherical"}, 0);
            d.hlslTemplate =
                "float _gfac_{node} = saturate({input:UV}.x);\n"
                "{output:Factor} = _gfac_{node};\n"
                "{output:Color} = float4(_gfac_{node}, _gfac_{node}, _gfac_{node}, 1.0);\n";
            defs_.push_back(d);
        }
        // --- Wave Texture ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Wave";
            d.displayName = "Wave Texture";
            d.category = NodeCategory::Mat_Procedural;
            d.description = "Wave pattern (sine, saw, triangle)";
            d.headerColor = 0xFF33CCCC;
            d.addInput("UV", PinType::UV, Vec2(0, 0));
            d.addInput("Scale", PinType::Float, 5.0f);
            d.addInput("Distortion", PinType::Float, 0.0f);
            d.addOutput("Factor", PinType::Float);
            d.addOutput("Color", PinType::Color);
            d.addEnumProperty("WaveType", "Type", {"Sine", "Sawtooth", "Triangle"}, 0);
            d.hlslHelperIncludes = "procedural";
            d.hlslTemplate =
                "float _wval_{node} = sin({input:UV}.x * {input:Scale} * 6.283185 + {input:Distortion}) * 0.5 + 0.5;\n"
                "{output:Factor} = _wval_{node};\n"
                "{output:Color} = float4(_wval_{node}, _wval_{node}, _wval_{node}, 1.0);\n";
            defs_.push_back(d);
        }
        // --- Brick Texture ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Brick";
            d.displayName = "Brick Texture";
            d.category = NodeCategory::Mat_Procedural;
            d.description = "Brick pattern";
            d.headerColor = 0xFF33CCCC;
            d.addInput("UV", PinType::UV, Vec2(0, 0));
            d.addInput("Color1", PinType::Color, Vec4(0.6f, 0.15f, 0.05f, 1));
            d.addInput("Color2", PinType::Color, Vec4(0.8f, 0.8f, 0.75f, 1));
            d.addInput("Scale", PinType::Float, 5.0f);
            d.addInput("Mortar Size", PinType::Float, 0.02f);
            d.addOutput("Color", PinType::Color);
            d.addOutput("Factor", PinType::Float);
            d.hlslHelperIncludes = "procedural";
            d.hlslTemplate =
                "float2 _buv_{node} = {input:UV} * {input:Scale};\n"
                "float _bfac_{node} = brickPattern(_buv_{node}, {input:Mortar Size});\n"
                "{output:Factor} = _bfac_{node};\n"
                "{output:Color} = lerp({input:Color2}, {input:Color1}, _bfac_{node});\n";
            defs_.push_back(d);
        }
        // --- Musgrave Texture ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Musgrave";
            d.displayName = "Musgrave Texture";
            d.category = NodeCategory::Mat_Procedural;
            d.description = "Musgrave fractal noise (fBM, Multifractal, etc.)";
            d.headerColor = 0xFF33CCCC;
            d.addInput("UV", PinType::UV, Vec2(0, 0));
            d.addInput("Scale", PinType::Float, 5.0f);
            d.addInput("Detail", PinType::Float, 2.0f);
            d.addInput("Dimension", PinType::Float, 2.0f);
            d.addInput("Lacunarity", PinType::Float, 2.0f);
            d.addOutput("Factor", PinType::Float);
            d.addEnumProperty("MusgraveType", "Type", {"fBM", "Multifractal", "Ridged", "Hybrid", "Hetero"}, 0);
            d.hlslHelperIncludes = "procedural";
            d.hlslTemplate =
                "float2 _muv_{node} = {input:UV} * {input:Scale};\n"
                "float _mval_{node} = musgraveNoise(_muv_{node}, (int){input:Detail}, {input:Dimension}, {input:Lacunarity});\n"
                "{output:Factor} = saturate(_mval_{node});\n";
            defs_.push_back(d);
        }
    }
    
    // =========================================================================
    // UV NODES
    // =========================================================================
    void registerUVNodes() {
        // --- UV Mapping (Transform) ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_UVMapping";
            d.displayName = "UV Mapping";
            d.category = NodeCategory::Mat_UV;
            d.description = "Transform UV coordinates (offset, scale, rotation)";
            d.headerColor = 0xFFCC66CC;
            d.addInput("UV", PinType::UV, Vec2(0, 0));
            d.addInput("Offset", PinType::Vec2, Vec2(0, 0));
            d.addInput("Scale", PinType::Vec2, Vec2(1, 1));
            d.addInput("Rotation", PinType::Float, 0.0f);
            d.addOutput("UV", PinType::UV);
            d.hlslTemplate =
                "float2 _uvm_{node} = ({input:UV} - 0.5) * {input:Scale};\n"
                "float _uvcos_{node} = cos({input:Rotation});\n"
                "float _uvsin_{node} = sin({input:Rotation});\n"
                "_uvm_{node} = float2(_uvm_{node}.x * _uvcos_{node} - _uvm_{node}.y * _uvsin_{node},\n"
                "                     _uvm_{node}.x * _uvsin_{node} + _uvm_{node}.y * _uvcos_{node});\n"
                "{output:UV} = _uvm_{node} + 0.5 + {input:Offset};\n";
            defs_.push_back(d);
        }
        // --- UV Tiling ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_UVTiling";
            d.displayName = "UV Tiling";
            d.category = NodeCategory::Mat_UV;
            d.description = "Tile UVs (scale and repeat)";
            d.headerColor = 0xFFCC66CC;
            d.addInput("UV", PinType::UV, Vec2(0, 0));
            d.addInput("Tiling X", PinType::Float, 1.0f);
            d.addInput("Tiling Y", PinType::Float, 1.0f);
            d.addOutput("UV", PinType::UV);
            d.hlslTemplate =
                "{output:UV} = frac({input:UV} * float2({input:Tiling X}, {input:Tiling Y}));\n";
            defs_.push_back(d);
        }
    }
    
    // =========================================================================
    // OUTPUT NODES
    // =========================================================================
    void registerOutputNodes() {
        // --- PBR Material Output ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_PBROutput";
            d.displayName = "PBR Material Output";
            d.category = NodeCategory::Mat_Output;
            d.description = "Final PBR material output";
            d.headerColor = 0xFF33CC66;
            d.isOutput = true;
            d.addInput("Base Color", PinType::Color, Vec4(0.8f, 0.8f, 0.8f, 1.0f));
            d.addInput("Metallic", PinType::Float, 0.0f);
            d.addInput("Roughness", PinType::Float, 0.5f);
            d.addInput("Normal", PinType::Normal, Vec3(0, 0, 1));
            d.addInput("Emissive", PinType::Color, Vec4(0, 0, 0, 0));
            d.addInput("AO", PinType::Float, 1.0f);
            d.addInput("Alpha", PinType::Float, 1.0f);
            d.addInput("Height", PinType::Float, 0.0f);
            d.addInput("Subsurface", PinType::Float, 0.0f);
            d.addInput("Clearcoat", PinType::Float, 0.0f);
            d.addInput("Anisotropy", PinType::Float, 0.0f);
            // No HLSL template - handled specially by code generator
            d.hlslTemplate = "";
            defs_.push_back(d);
        }
    }
    
    // =========================================================================
    // UTILITY NODES
    // =========================================================================
    void registerUtilityNodes() {
        // --- Fresnel ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_Fresnel";
            d.displayName = "Fresnel";
            d.category = NodeCategory::Mat_Utility;
            d.description = "Fresnel effect based on view angle";
            d.headerColor = 0xFF888888;
            d.addInput("IOR", PinType::Float, 1.5f);
            d.addInput("Normal", PinType::Normal, Vec3(0, 0, 1));
            d.addOutput("Factor", PinType::Float);
            d.hlslTemplate =
                "float3 _fV_{node} = normalize(cameraPos - _input.worldPos);\n"
                "float _fNdotV_{node} = max(dot({input:Normal}, _fV_{node}), 0.0);\n"
                "float _fR0_{node} = pow(({input:IOR} - 1.0) / ({input:IOR} + 1.0), 2.0);\n"
                "{output:Factor} = _fR0_{node} + (1.0 - _fR0_{node}) * pow(1.0 - _fNdotV_{node}, 5.0);\n";
            defs_.push_back(d);
        }
        // --- Layer Weight ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_LayerWeight";
            d.displayName = "Layer Weight";
            d.category = NodeCategory::Mat_Utility;
            d.description = "Blend weight based on facing/fresnel";
            d.headerColor = 0xFF888888;
            d.addInput("Blend", PinType::Float, 0.5f);
            d.addInput("Normal", PinType::Normal, Vec3(0, 0, 1));
            d.addOutput("Fresnel", PinType::Float);
            d.addOutput("Facing", PinType::Float);
            d.hlslTemplate =
                "float3 _lwV_{node} = normalize(cameraPos - _input.worldPos);\n"
                "float _lwNdotV_{node} = max(dot({input:Normal}, _lwV_{node}), 0.0);\n"
                "{output:Fresnel} = pow(1.0 - _lwNdotV_{node}, max(5.0 * (1.0 - {input:Blend}), 0.001));\n"
                "{output:Facing} = abs(_lwNdotV_{node});\n";
            defs_.push_back(d);
        }
        // --- Math: Vector Math (operations on vectors via enum) ---
        {
            MaterialNodeDef d;
            d.typeName = "Mat_VectorMath";
            d.displayName = "Vector Math";
            d.category = NodeCategory::Mat_Utility;
            d.description = "Various vector math operations";
            d.headerColor = 0xFF888888;
            d.addInput("A", PinType::Vec3, Vec3(0, 0, 0));
            d.addInput("B", PinType::Vec3, Vec3(0, 0, 0));
            d.addInput("Scale", PinType::Float, 1.0f);
            d.addOutput("Vector", PinType::Vec3);
            d.addOutput("Value", PinType::Float);
            d.addEnumProperty("Operation", "Operation",
                {"Add", "Subtract", "Multiply", "Divide", "Scale", "Dot", "Cross", "Normalize", "Length", "Distance", "Reflect"}, 0);
            d.hlslTemplate =
                "{output:Vector} = {input:A} + {input:B};\n"
                "{output:Value} = length({output:Vector});\n";
            defs_.push_back(d);
        }
    }
    
    // =========================================================================
    // HELPER REGISTRATION FUNCTIONS
    // =========================================================================
    
    void registerBinaryMathNode(const std::string& type, const std::string& display,
                                 const std::string& op, const std::string& desc) {
        MaterialNodeDef d;
        d.typeName = type;
        d.displayName = display;
        d.category = NodeCategory::Mat_Math;
        d.description = desc;
        d.headerColor = 0xFF33AA33;
        d.addInput("A", PinType::Float, 0.0f);
        d.addInput("B", PinType::Float, 0.0f);
        d.addOutput("Result", PinType::Float);
        d.hlslTemplate = "{output:Result} = {input:A} " + op + " {input:B};\n";
        defs_.push_back(d);
    }
    
    void registerUnaryMathNode(const std::string& type, const std::string& display,
                                const std::string& func, const std::string& desc) {
        MaterialNodeDef d;
        d.typeName = type;
        d.displayName = display;
        d.category = NodeCategory::Mat_Math;
        d.description = desc;
        d.headerColor = 0xFF33AA33;
        d.addInput("Value", PinType::Float, 0.0f);
        d.addOutput("Result", PinType::Float);
        d.hlslTemplate = "{output:Result} = " + func + "({input:Value});\n";
        defs_.push_back(d);
    }
    
    std::vector<MaterialNodeDef> defs_;
};

} // namespace luma
