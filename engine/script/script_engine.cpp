// Script Engine Implementation
// Note: This is a stub implementation. In production, would link against Lua library.

#include "script_engine.h"
#include <cstring>

// Mock Lua types for compilation without Lua library
#ifndef LUA_VERSION
struct lua_State { int dummy; };
#define LUA_REGISTRYINDEX (-10000)
#define LUA_GLOBALSINDEX (-10002)
#define LUA_OK 0
#define LUA_MULTRET (-1)
#define LUA_TNUMBER 3
#define LUA_TSTRING 4
#define LUA_TTABLE 5
#define LUA_TFUNCTION 6
#define LUA_TBOOLEAN 1
#define LUA_TNIL 0

// Mock Lua functions
inline lua_State* luaL_newstate() { return new lua_State(); }
inline void lua_close(lua_State* L) { delete L; }
inline void luaL_openlibs(lua_State*) {}
inline int luaL_loadstring(lua_State*, const char*) { return LUA_OK; }
inline int lua_pcall(lua_State*, int, int, int) { return LUA_OK; }
inline void lua_pushnil(lua_State*) {}
inline void lua_pushboolean(lua_State*, int) {}
inline void lua_pushnumber(lua_State*, double) {}
inline void lua_pushstring(lua_State*, const char*) {}
inline void lua_newtable(lua_State*) {}
inline void lua_settable(lua_State*, int) {}
inline void lua_gettable(lua_State*, int) {}
inline void lua_setglobal(lua_State*, const char*) {}
inline void lua_getglobal(lua_State*, const char*) {}
inline int lua_type(lua_State*, int) { return LUA_TNIL; }
inline int lua_toboolean(lua_State*, int) { return 0; }
inline double lua_tonumber(lua_State*, int) { return 0; }
inline const char* lua_tostring(lua_State*, int) { return ""; }
inline void lua_pop(lua_State*, int) {}
inline int lua_gettop(lua_State*) { return 0; }
inline void lua_settop(lua_State*, int) {}
inline int luaL_ref(lua_State*, int) { return -1; }
inline void luaL_unref(lua_State*, int, int) {}
inline void lua_rawgeti(lua_State*, int, int) {}
inline void lua_pushvalue(lua_State*, int) {}
inline void lua_setfield(lua_State*, int, const char*) {}
inline void lua_getfield(lua_State*, int, const char*) {}
inline void lua_createtable(lua_State*, int, int) {}
inline void lua_rawset(lua_State*, int) {}
inline void lua_rawseti(lua_State*, int, int) {}
inline int lua_isnil(lua_State*, int) { return 1; }
inline int lua_isnumber(lua_State*, int) { return 0; }
inline int lua_isstring(lua_State*, int) { return 0; }
inline int lua_isfunction(lua_State*, int) { return 0; }
inline int lua_istable(lua_State*, int) { return 0; }
inline int lua_iscfunction(lua_State*, int) { return 0; }
inline void lua_pushcfunction(lua_State*, int(*)(lua_State*)) {}
inline void lua_register(lua_State* L, const char* n, int(*f)(lua_State*)) {}
inline void* lua_newuserdata(lua_State*, size_t) { return nullptr; }
inline void lua_setmetatable(lua_State*, int) {}
inline void luaL_newmetatable(lua_State*, const char*) {}
inline void luaL_setmetatable(lua_State*, const char*) {}
inline void* luaL_checkudata(lua_State*, int, const char*) { return nullptr; }
inline int luaL_error(lua_State*, const char*, ...) { return 0; }
#endif

namespace luma {

ScriptEngine::ScriptEngine() = default;

ScriptEngine::~ScriptEngine() {
    shutdown();
}

bool ScriptEngine::initialize() {
    if (L_) return true;
    
    L_ = luaL_newstate();
    if (!L_) {
        lastError_ = "Failed to create Lua state";
        return false;
    }
    
    luaL_openlibs(L_);
    
    // Bind engine APIs
    bindVec3();
    bindQuat();
    bindInput();
    bindEntity();
    bindNetwork();
    bindDebug();
    
    // Register network handlers
    if (networkEnabled_) {
        registerNetworkHandlers();
    }
    
    return true;
}

void ScriptEngine::shutdown() {
    // Destroy all instances
    for (auto& instance : instances_) {
        if (instance->instanceRef >= 0) {
            luaL_unref(L_, LUA_REGISTRYINDEX, instance->instanceRef);
        }
    }
    instances_.clear();
    networkIdToInstance_.clear();
    
    // Unref class tables
    for (auto& [name, cls] : classes_) {
        if (cls->classTableRef >= 0) {
            luaL_unref(L_, LUA_REGISTRYINDEX, cls->classTableRef);
        }
    }
    classes_.clear();
    
    if (L_) {
        lua_close(L_);
        L_ = nullptr;
    }
}

bool ScriptEngine::loadScript(const std::string& filename) {
    // In production, would read file and call loadScriptString
    lastError_ = "File loading not implemented in stub";
    return false;
}

bool ScriptEngine::loadScriptString(const std::string& code, const std::string& name) {
    if (!L_) {
        lastError_ = "Lua not initialized";
        return false;
    }
    
    int result = luaL_loadstring(L_, code.c_str());
    if (result != LUA_OK) {
        lastError_ = lua_tostring(L_, -1);
        lua_pop(L_, 1);
        return false;
    }
    
    result = lua_pcall(L_, 0, 0, 0);
    if (result != LUA_OK) {
        lastError_ = lua_tostring(L_, -1);
        lua_pop(L_, 1);
        return false;
    }
    
    return true;
}

ScriptClass* ScriptEngine::registerClass(const std::string& name) {
    auto cls = std::make_unique<ScriptClass>();
    cls->name = name;
    
    ScriptClass* ptr = cls.get();
    classes_[name] = std::move(cls);
    
    return ptr;
}

ScriptClass* ScriptEngine::getClass(const std::string& name) {
    auto it = classes_.find(name);
    return it != classes_.end() ? it->second.get() : nullptr;
}

ScriptInstance* ScriptEngine::createInstance(const std::string& className, uint32_t entityId) {
    ScriptClass* cls = getClass(className);
    if (!cls) {
        lastError_ = "Class not found: " + className;
        return nullptr;
    }
    
    auto instance = std::make_unique<ScriptInstance>(cls, entityId);
    
    // Assign network ID if networking enabled
    if (networkEnabled_) {
        instance->setNetworkId(nextNetworkId_++);
        networkIdToInstance_[instance->getNetworkId()] = instance.get();
    }
    
    // Initialize property values from class defaults
    for (const auto& prop : cls->properties) {
        instance->propertyValues[prop.name] = prop.value;
    }
    
    // Create Lua instance table
    if (L_) {
        lua_newtable(L_);
        instance->instanceRef = luaL_ref(L_, LUA_REGISTRYINDEX);
        
        // Call onStart if defined
        if (cls->onStartRef >= 0) {
            lua_rawgeti(L_, LUA_REGISTRYINDEX, cls->onStartRef);
            lua_rawgeti(L_, LUA_REGISTRYINDEX, instance->instanceRef);
            lua_pcall(L_, 1, 0, 0);
        }
    }
    
    ScriptInstance* ptr = instance.get();
    instances_.push_back(std::move(instance));
    
    return ptr;
}

void ScriptEngine::destroyInstance(ScriptInstance* instance) {
    if (!instance) return;
    
    // Call onDestroy
    ScriptClass* cls = instance->getScriptClass();
    if (cls && cls->onDestroyRef >= 0 && L_) {
        lua_rawgeti(L_, LUA_REGISTRYINDEX, cls->onDestroyRef);
        lua_rawgeti(L_, LUA_REGISTRYINDEX, instance->instanceRef);
        lua_pcall(L_, 1, 0, 0);
    }
    
    // Remove from network map
    if (instance->getNetworkId() != 0) {
        networkIdToInstance_.erase(instance->getNetworkId());
    }
    
    // Unref Lua table
    if (instance->instanceRef >= 0 && L_) {
        luaL_unref(L_, LUA_REGISTRYINDEX, instance->instanceRef);
    }
    
    // Remove from instances
    instances_.erase(
        std::remove_if(instances_.begin(), instances_.end(),
            [instance](const auto& inst) { return inst.get() == instance; }),
        instances_.end()
    );
}

void ScriptEngine::update(float dt) {
    if (!L_) return;
    
    for (auto& instance : instances_) {
        if (!instance->isEnabled()) continue;
        
        ScriptClass* cls = instance->getScriptClass();
        if (!cls || cls->onUpdateRef < 0) continue;
        
        // Call onUpdate(self, dt)
        lua_rawgeti(L_, LUA_REGISTRYINDEX, cls->onUpdateRef);
        lua_rawgeti(L_, LUA_REGISTRYINDEX, instance->instanceRef);
        lua_pushnumber(L_, dt);
        lua_pcall(L_, 2, 0, 0);
    }
    
    // Sync networked properties
    if (networkEnabled_ && getNetworkManager().isServer()) {
        for (auto& instance : instances_) {
            syncNetworkedProperties(instance.get());
        }
    }
}

bool ScriptEngine::callFunction(const std::string& funcName, const std::vector<ScriptValue>& args,
                                std::vector<ScriptValue>* results) {
    if (!L_) return false;
    
    lua_getglobal(L_, funcName.c_str());
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }
    
    for (const auto& arg : args) {
        pushValue(arg);
    }
    
    int nresults = results ? LUA_MULTRET : 0;
    if (lua_pcall(L_, (int)args.size(), nresults, 0) != LUA_OK) {
        lastError_ = lua_tostring(L_, -1);
        lua_pop(L_, 1);
        return false;
    }
    
    if (results) {
        int n = lua_gettop(L_);
        for (int i = 1; i <= n; i++) {
            results->push_back(popValue());
        }
    }
    
    return true;
}

bool ScriptEngine::callMethod(ScriptInstance* instance, const std::string& methodName,
                              const std::vector<ScriptValue>& args,
                              std::vector<ScriptValue>* results) {
    if (!L_ || !instance) return false;
    
    // Get instance table
    lua_rawgeti(L_, LUA_REGISTRYINDEX, instance->instanceRef);
    lua_getfield(L_, -1, methodName.c_str());
    
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 2);
        return false;
    }
    
    // Push self
    lua_rawgeti(L_, LUA_REGISTRYINDEX, instance->instanceRef);
    
    // Push args
    for (const auto& arg : args) {
        pushValue(arg);
    }
    
    int nresults = results ? LUA_MULTRET : 0;
    if (lua_pcall(L_, (int)args.size() + 1, nresults, 0) != LUA_OK) {
        lastError_ = lua_tostring(L_, -1);
        lua_pop(L_, 2);
        return false;
    }
    
    lua_pop(L_, 1);  // Pop instance table
    
    if (results) {
        int n = lua_gettop(L_);
        for (int i = 1; i <= n; i++) {
            results->push_back(popValue());
        }
    }
    
    return true;
}

void ScriptEngine::callRPC(ScriptInstance* instance, const std::string& rpcName,
                           const std::vector<ScriptValue>& args, ConnectionId target) {
    if (!instance || !networkEnabled_) return;
    
    ScriptClass* cls = instance->getScriptClass();
    if (!cls) return;
    
    ScriptRPCDef* rpcDef = cls->getRPC(rpcName);
    if (!rpcDef) return;
    
    // Check authority
    auto& netMgr = getNetworkManager();
    if (rpcDef->serverOnly && !netMgr.isServer()) return;
    if (rpcDef->clientOnly && netMgr.isServer()) return;
    
    // Build network message
    NetworkMessage msg(NetworkMessageType::ScriptRPC);
    msg.writeUInt32(instance->getNetworkId());
    msg.writeString(rpcName);
    msg.writeUInt16((uint16_t)args.size());
    
    for (const auto& arg : args) {
        arg.serialize(msg);
    }
    
    // Send
    if (target == BROADCAST_CONNECTION) {
        netMgr.broadcast(msg);
    } else {
        netMgr.send(target, msg);
    }
}

void ScriptEngine::handleNetworkRPC(uint32_t networkId, const std::string& rpcName,
                                     ConnectionId sender, NetworkMessage& argsMsg) {
    ScriptInstance* instance = getInstanceByNetworkId(networkId);
    if (!instance) return;
    
    ScriptClass* cls = instance->getScriptClass();
    if (!cls) return;
    
    ScriptRPCDef* rpcDef = cls->getRPC(rpcName);
    if (!rpcDef || rpcDef->luaFuncRef < 0) return;
    
    // Deserialize args
    uint16_t argCount = argsMsg.readUInt16();
    std::vector<ScriptValue> args;
    for (uint16_t i = 0; i < argCount; i++) {
        args.push_back(ScriptValue::deserialize(argsMsg));
    }
    
    // Call the RPC function
    if (L_) {
        lua_rawgeti(L_, LUA_REGISTRYINDEX, rpcDef->luaFuncRef);
        lua_rawgeti(L_, LUA_REGISTRYINDEX, instance->instanceRef);
        
        for (const auto& arg : args) {
            pushValue(arg);
        }
        
        lua_pcall(L_, (int)args.size() + 1, 0, 0);
    }
}

void ScriptEngine::syncNetworkedProperties(ScriptInstance* instance) {
    if (!instance || !networkEnabled_) return;
    
    ScriptClass* cls = instance->getScriptClass();
    if (!cls) return;
    
    // Build sync message with dirty properties
    NetworkMessage msg(NetworkMessageType::ScriptStateSync);
    msg.writeUInt32(instance->getNetworkId());
    
    uint16_t propCount = 0;
    size_t countPos = msg.getSize();
    msg.writeUInt16(0);  // Placeholder
    
    for (auto& prop : cls->properties) {
        if (!prop.networked || !prop.dirty) continue;
        
        auto it = instance->propertyValues.find(prop.name);
        if (it == instance->propertyValues.end()) continue;
        
        msg.writeString(prop.name);
        it->second.serialize(msg);
        propCount++;
        prop.dirty = false;
    }
    
    if (propCount > 0) {
        // Update count (would need to modify message in real impl)
        getNetworkManager().broadcast(msg);
    }
}

void ScriptEngine::handlePropertySync(uint32_t networkId, NetworkMessage& msg) {
    ScriptInstance* instance = getInstanceByNetworkId(networkId);
    if (!instance) return;
    
    uint16_t propCount = msg.readUInt16();
    
    for (uint16_t i = 0; i < propCount; i++) {
        std::string propName = msg.readString();
        ScriptValue value = ScriptValue::deserialize(msg);
        
        instance->propertyValues[propName] = value;
        
        // Update Lua table
        if (L_ && instance->instanceRef >= 0) {
            lua_rawgeti(L_, LUA_REGISTRYINDEX, instance->instanceRef);
            pushValue(value);
            lua_setfield(L_, -2, propName.c_str());
            lua_pop(L_, 1);
        }
    }
}

ScriptInstance* ScriptEngine::getInstanceByNetworkId(uint32_t networkId) {
    auto it = networkIdToInstance_.find(networkId);
    return it != networkIdToInstance_.end() ? it->second : nullptr;
}

void ScriptEngine::pushValue(const ScriptValue& value) {
    if (!L_) return;
    
    switch (value.type) {
        case ScriptValueType::None:
            lua_pushnil(L_);
            break;
        case ScriptValueType::Boolean:
            lua_pushboolean(L_, value.boolValue);
            break;
        case ScriptValueType::Number:
            lua_pushnumber(L_, value.numberValue);
            break;
        case ScriptValueType::String:
            lua_pushstring(L_, value.stringValue.c_str());
            break;
        case ScriptValueType::Vec3:
            // Would create Vec3 userdata
            lua_newtable(L_);
            lua_pushnumber(L_, value.vec3Value.x);
            lua_setfield(L_, -2, "x");
            lua_pushnumber(L_, value.vec3Value.y);
            lua_setfield(L_, -2, "y");
            lua_pushnumber(L_, value.vec3Value.z);
            lua_setfield(L_, -2, "z");
            break;
        default:
            lua_pushnil(L_);
            break;
    }
}

ScriptValue ScriptEngine::popValue() {
    if (!L_) return ScriptValue();
    
    ScriptValue value;
    int t = lua_type(L_, -1);
    
    switch (t) {
        case LUA_TBOOLEAN:
            value = ScriptValue(lua_toboolean(L_, -1) != 0);
            break;
        case LUA_TNUMBER:
            value = ScriptValue(lua_tonumber(L_, -1));
            break;
        case LUA_TSTRING:
            value = ScriptValue(std::string(lua_tostring(L_, -1)));
            break;
        default:
            break;
    }
    
    lua_pop(L_, 1);
    return value;
}

void ScriptEngine::registerNetworkHandlers() {
    auto& netMgr = getNetworkManager();
    auto* peer = netMgr.getPeer();
    if (!peer) return;
    
    // Handle incoming script RPC
    peer->setMessageHandler(NetworkMessageType::ScriptRPC,
        [this](ConnectionId sender, NetworkMessage& msg) {
            uint32_t networkId = msg.readUInt32();
            std::string rpcName = msg.readString();
            handleNetworkRPC(networkId, rpcName, sender, msg);
        });
    
    // Handle property sync
    peer->setMessageHandler(NetworkMessageType::ScriptStateSync,
        [this](ConnectionId sender, NetworkMessage& msg) {
            uint32_t networkId = msg.readUInt32();
            handlePropertySync(networkId, msg);
        });
}

// Binding implementations (stubs)
void ScriptEngine::bindVec3() {
    // In production: register Vec3 userdata type with metamethods
}

void ScriptEngine::bindQuat() {
    // In production: register Quat userdata type with metamethods
}

void ScriptEngine::bindInput() {
    // In production: register input functions
}

void ScriptEngine::bindEntity() {
    // In production: register entity/component access
}

void ScriptEngine::bindNetwork() {
    // In production: register network functions
    // - Network.isServer()
    // - Network.isClient()
    // - Network.hasAuthority()
    // - Network.rpc(target, name, args...)
}

void ScriptEngine::bindDebug() {
    // In production: register debug/print functions
}

}  // namespace luma
