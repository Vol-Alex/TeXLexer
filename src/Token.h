#pragma once

#include "TokenType.h"

#include <string>

namespace TXL
{
struct Token final
{
    TokenType type;
    std::string content; // TODO: Use string_view
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
