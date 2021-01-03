#pragma once

#include "Token.h"

namespace TXL
{
class Lexer final
{
public:
    Lexer(std::string_view text);
    ~Lexer();

    Token next();

private:
    void* _buffer;
};
} // namespace TXL
