#pragma once

#include "TokenType.h"

#include <string_view>

namespace TXL
{
struct Token final
{
    TokenType type;
    std::string_view content;
};

inline bool operator ==(const Token& l, const Token& r)
{
    return l.type == r.type && l.content == r.content;
}

inline bool operator !=(const Token& l, const Token& r)
{
    return !(l == r);
}
} // namespace TXL
