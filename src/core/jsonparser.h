#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <sstream>

/**
 * Minimal JSON parser for Aseprite format
 * 
 * Supports only the subset needed for Aseprite JSON:
 * - Objects: { "key": value }
 * - Arrays: [ value1, value2 ]
 * - Strings: "text"
 * - Numbers: 123, 123.45
 * - Booleans: true, false
 */

class JsonValue
{
public:
    enum class Type { Null, Object, Array, String, Number, Boolean };

    JsonValue() : type(Type::Null) {}
    
    Type getType() const { return type; }
    bool isObject() const { return type == Type::Object; }
    bool isArray() const { return type == Type::Array; }
    bool isString() const { return type == Type::String; }
    bool isNumber() const { return type == Type::Number; }
    bool isBool() const { return type == Type::Boolean; }

    // Object access
    const JsonValue& operator[](const std::string& key) const;
    bool has(const std::string& key) const;

    // Array access
    const JsonValue& operator[](size_t index) const;
    size_t size() const;

    // Value getters
    std::string asString() const { return stringValue; }
    int asInt() const { return static_cast<int>(numberValue); }
    double asDouble() const { return numberValue; }
    bool asBool() const { return boolValue; }

    // Factory methods
    static JsonValue makeObject();
    static JsonValue makeArray();
    static JsonValue makeString(const std::string& value);
    static JsonValue makeNumber(double value);
    static JsonValue makeBool(bool value);

    // Mutators (for building)
    void addMember(const std::string& key, JsonValue value);
    void addElement(JsonValue value);

private:
    Type type;
    std::unordered_map<std::string, JsonValue> objectValue;
    std::vector<JsonValue> arrayValue;
    std::string stringValue;
    double numberValue = 0.0;
    bool boolValue = false;

    static JsonValue nullValue;
};

class JsonParser
{
public:
    /**
     * Parse JSON string
     * @return Parsed JsonValue or null value on error
     */
    static JsonValue parse(const std::string& json);

private:
    std::string json;
    size_t pos = 0;

    JsonParser(const std::string& str) : json(str), pos(0) {}

    JsonValue parseValue();
    JsonValue parseObject();
    JsonValue parseArray();
    JsonValue parseString();
    JsonValue parseNumber();
    JsonValue parseBool();
    JsonValue parseNull();

    void skipWhitespace();
    char peek() const;
    char advance();
    bool match(char expected);
    bool isDigit(char c) const;
};
