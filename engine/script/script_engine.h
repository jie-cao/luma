// Lua Script Engine
// Lua state management, bindings, and network-aware scripting
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/network/network.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

// Forward declare Lua types (to avoid including lua.h in header)
struct lua_State;

namespace luma {

// ===== Script Value Types =====
enum class ScriptValueType {
    None,   // renamed from Nil to avoid Obj-C macro conflict
    Boolean,
    Number,
    String,
    Table,
    Function,
    UserData,
    Vec3,
    Quat
};

// ===== Script Value =====
struct ScriptValue {
    ScriptValueType type = ScriptValueType::None;
    
    union {
        bool boolValue;
        double numberValue;
        void* userDataValue;
    };
    std::string stringValue;
    Vec3 vec3Value;
    Quat quatValue;
    std::unordered_map<std::string, ScriptValue> tableValue;
    
    ScriptValue() : type(ScriptValueType::None), numberValue(0) {}
    ScriptValue(bool b) : type(ScriptValueType::Boolean), boolValue(b) {}
    ScriptValue(double n) : type(ScriptValueType::Number), numberValue(n) {}
    ScriptValue(const std::string& s) : type(ScriptValueType::String), numberValue(0), stringValue(s) {}
    ScriptValue(const Vec3& v) : type(ScriptValueType::Vec3), numberValue(0), vec3Value(v) {}
    ScriptValue(const Quat& q) : type(ScriptValueType::Quat), numberValue(0), quatValue(q) {}
    
    bool isNil() const { return type == ScriptValueType::None; }
    bool isNone() const { return type == ScriptValueType::None; }
    bool isBool() const { return type == ScriptValueType::Boolean; }
    bool isNumber() const { return type == ScriptValueType::Number; }
    bool isString() const { return type == ScriptValueType::String; }
    bool isTable() const { return type == ScriptValueType::Table; }
    bool isVec3() const { return type == ScriptValueType::Vec3; }
    bool isQuat() const { return type == ScriptValueType::Quat; }
    
    // Serialization for network
    void serialize(NetworkMessage& msg) const {
        msg.writeByte((uint8_t)type);
        switch (type) {
            case ScriptValueType::Boolean:
                msg.writeByte(boolValue ? 1 : 0);
                break;
            case ScriptValueType::Number:
                msg.writeFloat((float)numberValue);
                break;
            case ScriptValueType::String:
                msg.writeString(stringValue);
                break;
            case ScriptValueType::Vec3:
                msg.writeVec3(vec3Value);
                break;
            default:
                break;
        }
    }
    
    static ScriptValue deserialize(NetworkMessage& msg) {
        ScriptValueType t = (ScriptValueType)msg.readByte();
        switch (t) {
            case ScriptValueType::Boolean:
                return ScriptValue(msg.readByte() != 0);
            case ScriptValueType::Number:
                return ScriptValue((double)msg.readFloat());
            case ScriptValueType::String:
                return ScriptValue(msg.readString());
            case ScriptValueType::Vec3:
                return ScriptValue(msg.readVec3());
            default:
                return ScriptValue();
        }
    }
};

// ===== Script Property (for networking) =====
struct ScriptProperty {
    std::string name;
    ScriptValue value;
    bool networked = false;       // Sync over network
    bool serverAuthority = true;  // Server owns this property
    bool dirty = false;           // Changed since last sync
};

// ===== Script RPC Definition =====
struct ScriptRPCDef {
    std::string name;
    bool serverOnly = false;   // Only server can call
    bool clientOnly = false;   // Only clients can call
    bool ownerOnly = false;    // Only owner can call
    int luaFuncRef = -1;       // Lua function reference
};

// ===== Script Class =====
class ScriptClass {
public:
    std::string name;
    std::string sourceFile;
    std::string sourceCode;
    
    std::vector<ScriptProperty> properties;
    std::vector<ScriptRPCDef> rpcs;
    
    // Lua references
    int classTableRef = -1;
    int onStartRef = -1;
    int onUpdateRef = -1;
    int onDestroyRef = -1;
    int onNetworkSpawnRef = -1;
    int onNetworkDespawnRef = -1;
    
    ScriptProperty* getProperty(const std::string& propName) {
        for (auto& prop : properties) {
            if (prop.name == propName) return &prop;
        }
        return nullptr;
    }
    
    ScriptRPCDef* getRPC(const std::string& rpcName) {
        for (auto& rpc : rpcs) {
            if (rpc.name == rpcName) return &rpc;
        }
        return nullptr;
    }
};

// ===== Script Instance =====
class ScriptInstance {
public:
    ScriptInstance(ScriptClass* scriptClass, uint32_t entityId)
        : scriptClass_(scriptClass), entityId_(entityId) {}
    
    ScriptClass* getScriptClass() { return scriptClass_; }
    uint32_t getEntityId() const { return entityId_; }
    
    // Network
    void setNetworkId(uint32_t id) { networkId_ = id; }
    uint32_t getNetworkId() const { return networkId_; }
    
    void setOwnerConnection(ConnectionId owner) { ownerConnection_ = owner; }
    ConnectionId getOwnerConnection() const { return ownerConnection_; }
    
    bool hasAuthority() const {
        auto& netMgr = getNetworkManager();
        if (netMgr.isServer()) return true;
        if (ownerConnection_ == SERVER_CONNECTION && netMgr.isClient()) return false;
        // Client has authority over owned objects
        return true;  // Simplified
    }
    
    // Property values (local copy)
    std::unordered_map<std::string, ScriptValue> propertyValues;
    
    // Lua instance table reference
    int instanceRef = -1;
    
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    
private:
    ScriptClass* scriptClass_ = nullptr;
    uint32_t entityId_ = 0;
    uint32_t networkId_ = 0;
    ConnectionId ownerConnection_ = SERVER_CONNECTION;
    bool enabled_ = true;
};

// ===== Script Engine =====
class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();
    
    // Initialize/shutdown
    bool initialize();
    void shutdown();
    bool isInitialized() const { return L_ != nullptr; }
    
    // Load scripts
    bool loadScript(const std::string& filename);
    bool loadScriptString(const std::string& code, const std::string& name = "chunk");
    
    // Register script class
    ScriptClass* registerClass(const std::string& name);
    ScriptClass* getClass(const std::string& name);
    
    // Create instance
    ScriptInstance* createInstance(const std::string& className, uint32_t entityId);
    void destroyInstance(ScriptInstance* instance);
    
    // Update all instances
    void update(float dt);
    
    // Call script function
    bool callFunction(const std::string& funcName, const std::vector<ScriptValue>& args = {},
                      std::vector<ScriptValue>* results = nullptr);
    
    bool callMethod(ScriptInstance* instance, const std::string& methodName,
                    const std::vector<ScriptValue>& args = {},
                    std::vector<ScriptValue>* results = nullptr);
    
    // Network integration
    void setNetworkEnabled(bool enabled) { networkEnabled_ = enabled; }
    bool isNetworkEnabled() const { return networkEnabled_; }
    
    // Call RPC on instance
    void callRPC(ScriptInstance* instance, const std::string& rpcName,
                 const std::vector<ScriptValue>& args, ConnectionId target = BROADCAST_CONNECTION);
    
    // Handle incoming network RPC
    void handleNetworkRPC(uint32_t networkId, const std::string& rpcName,
                          ConnectionId sender, NetworkMessage& argsMsg);
    
    // Sync networked properties
    void syncNetworkedProperties(ScriptInstance* instance);
    void handlePropertySync(uint32_t networkId, NetworkMessage& msg);
    
    // Get instance by network ID
    ScriptInstance* getInstanceByNetworkId(uint32_t networkId);
    
    // Bind engine APIs
    void bindVec3();
    void bindQuat();
    void bindInput();
    void bindEntity();
    void bindNetwork();
    void bindDebug();
    
    // Error handling
    std::string getLastError() const { return lastError_; }
    
    // Accessors
    lua_State* getLuaState() { return L_; }
    const std::unordered_map<std::string, std::unique_ptr<ScriptClass>>& getClasses() const {
        return classes_;
    }
    
private:
    void pushValue(const ScriptValue& value);
    ScriptValue popValue();
    void registerNetworkHandlers();
    
    lua_State* L_ = nullptr;
    std::string lastError_;
    
    std::unordered_map<std::string, std::unique_ptr<ScriptClass>> classes_;
    std::vector<std::unique_ptr<ScriptInstance>> instances_;
    std::unordered_map<uint32_t, ScriptInstance*> networkIdToInstance_;
    
    uint32_t nextNetworkId_ = 1;
    bool networkEnabled_ = false;
};

// ===== Global Script Engine =====
inline ScriptEngine& getScriptEngine() {
    static ScriptEngine engine;
    return engine;
}

// ===== Script Component (for ECS integration) =====
struct ScriptComponent {
    std::string className;
    ScriptInstance* instance = nullptr;
    
    // Network sync settings
    bool networked = false;
    bool localPlayerAuthority = false;
};

}  // namespace luma
