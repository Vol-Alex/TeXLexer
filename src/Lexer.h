#pragma once

#include "Token.h"

namespace TXL
{
class Lexer final
{
public:
    Lexer();
    Lexer(std::string_view text);
    ~Lexer();

    Token next();

private:
    void* _buffer = nullptr;
};
} // namespace TXL
