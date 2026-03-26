#pragma once

#include <string>

class UrlUtils
{
public:
    static std::string decode(const std::string &encoded)
    {
        std::string decoded;
        for (size_t i = 0; i < encoded.size(); ++i)
        {
            if (encoded[i] == '%' && i + 2 < encoded.size())
            {
                int val = std::stoi(encoded.substr(i + 1, 2), nullptr, 16);
                decoded += static_cast<char>(val);
                i += 2;
            }
            else if (encoded[i] == '+')
            {
                decoded += ' ';
            }
            else
            {
                decoded += encoded[i];
            }
        }
        return decoded;
    }
};
