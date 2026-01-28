#include "jsonparser.h"
#include <cctype>
#include <stdexcept>

JsonValue JsonValue::nullValue;

const JsonValue& JsonValue::operator[](const std::string& key) const
{
    if (!isObject()) return nullValue;
    auto it = objectValue.find(key);
    return (it != objectValue.end()) ? it->second : nullValue;
}

bool JsonValue::has(const std::string& key) const
{
    return isObject() && objectValue.find(key) != objectValue.end();
}

const JsonValue& JsonValue::operator[](size_t index) const
{
    if (!isArray() || index >= arrayValue.size()) return nullValue;
    return arrayValue[index];
}

size_t JsonValue::size() const
{
    if (isArray()) return arrayValue.size();
    if (isObject()) return objectValue.size();
    return 0;
}

JsonValue JsonValue::makeObject()
{
    JsonValue v;
    v.type = Type::Object;
    return v;
}

JsonValue JsonValue::makeArray()
{
    JsonValue v;
    v.type = Type::Array;
    return v;
}

JsonValue JsonValue::makeString(const std::string& value)
{
    JsonValue v;
    v.type = Type::String;
    v.stringValue = value;
    return v;
}

JsonValue JsonValue::makeNumber(double value)
{
    JsonValue v;
    v.type = Type::Number;
    v.numberValue = value;
    return v;
}

JsonValue JsonValue::makeBool(bool value)
{
    JsonValue v;
    v.type = Type::Boolean;
    v.boolValue = value;
    return v;
}

void JsonValue::addMember(const std::string& key, JsonValue value)
{
    if (isObject())
    {
        objectValue[key] = std::move(value);
    }
}

void JsonValue::addElement(JsonValue value)
{
    if (isArray())
    {
        arrayValue.push_back(std::move(value));
    }
}

// Parser implementation

JsonValue JsonParser::parse(const std::string& json)
{
    JsonParser parser(json);
    parser.skipWhitespace();
    return parser.parseValue();
}

void JsonParser::skipWhitespace()
{
    while (pos < json.length() && std::isspace(json[pos]))
    {
        pos++;
    }
}

char JsonParser::peek() const
{
    return (pos < json.length()) ? json[pos] : '\0';
}

char JsonParser::advance()
{
    return (pos < json.length()) ? json[pos++] : '\0';
}

bool JsonParser::match(char expected)
{
    skipWhitespace();
    if (peek() == expected)
    {
        advance();
        return true;
    }
    return false;
}

bool JsonParser::isDigit(char c) const
{
    return c >= '0' && c <= '9';
}

JsonValue JsonParser::parseValue()
{
    skipWhitespace();
    char c = peek();

    if (c == '{') return parseObject();
    if (c == '[') return parseArray();
    if (c == '"') return parseString();
    if (c == 't' || c == 'f') return parseBool();
    if (c == 'n') return parseNull();
    if (c == '-' || isDigit(c)) return parseNumber();

    return JsonValue(); // null
}

JsonValue JsonParser::parseObject()
{
    JsonValue obj = JsonValue::makeObject();
    
    if (!match('{')) return obj;

    skipWhitespace();
    if (match('}')) return obj; // Empty object

    while (true)
    {
        skipWhitespace();
        
        // Parse key
        if (peek() != '"') break;
        JsonValue key = parseString();
        
        skipWhitespace();
        if (!match(':')) break;
        
        // Parse value
        JsonValue value = parseValue();
        obj.addMember(key.asString(), std::move(value));
        
        skipWhitespace();
        if (match('}')) break;
        if (!match(',')) break;
    }

    return obj;
}

JsonValue JsonParser::parseArray()
{
    JsonValue arr = JsonValue::makeArray();
    
    if (!match('[')) return arr;

    skipWhitespace();
    if (match(']')) return arr; // Empty array

    while (true)
    {
        JsonValue value = parseValue();
        arr.addElement(std::move(value));
        
        skipWhitespace();
        if (match(']')) break;
        if (!match(',')) break;
    }

    return arr;
}

JsonValue JsonParser::parseString()
{
    if (!match('"')) return JsonValue::makeString("");

    std::string result;
    while (pos < json.length())
    {
        char c = advance();
        
        if (c == '"') break;
        
        if (c == '\\')
        {
            // Handle escape sequences
            char next = advance();
            switch (next)
            {
            case '"':  result += '"'; break;
            case '\\': result += '\\'; break;
            case '/':  result += '/'; break;
            case 'b':  result += '\b'; break;
            case 'f':  result += '\f'; break;
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            case 't':  result += '\t'; break;
            default:   result += next; break;
            }
        }
        else
        {
            result += c;
        }
    }

    return JsonValue::makeString(result);
}

JsonValue JsonParser::parseNumber()
{
    std::string numStr;
    
    if (peek() == '-')
    {
        numStr += advance();
    }

    while (isDigit(peek()))
    {
        numStr += advance();
    }

    if (peek() == '.')
    {
        numStr += advance();
        while (isDigit(peek()))
        {
            numStr += advance();
        }
    }

    if (peek() == 'e' || peek() == 'E')
    {
        numStr += advance();
        if (peek() == '+' || peek() == '-')
        {
            numStr += advance();
        }
        while (isDigit(peek()))
        {
            numStr += advance();
        }
    }

    return JsonValue::makeNumber(std::stod(numStr));
}

JsonValue JsonParser::parseBool()
{
    if (json.substr(pos, 4) == "true")
    {
        pos += 4;
        return JsonValue::makeBool(true);
    }
    if (json.substr(pos, 5) == "false")
    {
        pos += 5;
        return JsonValue::makeBool(false);
    }
    return JsonValue();
}

JsonValue JsonParser::parseNull()
{
    if (json.substr(pos, 4) == "null")
    {
        pos += 4;
    }
    return JsonValue();
}
