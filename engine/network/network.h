// Network System - Core networking infrastructure
// Connection management, message serialization, state sync, RPC
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <queue>
#include <mutex>
#include <cstdint>
#include <cstring>

namespace luma {

// ===== Network Constants =====
constexpr uint32_t NETWORK_PROTOCOL_VERSION = 1;
constexpr uint32_t NETWORK_MAX_PACKET_SIZE = 1400;  // MTU-safe
constexpr uint32_t NETWORK_MAX_CONNECTIONS = 64;

// ===== Connection ID =====
using ConnectionId = uint32_t;
constexpr ConnectionId INVALID_CONNECTION = 0;
constexpr ConnectionId SERVER_CONNECTION = 1;
constexpr ConnectionId BROADCAST_CONNECTION = 0xFFFFFFFF;

// ===== Network Role =====
enum class NetworkRole {
    None,
    Client,
    Server,
    Host  // Server + local client
};

// ===== Message Type =====
enum class NetworkMessageType : uint8_t {
    // Connection
    Connect = 1,
    ConnectResponse = 2,
    Disconnect = 3,
    Heartbeat = 4,
    
    // State sync
    StateUpdate = 10,
    StateFull = 11,
    StateRequest = 12,
    
    // RPC
    RPC = 20,
    RPCResponse = 21,
    
    // Entity
    EntitySpawn = 30,
    EntityDestroy = 31,
    EntityOwnership = 32,
    
    // Script
    ScriptRPC = 40,
    ScriptStateSync = 41,
    
    // Custom
    Custom = 100
};

// ===== Network Message =====
class NetworkMessage {
public:
    NetworkMessage() = default;
    explicit NetworkMessage(NetworkMessageType type) : type_(type) {}
    
    NetworkMessageType getType() const { return type_; }
    void setType(NetworkMessageType type) { type_ = type; }
    
    // Write methods
    void writeByte(uint8_t value) { data_.push_back(value); }
    void writeUInt16(uint16_t value) {
        data_.push_back(value & 0xFF);
        data_.push_back((value >> 8) & 0xFF);
    }
    void writeUInt32(uint32_t value) {
        for (int i = 0; i < 4; i++) {
            data_.push_back((value >> (i * 8)) & 0xFF);
        }
    }
    void writeInt32(int32_t value) { writeUInt32((uint32_t)value); }
    void writeFloat(float value) {
        uint32_t bits;
        memcpy(&bits, &value, 4);
        writeUInt32(bits);
    }
    void writeString(const std::string& str) {
        writeUInt16((uint16_t)str.size());
        for (char c : str) {
            writeByte((uint8_t)c);
        }
    }
    void writeVec3(const Vec3& v) {
        writeFloat(v.x);
        writeFloat(v.y);
        writeFloat(v.z);
    }
    void writeBytes(const uint8_t* data, size_t size) {
        writeUInt32((uint32_t)size);
        data_.insert(data_.end(), data, data + size);
    }
    
    // Read methods
    uint8_t readByte() {
        if (readPos_ >= data_.size()) return 0;
        return data_[readPos_++];
    }
    uint16_t readUInt16() {
        uint16_t value = readByte();
        value |= (uint16_t)readByte() << 8;
        return value;
    }
    uint32_t readUInt32() {
        uint32_t value = 0;
        for (int i = 0; i < 4; i++) {
            value |= (uint32_t)readByte() << (i * 8);
        }
        return value;
    }
    int32_t readInt32() { return (int32_t)readUInt32(); }
    float readFloat() {
        uint32_t bits = readUInt32();
        float value;
        memcpy(&value, &bits, 4);
        return value;
    }
    std::string readString() {
        uint16_t len = readUInt16();
        std::string str;
        str.reserve(len);
        for (uint16_t i = 0; i < len; i++) {
            str += (char)readByte();
        }
        return str;
    }
    Vec3 readVec3() {
        return Vec3(readFloat(), readFloat(), readFloat());
    }
    std::vector<uint8_t> readBytes() {
        uint32_t size = readUInt32();
        std::vector<uint8_t> bytes(size);
        for (uint32_t i = 0; i < size; i++) {
            bytes[i] = readByte();
        }
        return bytes;
    }
    
    // Data access
    const std::vector<uint8_t>& getData() const { return data_; }
    size_t getSize() const { return data_.size(); }
    void clear() { data_.clear(); readPos_ = 0; }
    void resetRead() { readPos_ = 0; }
    
    // Serialization
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> packet;
        packet.reserve(5 + data_.size());
        
        // Header: type (1) + size (4)
        packet.push_back((uint8_t)type_);
        uint32_t size = (uint32_t)data_.size();
        for (int i = 0; i < 4; i++) {
            packet.push_back((size >> (i * 8)) & 0xFF);
        }
        
        // Data
        packet.insert(packet.end(), data_.begin(), data_.end());
        return packet;
    }
    
    static NetworkMessage deserialize(const uint8_t* data, size_t size) {
        if (size < 5) return NetworkMessage();
        
        NetworkMessage msg;
        msg.type_ = (NetworkMessageType)data[0];
        
        uint32_t dataSize = 0;
        for (int i = 0; i < 4; i++) {
            dataSize |= (uint32_t)data[1 + i] << (i * 8);
        }
        
        if (size >= 5 + dataSize) {
            msg.data_.assign(data + 5, data + 5 + dataSize);
        }
        
        return msg;
    }
    
private:
    NetworkMessageType type_ = NetworkMessageType::Custom;
    std::vector<uint8_t> data_;
    size_t readPos_ = 0;
};

// ===== Connection State =====
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting
};

// ===== Network Connection =====
struct NetworkConnection {
    ConnectionId id = INVALID_CONNECTION;
    ConnectionState state = ConnectionState::Disconnected;
    std::string address;
    uint16_t port = 0;
    
    double lastHeartbeat = 0.0;
    double roundTripTime = 0.0;
    uint64_t bytesSent = 0;
    uint64_t bytesReceived = 0;
    uint32_t packetsSent = 0;
    uint32_t packetsReceived = 0;
    uint32_t packetsLost = 0;
    
    // User data
    std::string username;
    uint32_t userId = 0;
    
    bool isConnected() const { return state == ConnectionState::Connected; }
};

// ===== RPC Definition =====
struct RPCDefinition {
    std::string name;
    uint32_t id;
    bool serverOnly;  // Only server can call
    bool clientOnly;  // Only clients can call
    bool requiresOwnership;  // Must own the entity to call
    
    using Handler = std::function<void(ConnectionId sender, NetworkMessage& args)>;
    Handler handler;
};

// ===== Network Peer (Base for Client/Server) =====
class NetworkPeer {
public:
    virtual ~NetworkPeer() = default;
    
    virtual bool start(const std::string& address, uint16_t port) = 0;
    virtual void stop() = 0;
    virtual void update(double dt) = 0;
    
    virtual void send(ConnectionId target, const NetworkMessage& msg) = 0;
    virtual void broadcast(const NetworkMessage& msg) = 0;
    
    virtual NetworkRole getRole() const = 0;
    virtual bool isRunning() const = 0;
    
    // Connection management
    const std::unordered_map<ConnectionId, NetworkConnection>& getConnections() const {
        return connections_;
    }
    
    NetworkConnection* getConnection(ConnectionId id) {
        auto it = connections_.find(id);
        return it != connections_.end() ? &it->second : nullptr;
    }
    
    // Message handlers
    using MessageHandler = std::function<void(ConnectionId sender, NetworkMessage& msg)>;
    
    void setMessageHandler(NetworkMessageType type, MessageHandler handler) {
        messageHandlers_[type] = handler;
    }
    
    // RPC
    void registerRPC(const std::string& name, RPCDefinition::Handler handler,
                     bool serverOnly = false, bool clientOnly = false) {
        RPCDefinition rpc;
        rpc.name = name;
        rpc.id = nextRPCId_++;
        rpc.serverOnly = serverOnly;
        rpc.clientOnly = clientOnly;
        rpc.handler = handler;
        rpcDefinitions_[rpc.name] = rpc;
        rpcById_[rpc.id] = &rpcDefinitions_[rpc.name];
    }
    
    void callRPC(ConnectionId target, const std::string& name, NetworkMessage& args) {
        auto it = rpcDefinitions_.find(name);
        if (it == rpcDefinitions_.end()) return;
        
        NetworkMessage msg(NetworkMessageType::RPC);
        msg.writeUInt32(it->second.id);
        msg.writeBytes(args.getData().data(), args.getData().size());
        
        if (target == BROADCAST_CONNECTION) {
            broadcast(msg);
        } else {
            send(target, msg);
        }
    }
    
    // Callbacks
    using ConnectionCallback = std::function<void(ConnectionId id, const NetworkConnection& conn)>;
    void setOnConnect(ConnectionCallback cb) { onConnect_ = cb; }
    void setOnDisconnect(ConnectionCallback cb) { onDisconnect_ = cb; }
    
protected:
    void handleMessage(ConnectionId sender, NetworkMessage& msg) {
        // Handle RPC
        if (msg.getType() == NetworkMessageType::RPC) {
            uint32_t rpcId = msg.readUInt32();
            auto it = rpcById_.find(rpcId);
            if (it != rpcById_.end() && it->second->handler) {
                NetworkMessage args;
                auto data = msg.readBytes();
                for (uint8_t b : data) args.writeByte(b);
                args.resetRead();
                it->second->handler(sender, args);
            }
            return;
        }
        
        // Custom handlers
        auto it = messageHandlers_.find(msg.getType());
        if (it != messageHandlers_.end()) {
            it->second(sender, msg);
        }
    }
    
    std::unordered_map<ConnectionId, NetworkConnection> connections_;
    std::unordered_map<NetworkMessageType, MessageHandler> messageHandlers_;
    std::unordered_map<std::string, RPCDefinition> rpcDefinitions_;
    std::unordered_map<uint32_t, RPCDefinition*> rpcById_;
    uint32_t nextRPCId_ = 1;
    
    ConnectionCallback onConnect_;
    ConnectionCallback onDisconnect_;
};

// ===== Network Server =====
class NetworkServer : public NetworkPeer {
public:
    bool start(const std::string& address, uint16_t port) override {
        address_ = address;
        port_ = port;
        running_ = true;
        nextConnectionId_ = 2;  // 1 is reserved for server
        
        // In real implementation, would create socket and bind
        return true;
    }
    
    void stop() override {
        running_ = false;
        for (auto& [id, conn] : connections_) {
            NetworkMessage msg(NetworkMessageType::Disconnect);
            send(id, msg);
        }
        connections_.clear();
    }
    
    void update(double dt) override {
        if (!running_) return;
        
        // Process incoming packets (simulated)
        // In real implementation, would poll socket
        
        // Send heartbeats
        heartbeatTimer_ += dt;
        if (heartbeatTimer_ >= 1.0) {
            heartbeatTimer_ = 0.0;
            NetworkMessage heartbeat(NetworkMessageType::Heartbeat);
            broadcast(heartbeat);
        }
        
        // Check for timeouts
        double now = 0.0;  // Would use actual time
        for (auto it = connections_.begin(); it != connections_.end(); ) {
            if (now - it->second.lastHeartbeat > 10.0) {
                if (onDisconnect_) onDisconnect_(it->first, it->second);
                it = connections_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void send(ConnectionId target, const NetworkMessage& msg) override {
        auto it = connections_.find(target);
        if (it == connections_.end()) return;
        
        auto packet = msg.serialize();
        it->second.bytesSent += packet.size();
        it->second.packetsSent++;
        
        // In real implementation, would send via socket
        pendingOutgoing_.push({target, packet});
    }
    
    void broadcast(const NetworkMessage& msg) override {
        for (auto& [id, conn] : connections_) {
            send(id, msg);
        }
    }
    
    NetworkRole getRole() const override { return NetworkRole::Server; }
    bool isRunning() const override { return running_; }
    
    // Accept connection (called when client connects)
    ConnectionId acceptConnection(const std::string& address, uint16_t port) {
        ConnectionId id = nextConnectionId_++;
        
        NetworkConnection conn;
        conn.id = id;
        conn.address = address;
        conn.port = port;
        conn.state = ConnectionState::Connected;
        conn.lastHeartbeat = 0.0;
        
        connections_[id] = conn;
        
        if (onConnect_) onConnect_(id, conn);
        
        return id;
    }
    
    void disconnectClient(ConnectionId id) {
        auto it = connections_.find(id);
        if (it != connections_.end()) {
            NetworkMessage msg(NetworkMessageType::Disconnect);
            send(id, msg);
            
            if (onDisconnect_) onDisconnect_(id, it->second);
            connections_.erase(it);
        }
    }
    
    size_t getClientCount() const { return connections_.size(); }
    
private:
    std::string address_;
    uint16_t port_ = 0;
    bool running_ = false;
    ConnectionId nextConnectionId_ = 2;
    double heartbeatTimer_ = 0.0;
    
    std::queue<std::pair<ConnectionId, std::vector<uint8_t>>> pendingOutgoing_;
};

// ===== Network Client =====
class NetworkClient : public NetworkPeer {
public:
    bool start(const std::string& address, uint16_t port) override {
        serverAddress_ = address;
        serverPort_ = port;
        
        // Create connection to server
        NetworkConnection conn;
        conn.id = SERVER_CONNECTION;
        conn.address = address;
        conn.port = port;
        conn.state = ConnectionState::Connecting;
        connections_[SERVER_CONNECTION] = conn;
        
        // Send connect request
        NetworkMessage msg(NetworkMessageType::Connect);
        msg.writeUInt32(NETWORK_PROTOCOL_VERSION);
        send(SERVER_CONNECTION, msg);
        
        running_ = true;
        return true;
    }
    
    void stop() override {
        if (running_) {
            NetworkMessage msg(NetworkMessageType::Disconnect);
            send(SERVER_CONNECTION, msg);
        }
        running_ = false;
        connections_.clear();
    }
    
    void update(double dt) override {
        if (!running_) return;
        
        // Process incoming packets
        // In real implementation, would poll socket
        
        // Send heartbeat
        heartbeatTimer_ += dt;
        if (heartbeatTimer_ >= 1.0) {
            heartbeatTimer_ = 0.0;
            NetworkMessage heartbeat(NetworkMessageType::Heartbeat);
            send(SERVER_CONNECTION, heartbeat);
        }
    }
    
    void send(ConnectionId target, const NetworkMessage& msg) override {
        // Client only sends to server
        auto packet = msg.serialize();
        
        auto* conn = getConnection(SERVER_CONNECTION);
        if (conn) {
            conn->bytesSent += packet.size();
            conn->packetsSent++;
        }
        
        // In real implementation, would send via socket
        pendingOutgoing_.push(packet);
    }
    
    void broadcast(const NetworkMessage& msg) override {
        // Client broadcast is just send to server
        send(SERVER_CONNECTION, msg);
    }
    
    NetworkRole getRole() const override { return NetworkRole::Client; }
    bool isRunning() const override { return running_; }
    
    bool isConnected() const {
        auto* conn = const_cast<NetworkClient*>(this)->getConnection(SERVER_CONNECTION);
        return conn && conn->isConnected();
    }
    
    ConnectionState getConnectionState() const {
        auto* conn = const_cast<NetworkClient*>(this)->getConnection(SERVER_CONNECTION);
        return conn ? conn->state : ConnectionState::Disconnected;
    }
    
private:
    std::string serverAddress_;
    uint16_t serverPort_ = 0;
    bool running_ = false;
    double heartbeatTimer_ = 0.0;
    
    std::queue<std::vector<uint8_t>> pendingOutgoing_;
};

// ===== Network Manager =====
class NetworkManager {
public:
    static NetworkManager& get() {
        static NetworkManager instance;
        return instance;
    }
    
    // Start as server
    bool startServer(uint16_t port) {
        stop();
        server_ = std::make_unique<NetworkServer>();
        if (!server_->start("0.0.0.0", port)) {
            server_.reset();
            return false;
        }
        role_ = NetworkRole::Server;
        return true;
    }
    
    // Start as client
    bool startClient(const std::string& address, uint16_t port) {
        stop();
        client_ = std::make_unique<NetworkClient>();
        if (!client_->start(address, port)) {
            client_.reset();
            return false;
        }
        role_ = NetworkRole::Client;
        return true;
    }
    
    // Start as host (server + local client)
    bool startHost(uint16_t port) {
        if (!startServer(port)) return false;
        role_ = NetworkRole::Host;
        return true;
    }
    
    void stop() {
        if (server_) server_->stop();
        if (client_) client_->stop();
        server_.reset();
        client_.reset();
        role_ = NetworkRole::None;
    }
    
    void update(double dt) {
        if (server_) server_->update(dt);
        if (client_) client_->update(dt);
    }
    
    // Get active peer
    NetworkPeer* getPeer() {
        if (server_) return server_.get();
        if (client_) return client_.get();
        return nullptr;
    }
    
    NetworkServer* getServer() { return server_.get(); }
    NetworkClient* getClient() { return client_.get(); }
    
    NetworkRole getRole() const { return role_; }
    bool isServer() const { return role_ == NetworkRole::Server || role_ == NetworkRole::Host; }
    bool isClient() const { return role_ == NetworkRole::Client; }
    bool isHost() const { return role_ == NetworkRole::Host; }
    bool isActive() const { return role_ != NetworkRole::None; }
    
    // Convenience: send/RPC
    void send(ConnectionId target, const NetworkMessage& msg) {
        if (auto* peer = getPeer()) peer->send(target, msg);
    }
    
    void broadcast(const NetworkMessage& msg) {
        if (auto* peer = getPeer()) peer->broadcast(msg);
    }
    
    void registerRPC(const std::string& name, RPCDefinition::Handler handler,
                     bool serverOnly = false, bool clientOnly = false) {
        // Register on both server and client if available
        if (server_) server_->registerRPC(name, handler, serverOnly, clientOnly);
        if (client_) client_->registerRPC(name, handler, serverOnly, clientOnly);
    }
    
    void callRPC(ConnectionId target, const std::string& name, NetworkMessage& args) {
        if (auto* peer = getPeer()) peer->callRPC(target, name, args);
    }
    
private:
    NetworkManager() = default;
    
    std::unique_ptr<NetworkServer> server_;
    std::unique_ptr<NetworkClient> client_;
    NetworkRole role_ = NetworkRole::None;
};

// ===== Convenience =====
inline NetworkManager& getNetworkManager() { return NetworkManager::get(); }

}  // namespace luma
