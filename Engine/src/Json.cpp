#include "XYZ/Engine/Json.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace xyz::engine {
namespace {

class JsonParser {
public:
    JsonParser(std::string_view input, std::string_view sourceName, std::string& error)
        : input_(input), sourceName_(sourceName), error_(error) {}

    std::optional<JsonValue> parseDocument() {
        skipWhitespace();
        JsonValue value;
        if (!parseValue(value)) {
            return std::nullopt;
        }
        skipWhitespace();
        if (offset_ != input_.size()) {
            fail("unexpected trailing input");
            return std::nullopt;
        }
        return value;
    }

private:
    bool parseValue(JsonValue& value) {
        if (offset_ >= input_.size()) {
            return fail("expected a JSON value");
        }

        switch (input_[offset_]) {
            case 'n':
                if (consumeLiteral("null")) {
                    value = JsonValue(nullptr);
                    return true;
                }
                return false;
            case 't':
                if (consumeLiteral("true")) {
                    value = JsonValue(true);
                    return true;
                }
                return false;
            case 'f':
                if (consumeLiteral("false")) {
                    value = JsonValue(false);
                    return true;
                }
                return false;
            case '"': {
                std::string parsed;
                if (!parseString(parsed)) {
                    return false;
                }
                value = JsonValue(std::move(parsed));
                return true;
            }
            case '[':
                return parseArray(value);
            case '{':
                return parseObject(value);
            default:
                if (input_[offset_] == '-' || std::isdigit(static_cast<unsigned char>(input_[offset_]))) {
                    double parsed = 0.0;
                    if (!parseNumber(parsed)) {
                        return false;
                    }
                    value = JsonValue(parsed);
                    return true;
                }
                return fail("unexpected character while reading a JSON value");
        }
    }

    bool parseArray(JsonValue& value) {
        ++offset_;
        JsonValue::Array array;
        skipWhitespace();
        if (consume(']')) {
            value = JsonValue(std::move(array));
            return true;
        }

        while (true) {
            JsonValue element;
            skipWhitespace();
            if (!parseValue(element)) {
                return false;
            }
            array.push_back(std::move(element));
            skipWhitespace();
            if (consume(']')) {
                value = JsonValue(std::move(array));
                return true;
            }
            if (!consume(',')) {
                return fail("expected ',' or ']' in JSON array");
            }
        }
    }

    bool parseObject(JsonValue& value) {
        ++offset_;
        JsonValue::Object object;
        skipWhitespace();
        if (consume('}')) {
            value = JsonValue(std::move(object));
            return true;
        }

        while (true) {
            skipWhitespace();
            if (offset_ >= input_.size() || input_[offset_] != '"') {
                return fail("expected a quoted object key");
            }

            std::string key;
            if (!parseString(key)) {
                return false;
            }
            skipWhitespace();
            if (!consume(':')) {
                return fail("expected ':' after JSON object key");
            }
            skipWhitespace();
            if (object.contains(key)) {
                return fail("duplicate JSON object key: " + key);
            }

            JsonValue child;
            if (!parseValue(child)) {
                return false;
            }
            object.emplace(std::move(key), std::move(child));
            skipWhitespace();
            if (consume('}')) {
                value = JsonValue(std::move(object));
                return true;
            }
            if (!consume(',')) {
                return fail("expected ',' or '}' in JSON object");
            }
        }
    }

    bool parseString(std::string& value) {
        if (!consume('"')) {
            return fail("expected a JSON string");
        }

        while (offset_ < input_.size()) {
            const unsigned char character = static_cast<unsigned char>(input_[offset_++]);
            if (character == '"') {
                return true;
            }
            if (character < 0x20U) {
                return fail("control character in JSON string");
            }
            if (character != '\\') {
                value.push_back(static_cast<char>(character));
                continue;
            }

            if (offset_ >= input_.size()) {
                return fail("unterminated JSON escape");
            }
            const char escape = input_[offset_++];
            switch (escape) {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'b': value.push_back('\b'); break;
                case 'f': value.push_back('\f'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                case 'u': {
                    std::uint32_t codePoint = 0;
                    if (!parseUnicodeEscape(codePoint)) {
                        return false;
                    }
                    appendUtf8(value, codePoint);
                    break;
                }
                default:
                    return fail("invalid escape in JSON string");
            }
        }

        return fail("unterminated JSON string");
    }

    bool parseUnicodeEscape(std::uint32_t& codePoint) {
        if (input_.size() - offset_ < 4) {
            return fail("incomplete Unicode escape in JSON string");
        }

        for (int index = 0; index < 4; ++index) {
            const char character = input_[offset_++];
            codePoint <<= 4U;
            if (character >= '0' && character <= '9') {
                codePoint += static_cast<std::uint32_t>(character - '0');
            } else if (character >= 'a' && character <= 'f') {
                codePoint += static_cast<std::uint32_t>(character - 'a' + 10);
            } else if (character >= 'A' && character <= 'F') {
                codePoint += static_cast<std::uint32_t>(character - 'A' + 10);
            } else {
                return fail("invalid Unicode escape in JSON string");
            }
        }

        if (codePoint >= 0xD800U && codePoint <= 0xDFFFU) {
            return fail("surrogate Unicode escapes are not supported");
        }
        return true;
    }

    static void appendUtf8(std::string& value, std::uint32_t codePoint) {
        if (codePoint <= 0x7FU) {
            value.push_back(static_cast<char>(codePoint));
        } else if (codePoint <= 0x7FFU) {
            value.push_back(static_cast<char>(0xC0U | (codePoint >> 6U)));
            value.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
        } else {
            value.push_back(static_cast<char>(0xE0U | (codePoint >> 12U)));
            value.push_back(static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU)));
            value.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
        }
    }

    bool parseNumber(double& value) {
        const std::size_t start = offset_;
        if (consume('-')) {
            if (offset_ >= input_.size()) {
                return fail("incomplete JSON number");
            }
        }

        if (consume('0')) {
            if (offset_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[offset_]))) {
                return fail("leading zero in JSON number");
            }
        } else {
            if (offset_ >= input_.size() || input_[offset_] < '1' || input_[offset_] > '9') {
                return fail("invalid JSON number");
            }
            while (offset_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[offset_]))) {
                ++offset_;
            }
        }

        if (consume('.')) {
            const std::size_t fractionStart = offset_;
            while (offset_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[offset_]))) {
                ++offset_;
            }
            if (fractionStart == offset_) {
                return fail("JSON number requires digits after decimal point");
            }
        }

        if (offset_ < input_.size() && (input_[offset_] == 'e' || input_[offset_] == 'E')) {
            ++offset_;
            if (offset_ < input_.size() && (input_[offset_] == '+' || input_[offset_] == '-')) {
                ++offset_;
            }
            const std::size_t exponentStart = offset_;
            while (offset_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[offset_]))) {
                ++offset_;
            }
            if (exponentStart == offset_) {
                return fail("JSON number requires exponent digits");
            }
        }

        const std::string token(input_.substr(start, offset_ - start));
        char* end = nullptr;
        value = std::strtod(token.c_str(), &end);
        if (end == nullptr || end != token.c_str() + token.size() || !std::isfinite(value)) {
            return fail("JSON number is not finite");
        }
        return true;
    }

    bool consumeLiteral(std::string_view literal) {
        if (input_.substr(offset_, literal.size()) != literal) {
            fail("invalid JSON literal");
            return false;
        }
        offset_ += literal.size();
        return true;
    }

    bool consume(char expected) {
        if (offset_ < input_.size() && input_[offset_] == expected) {
            ++offset_;
            return true;
        }
        return false;
    }

    void skipWhitespace() noexcept {
        while (offset_ < input_.size()
               && std::isspace(static_cast<unsigned char>(input_[offset_]))) {
            ++offset_;
        }
    }

    bool fail(const std::string& message) {
        if (error_.empty()) {
            std::ostringstream stream;
            stream << sourceName_ << ":" << offset_ << ": " << message;
            error_ = stream.str();
        }
        return false;
    }

    std::string_view input_;
    std::string_view sourceName_;
    std::string& error_;
    std::size_t offset_ = 0;
};

} // namespace

JsonValue::JsonValue(std::nullptr_t)
    : value_(nullptr) {}

JsonValue::JsonValue(bool value)
    : value_(value) {}

JsonValue::JsonValue(double value)
    : value_(value) {}

JsonValue::JsonValue(std::string value)
    : value_(std::move(value)) {}

JsonValue::JsonValue(Array value)
    : value_(std::move(value)) {}

JsonValue::JsonValue(Object value)
    : value_(std::move(value)) {}

std::optional<JsonValue> JsonValue::parse(
    std::string_view input,
    std::string_view sourceName,
    std::string& error) {
    error.clear();
    return JsonParser(input, sourceName, error).parseDocument();
}

std::optional<JsonValue> JsonValue::parseFile(
    const std::filesystem::path& path,
    std::string& error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = path.string() + ": could not open JSON file";
        return std::nullopt;
    }

    const std::string contents{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()};
    return parse(contents, path.string(), error);
}

JsonValue::Type JsonValue::type() const noexcept {
    switch (value_.index()) {
        case 0: return Type::Null;
        case 1: return Type::Boolean;
        case 2: return Type::Number;
        case 3: return Type::String;
        case 4: return Type::Array;
        case 5: return Type::Object;
        default: return Type::Null;
    }
}

bool JsonValue::isNull() const noexcept { return std::holds_alternative<std::nullptr_t>(value_); }
bool JsonValue::isBoolean() const noexcept { return std::holds_alternative<bool>(value_); }
bool JsonValue::isNumber() const noexcept { return std::holds_alternative<double>(value_); }
bool JsonValue::isString() const noexcept { return std::holds_alternative<std::string>(value_); }
bool JsonValue::isArray() const noexcept { return std::holds_alternative<Array>(value_); }
bool JsonValue::isObject() const noexcept { return std::holds_alternative<Object>(value_); }

bool JsonValue::boolean() const { return std::get<bool>(value_); }
double JsonValue::number() const { return std::get<double>(value_); }
const std::string& JsonValue::string() const { return std::get<std::string>(value_); }
const JsonValue::Array& JsonValue::array() const { return std::get<Array>(value_); }
const JsonValue::Object& JsonValue::object() const { return std::get<Object>(value_); }

const JsonValue* JsonValue::find(std::string_view key) const noexcept {
    const auto* object = std::get_if<Object>(&value_);
    if (object == nullptr) {
        return nullptr;
    }
    const auto iterator = object->find(std::string(key));
    return iterator == object->end() ? nullptr : &iterator->second;
}

}
