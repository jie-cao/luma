// Lightweight JSON Parser and Writer
// Header-only implementation for LUMA Engine
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdexcept>
#include <cmath>

namespace luma {

// Forward declaration
class JsonValue;
using JsonObject = std::unordered_map<std::string, JsonValue>;
using JsonArray = std::vector<JsonValue>;

// JSON Value - can hold null, bool, number, string, array, or object
class JsonValue {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };
    
    using Value = std::variant<
        std::nullptr_t,
        bool,
        double,
        std::string,
        std::shared_ptr<JsonArray>,
        std::shared_ptr<JsonObject>
    >;
    
private:
    Value value_;
    
public:
    // Constructors
    JsonValue() : value_(nullptr) {}
    JsonValue(std::nullptr_t) : value_(nullptr) {}
    JsonValue(bool b) : value_(b) {}
    JsonValue(int n) : value_(static_cast<double>(n)) {}
    JsonValue(float n) : value_(static_cast<double>(n)) {}
    JsonValue(double n) : value_(n) {}
    JsonValue(const char* s) : value_(std::string(s)) {}
    JsonValue(const std::string& s) : value_(s) {}
    JsonValue(std::string&& s) : value_(std::move(s)) {}
    
    // Array/Object constructors
    JsonValue(const JsonArray& arr) : value_(std::make_shared<JsonArray>(arr)) {}
    JsonValue(JsonArray&& arr) : value_(std::make_shared<JsonArray>(std::move(arr))) {}
    JsonValue(const JsonObject& obj) : value_(std::make_shared<JsonObject>(obj)) {}
    JsonValue(JsonObject&& obj) : value_(std::make_shared<JsonObject>(std::move(obj))) {}
    
    // Type checking
    Type type() const {
        return static_cast<Type>(value_.index());
    }
    
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
    bool isBool() const { return std::holds_alternative<bool>(value_); }
    bool isNumber() const { return std::holds_alternative<double>(value_); }
    bool isString() const { return std::holds_alternative<std::string>(value_); }
    bool isArray() const { return std::holds_alternative<std::shared_ptr<JsonArray>>(value_); }
    bool isObject() const { return std::holds_alternative<std::shared_ptr<JsonObject>>(value_); }
    
    // Value getters
    bool asBool(bool defaultVal = false) const {
        if (isBool()) return std::get<bool>(value_);
        return defaultVal;
    }
    
    double asNumber(double defaultVal = 0.0) const {
        if (isNumber()) return std::get<double>(value_);
        return defaultVal;
    }
    
    int asInt(int defaultVal = 0) const {
        return static_cast<int>(asNumber(defaultVal));
    }
    
    float asFloat(float defaultVal = 0.0f) const {
        return static_cast<float>(asNumber(defaultVal));
    }
    
    const std::string& asString(const std::string& defaultVal = "") const {
        static std::string empty;
        if (isString()) return std::get<std::string>(value_);
        if (!defaultVal.empty()) return defaultVal;
        return empty;
    }
    
    const JsonArray& asArray() const {
        static JsonArray empty;
        if (isArray()) return *std::get<std::shared_ptr<JsonArray>>(value_);
        return empty;
    }
    
    JsonArray& asArray() {
        if (!isArray()) {
            value_ = std::make_shared<JsonArray>();
        }
        return *std::get<std::shared_ptr<JsonArray>>(value_);
    }
    
    const JsonObject& asObject() const {
        static JsonObject empty;
        if (isObject()) return *std::get<std::shared_ptr<JsonObject>>(value_);
        return empty;
    }
    
    JsonObject& asObject() {
        if (!isObject()) {
            value_ = std::make_shared<JsonObject>();
        }
        return *std::get<std::shared_ptr<JsonObject>>(value_);
    }
    
    // Array/Object accessors
    JsonValue& operator[](size_t index) {
        return asArray()[index];
    }
    
    const JsonValue& operator[](size_t index) const {
        return asArray()[index];
    }
    
    JsonValue& operator[](const std::string& key) {
        return asObject()[key];
    }
    
    const JsonValue& operator[](const std::string& key) const {
        static JsonValue null;
        const auto& obj = asObject();
        auto it = obj.find(key);
        if (it != obj.end()) return it->second;
        return null;
    }
    
    // Check if key exists
    bool has(const std::string& key) const {
        if (!isObject()) return false;
        return asObject().count(key) > 0;
    }
    
    // Get value with default
    template<typename T>
    T get(const std::string& key, const T& defaultVal) const;
    
    // Array size
    size_t size() const {
        if (isArray()) return asArray().size();
        if (isObject()) return asObject().size();
        return 0;
    }
    
    // Add to array
    void push(const JsonValue& val) {
        asArray().push_back(val);
    }
    
    // Static factory methods
    static JsonValue array() { return JsonValue(JsonArray{}); }
    static JsonValue object() { return JsonValue(JsonObject{}); }
};

// Template specializations for get<T>
template<> inline bool JsonValue::get<bool>(const std::string& key, const bool& defaultVal) const {
    return (*this)[key].asBool(defaultVal);
}
template<> inline int JsonValue::get<int>(const std::string& key, const int& defaultVal) const {
    return (*this)[key].asInt(defaultVal);
}
template<> inline float JsonValue::get<float>(const std::string& key, const float& defaultVal) const {
    return (*this)[key].asFloat(defaultVal);
}
template<> inline double JsonValue::get<double>(const std::string& key, const double& defaultVal) const {
    return (*this)[key].asNumber(defaultVal);
}
template<> inline std::string JsonValue::get<std::string>(const std::string& key, const std::string& defaultVal) const {
    const auto& val = (*this)[key];
    if (val.isString()) return val.asString();
    return defaultVal;
}

// ===== JSON Parser =====
class JsonParser {
    const std::string& input_;
    size_t pos_ = 0;
    
    char peek() const {
        return pos_ < input_.size() ? input_[pos_] : '\0';
    }
    
    char get() {
        return pos_ < input_.size() ? input_[pos_++] : '\0';
    }
    
    void skipWhitespace() {
        while (pos_ < input_.size() && std::isspace(input_[pos_])) {
            pos_++;
        }
    }
    
    std::string parseString() {
        if (get() != '"') throw std::runtime_error("Expected '\"'");
        
        std::string result;
        while (true) {
            char c = get();
            if (c == '"') break;
            if (c == '\\') {
                c = get();
                switch (c) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u': {
                        // Unicode escape
                        std::string hex;
                        for (int i = 0; i < 4; i++) hex += get();
                        int codepoint = std::stoi(hex, nullptr, 16);
                        if (codepoint < 0x80) {
                            result += static_cast<char>(codepoint);
                        } else if (codepoint < 0x800) {
                            result += static_cast<char>(0xC0 | (codepoint >> 6));
                            result += static_cast<char>(0x80 | (codepoint & 0x3F));
                        } else {
                            result += static_cast<char>(0xE0 | (codepoint >> 12));
                            result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (codepoint & 0x3F));
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error("Invalid escape sequence");
                }
            } else if (c == '\0') {
                throw std::runtime_error("Unterminated string");
            } else {
                result += c;
            }
        }
        return result;
    }
    
    JsonValue parseNumber() {
        size_t start = pos_;
        if (peek() == '-') pos_++;
        while (std::isdigit(peek())) pos_++;
        if (peek() == '.') {
            pos_++;
            while (std::isdigit(peek())) pos_++;
        }
        if (peek() == 'e' || peek() == 'E') {
            pos_++;
            if (peek() == '+' || peek() == '-') pos_++;
            while (std::isdigit(peek())) pos_++;
        }
        return std::stod(input_.substr(start, pos_ - start));
    }
    
    JsonValue parseArray() {
        if (get() != '[') throw std::runtime_error("Expected '['");
        
        JsonArray arr;
        skipWhitespace();
        
        if (peek() == ']') {
            pos_++;
            return arr;
        }
        
        while (true) {
            arr.push_back(parseValue());
            skipWhitespace();
            if (peek() == ']') {
                pos_++;
                break;
            }
            if (get() != ',') throw std::runtime_error("Expected ',' or ']'");
            skipWhitespace();
        }
        return arr;
    }
    
    JsonValue parseObject() {
        if (get() != '{') throw std::runtime_error("Expected '{'");
        
        JsonObject obj;
        skipWhitespace();
        
        if (peek() == '}') {
            pos_++;
            return obj;
        }
        
        while (true) {
            skipWhitespace();
            std::string key = parseString();
            skipWhitespace();
            if (get() != ':') throw std::runtime_error("Expected ':'");
            skipWhitespace();
            obj[key] = parseValue();
            skipWhitespace();
            if (peek() == '}') {
                pos_++;
                break;
            }
            if (get() != ',') throw std::runtime_error("Expected ',' or '}'");
        }
        return obj;
    }
    
    JsonValue parseValue() {
        skipWhitespace();
        char c = peek();
        
        if (c == '"') return parseString();
        if (c == '[') return parseArray();
        if (c == '{') return parseObject();
        if (c == 't') {
            if (input_.substr(pos_, 4) == "true") {
                pos_ += 4;
                return true;
            }
            throw std::runtime_error("Invalid token");
        }
        if (c == 'f') {
            if (input_.substr(pos_, 5) == "false") {
                pos_ += 5;
                return false;
            }
            throw std::runtime_error("Invalid token");
        }
        if (c == 'n') {
            if (input_.substr(pos_, 4) == "null") {
                pos_ += 4;
                return nullptr;
            }
            throw std::runtime_error("Invalid token");
        }
        if (c == '-' || std::isdigit(c)) {
            return parseNumber();
        }
        
        throw std::runtime_error("Unexpected character: " + std::string(1, c));
    }
    
public:
    explicit JsonParser(const std::string& input) : input_(input) {}
    
    JsonValue parse() {
        JsonValue result = parseValue();
        skipWhitespace();
        if (pos_ != input_.size()) {
            throw std::runtime_error("Trailing content after JSON");
        }
        return result;
    }
};

// ===== JSON Writer =====
class JsonWriter {
    std::ostringstream ss_;
    bool pretty_ = false;
    int indent_ = 0;
    
    void writeIndent() {
        if (pretty_) {
            for (int i = 0; i < indent_; i++) {
                ss_ << "  ";
            }
        }
    }
    
    void writeString(const std::string& s) {
        ss_ << '"';
        for (char c : s) {
            switch (c) {
                case '"': ss_ << "\\\""; break;
                case '\\': ss_ << "\\\\"; break;
                case '\b': ss_ << "\\b"; break;
                case '\f': ss_ << "\\f"; break;
                case '\n': ss_ << "\\n"; break;
                case '\r': ss_ << "\\r"; break;
                case '\t': ss_ << "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        ss_ << "\\u" << std::hex << std::setw(4) << std::setfill('0') 
                            << static_cast<int>(c);
                    } else {
                        ss_ << c;
                    }
            }
        }
        ss_ << '"';
    }
    
    void writeValue(const JsonValue& val) {
        switch (val.type()) {
            case JsonValue::Type::Null:
                ss_ << "null";
                break;
            case JsonValue::Type::Bool:
                ss_ << (val.asBool() ? "true" : "false");
                break;
            case JsonValue::Type::Number: {
                double n = val.asNumber();
                if (std::floor(n) == n && std::abs(n) < 1e15) {
                    ss_ << static_cast<long long>(n);
                } else {
                    ss_ << std::setprecision(15) << n;
                }
                break;
            }
            case JsonValue::Type::String:
                writeString(val.asString());
                break;
            case JsonValue::Type::Array: {
                const auto& arr = val.asArray();
                ss_ << '[';
                if (!arr.empty()) {
                    if (pretty_) ss_ << '\n';
                    indent_++;
                    for (size_t i = 0; i < arr.size(); i++) {
                        writeIndent();
                        writeValue(arr[i]);
                        if (i + 1 < arr.size()) ss_ << ',';
                        if (pretty_) ss_ << '\n';
                    }
                    indent_--;
                    writeIndent();
                }
                ss_ << ']';
                break;
            }
            case JsonValue::Type::Object: {
                const auto& obj = val.asObject();
                ss_ << '{';
                if (!obj.empty()) {
                    if (pretty_) ss_ << '\n';
                    indent_++;
                    size_t i = 0;
                    for (const auto& [key, value] : obj) {
                        writeIndent();
                        writeString(key);
                        ss_ << ':';
                        if (pretty_) ss_ << ' ';
                        writeValue(value);
                        if (++i < obj.size()) ss_ << ',';
                        if (pretty_) ss_ << '\n';
                    }
                    indent_--;
                    writeIndent();
                }
                ss_ << '}';
                break;
            }
        }
    }
    
public:
    explicit JsonWriter(bool pretty = false) : pretty_(pretty) {}
    
    std::string write(const JsonValue& val) {
        writeValue(val);
        return ss_.str();
    }
};

// ===== Convenience Functions =====
inline JsonValue parseJson(const std::string& json) {
    return JsonParser(json).parse();
}

inline std::string toJson(const JsonValue& val, bool pretty = false) {
    return JsonWriter(pretty).write(val);
}

inline JsonValue loadJsonFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseJson(buffer.str());
}

inline bool saveJsonFile(const std::string& path, const JsonValue& val, bool pretty = true) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << toJson(val, pretty);
    return true;
}

}  // namespace luma
