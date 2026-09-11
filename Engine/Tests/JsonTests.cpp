#include <cstdlib>
#include <iostream>
#include <string>

#include "XYZ/Engine/Json.h"

int main() {
    const auto check = [](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "Json test failed: " << message << "\n";
            std::exit(1);
        }
    };

    std::string error;
    const auto document = xyz::engine::JsonValue::parse(
        R"({"name":"walk","loop":true,"phase":0.5,"nodes":[null,{"id":"left_thigh"}]})",
        "memory",
        error);
    check(document.has_value(), "valid JSON parses");
    check(document->find("name")->string() == "walk", "strings are readable");
    check(document->find("loop")->boolean(), "booleans are readable");
    check(document->find("phase")->number() == 0.5, "numbers are readable");
    check(document->find("nodes")->array().size() == 2, "arrays are readable");
    check(document->find("nodes")->array()[0].isNull(), "null is readable");

    const auto malformed = xyz::engine::JsonValue::parse("{\"missing\":", "memory", error);
    check(!malformed.has_value(), "malformed JSON fails");
    check(error.find("memory") != std::string::npos, "parse error names its source");

    std::cout << "Json tests passed.\n";
}
