#pragma once

#include <string>

struct TempPath {
    std::string value;
    explicit TempPath(std::string v) : value(std::move(v)) {}
};