#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace xyz::engine {

class JsonValue {
public:
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;

    enum class Type {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    JsonValue() = default;
    explicit JsonValue(std::nullptr_t);
    explicit JsonValue(bool value);
    explicit JsonValue(double value);
    explicit JsonValue(std::string value);
    explicit JsonValue(Array value);
    explicit JsonValue(Object value);

    static std::optional<JsonValue> parse(
        std::string_view input,
        std::string_view sourceName,
        std::string& error);
    static std::optional<JsonValue> parseFile(
        const std::filesystem::path& path,
        std::string& error);

    [[nodiscard]] Type type() const noexcept;
    [[nodiscard]] bool isNull() const noexcept;
    [[nodiscard]] bool isBoolean() const noexcept;
    [[nodiscard]] bool isNumber() const noexcept;
    [[nodiscard]] bool isString() const noexcept;
    [[nodiscard]] bool isArray() const noexcept;
    [[nodiscard]] bool isObject() const noexcept;

    [[nodiscard]] bool boolean() const;
    [[nodiscard]] double number() const;
    [[nodiscard]] const std::string& string() const;
    [[nodiscard]] const Array& array() const;
    [[nodiscard]] const Object& object() const;
    [[nodiscard]] const JsonValue* find(std::string_view key) const noexcept;

private:
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value_{nullptr};
};

}
