// utils/ColorUtils.hpp

#pragma once
#include <string>
#include <stdexcept>
#include <cstdint>

struct RgbaColor
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
};

inline RgbaColor hexToRgba(const std::string &hex)
{
    // Aceita: "#RGB", "#RRGGBB", "#RRGGBBAA"
    std::string h = hex;
    if (!h.empty() && h[0] == '#')
        h = h.substr(1);

    // Expande forma curta: "RGB" → "RRGGBB"
    if (h.size() == 3)
        h = {h[0], h[0], h[1], h[1], h[2], h[2]};

    if (h.size() != 6 && h.size() != 8)
        throw std::invalid_argument("Invalid hex color: " + hex);

    auto hexByte = [&](size_t pos) -> uint8_t
    {
        return static_cast<uint8_t>(std::stoul(h.substr(pos, 2), nullptr, 16));
    };

    RgbaColor c;
    c.r = hexByte(0);
    c.g = hexByte(2);
    c.b = hexByte(4);
    c.a = (h.size() == 8) ? hexByte(6) : 255;
    return c;
}